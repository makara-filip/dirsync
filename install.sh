#!/bin/bash

set -e

cd src
cmake . -B build # prepare CMake cache
cmake --build build # build project and create executables

executable="$(pwd)/build/dirsync"
directory="$(pwd)/build"

echo "DirSync executable located in: '$executable'"
