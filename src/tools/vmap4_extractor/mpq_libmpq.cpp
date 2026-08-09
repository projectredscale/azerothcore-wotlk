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
#include <cstdio>
#include <deque>
#include <string>
#include <sys/stat.h>

ArchiveSet gOpenArchives;
DirectorySet gDirectoryArchives;

MPQArchive::MPQArchive(char const* filename)
{
    struct stat status = {};
    if (stat(filename, &status) == 0 && (status.st_mode & S_IFDIR))
    {
        printf("Opening directory %s as unpacked archive\n", filename);
        gDirectoryArchives.emplace_back(filename);
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

    // File not found in any opened archive: fall back to unpacked `patch-*.MPQ` folders.
    if (loadFromDisk(filename))
        return;

    eof = true;
    buffer = nullptr;
}

bool MPQFile::loadFromDisk(char const* filename)
{
    std::string diskPath(filename);
    std::replace(diskPath.begin(), diskPath.end(), '\\', '/');

    for (auto const& directory : gDirectoryArchives)
    {
        std::string fullPath = directory + "/" + diskPath;
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
