#!/usr/bin/env bash
# Build the mapp extractors + dbimport ("all" tools) in Release mode.
# Separate build that lives in ./build.
set -euo pipefail

cd "$(dirname "$0")"

mkdir -p build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTOOLS_BUILD=all \
  -DSCRIPTS=static -DMODULES=static
cmake --build build -j"$(nproc)"
