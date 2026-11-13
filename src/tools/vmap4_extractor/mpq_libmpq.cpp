/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "mpq_libmpq04.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <deque>
#include <map>
#include <string>
#include <sys/stat.h>

#ifndef _WIN32
#include <dirent.h>
#endif

ArchiveSet gOpenArchives;
DirectorySet gDirectoryArchives;

namespace
{
bool EqualsNoCase(char const* left, char const* right)
{
    while (*left && *right)
    {
        if (std::tolower(static_cast<unsigned char>(*left)) != std::tolower(static_cast<unsigned char>(*right)))
            return false;

        ++left;
        ++right;
    }

    return *left == *right;
}

std::string ToLower(std::string text)
{
    for (char& character : text)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    return text;
}

#if defined(_WIN32)
// A Windows filesystem resolves paths without regard to case on its own.
std::string FindEntryNoCase(std::string const& directory, std::string const& name)
{
    return name;
}
#else
// The spelling `name` has inside `directory`, empty when there is none. Archive paths are
// case-insensitive in the client; a loose file on this filesystem is not.
std::string FindEntryNoCase(std::string const& directory, std::string const& name)
{
    DIR* entries = opendir(directory.c_str());
    if (!entries)
        return std::string();

    std::string match;
    while (dirent* entry = readdir(entries))
    {
        if (EqualsNoCase(entry->d_name, name.c_str()))
        {
            match = entry->d_name;
            break;
        }
    }

    closedir(entries);
    return match;
}
#endif

// `relativePath` below `root` with every component spelled the way it is on disk.
bool ResolveOnDisk(std::string const& root, std::string const& relativePath, std::string& resolved)
{
    resolved = root;
    std::size_t position = 0;

    while (position < relativePath.size())
    {
        std::size_t const slash = relativePath.find('/', position);
        std::size_t const end = slash == std::string::npos ? relativePath.size() : slash;
        std::string const component = relativePath.substr(position, end - position);
        position = end + 1;

        if (component.empty() || component == ".")
            continue;

        std::string candidate = resolved + "/" + component;

        struct stat status = {};
        if (stat(candidate.c_str(), &status) != 0)
        {
            std::string const match = FindEntryNoCase(resolved, component);
            if (match.empty())
                return false;

            candidate = resolved + "/" + match;
        }

        resolved = candidate;
    }

    return true;
}
} // namespace

MPQArchive::MPQArchive(char const* filename)
{
    struct stat status = {};
    if (stat(filename, &status) == 0 && (status.st_mode & S_IFDIR))
    {
        printf("Opening directory %s as unpacked archive\n", filename);
        // Same order as gOpenArchives: the archive opened last wins, which is the highest patch
        // level the caller opened.
        gDirectoryArchives.push_front(filename);
        return;
    }

    int result = libmpq__archive_open(&mpq_a, filename, -1);
    printf("Opening %s\n", filename);
    if (result)
    {
        switch (result)
        {
            case LIBMPQ_ERROR_OPEN :
                printf("Error opening archive '%s': Does file really exist?\n", filename);
                break;
            case LIBMPQ_ERROR_FORMAT :            /* bad file format */
                printf("Error opening archive '%s': Bad file format\n", filename);
                break;
            case LIBMPQ_ERROR_SEEK :         /* seeking in file failed */
                printf("Error opening archive '%s': Seeking in file failed\n", filename);
                break;
            case LIBMPQ_ERROR_READ :              /* Read error in archive */
                printf("Error opening archive '%s': Read error in archive\n", filename);
                break;
            case LIBMPQ_ERROR_MALLOC :               /* maybe not enough memory? :) */
                printf("Error opening archive '%s': Maybe not enough memory\n", filename);
                break;
            default:
                printf("Error opening archive '%s': Unknown error\n", filename);
                break;
        }
        return;
    }
    opened = true;
    gOpenArchives.push_front(this);
}

void MPQArchive::close()
{
    if (!opened)
        return;
    libmpq__archive_close(mpq_a);
    opened = false;
}

MPQFile::MPQFile(char const* filename):
    eof(false),
    buffer(nullptr),
    pointer(0),
    size(0)
{
    // Unpacked `patch-*.MPQ` folders are the custom patch levels a client mounts over the stock
    // archives, so they are read first: a loose file replaces the archive copy of the same path.
    if (loadFromDisk(filename))
        return;

    for (auto & gOpenArchive : gOpenArchives)
    {
        mpq_archive* mpq_a = gOpenArchive->mpq_a;

        uint32 filenum;
        if (libmpq__file_number(mpq_a, filename, &filenum)) continue;
        libmpq__off_t transferred;
        libmpq__file_unpacked_size(mpq_a, filenum, &size);

        // HACK: in patch.mpq some files don't want to open and give 1 for filesize
        if (size <= 1)
        {
            // printf("info: file %s has size %d; considered dummy file.\n", filename, size);
            eof = true;
            buffer = nullptr;
            return;
        }
        buffer = new char[size];

        //libmpq_file_getdata
        libmpq__file_read(mpq_a, filenum, (unsigned char*)buffer, size, &transferred);
        /*libmpq_file_getdata(&mpq_a, hash, fileno, (unsigned char*)buffer);*/
        return;
    }

    eof = true;
    buffer = nullptr;
}

bool MPQFile::loadFromDisk(char const* filename)
{
    std::string diskPath(filename);
    std::replace(diskPath.begin(), diskPath.end(), '\\', '/');

    // Archive paths are case-insensitive, and the extractors ask for the same doodad thousands of
    // times, so each path is resolved once per directory: an empty value is a remembered miss.
    static std::map<std::string, std::string> resolvedPaths;

    for (auto const& directory : gDirectoryArchives)
    {
        std::string const cacheKey = directory + "/" + ToLower(diskPath);
        auto const cached = resolvedPaths.find(cacheKey);
        std::string fullPath;

        if (cached != resolvedPaths.end())
        {
            if (cached->second.empty())
                continue;

            fullPath = cached->second;
        }
        else if (ResolveOnDisk(directory, diskPath, fullPath))
            resolvedPaths.emplace(cacheKey, fullPath);
        else
        {
            resolvedPaths.emplace(cacheKey, std::string());
            continue;
        }

        FILE* file = fopen(fullPath.c_str(), "rb");
        if (!file)
            continue;

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (fileSize <= 0)
        {
            fclose(file);
            continue;
        }

        char* data = new char[fileSize];
        std::size_t bytesRead = fread(data, 1, fileSize, file);
        fclose(file);

        if (bytesRead != static_cast<std::size_t>(fileSize))
        {
            delete[] data;
            continue;
        }

        buffer = data;
        size = fileSize;
        return true;
    }
    return false;
}

std::size_t MPQFile::read(void* dest, std::size_t bytes)
{
    if (eof) return 0;

    std::size_t rpos = pointer + bytes;
    if (rpos > std::size_t(size))
    {
        bytes = size - pointer;
        eof = true;
    }

    memcpy(dest, &(buffer[pointer]), bytes);

    pointer = rpos;

    return bytes;
}

void MPQFile::seek(int offset)
{
    pointer = offset;
    eof = (pointer >= size);
}

void MPQFile::seekRelative(int offset)
{
    pointer += offset;
    eof = (pointer >= size);
}

void MPQFile::close()
{
    delete[] buffer;
    buffer = nullptr;
    eof = true;
}
