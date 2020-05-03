#!/bin/sh

# Remove build folder, if exists
if [ -d "$build/" ]; then
    rm -rf build/
fi

mkdir build && cd build

# Make sure CMake is installed
command -v cmake > /dev/null 2>&1 || { echo >&2 "I require CMake but it's not installed. Please get it from <https://cmake.org/download>, then try again."; exit 1; }

# Run CMake, then make, and finally install the stripped binary
cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install/strip
