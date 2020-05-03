#!/bin/sh

if ! [ $(id -u) = 0 ]; then
   echo "This script needs to be run with root privileges!" >&2
   exit 1
fi

if [ $SUDO_USER ]; then
    real_user=$SUDO_USER
else
    real_user=$(id -un)
fi

# Remove build folder, if exists
if [ -d "$build/" ]; then
    rm -rf build/
fi

# Make sure Sudo is installed
command -v sudo > /dev/null 2>&1 || { echo >&2 "I require Sudo but it's not installed. Please get it from <https://www.sudo.ws/download.html>, then try again."; exit 1; }

sudo -u $real_user mkdir build && cd build

# Make sure CMake is installed
command -v cmake > /dev/null 2>&1 || { echo >&2 "I require CMake but it's not installed. Please get it from <https://cmake.org/download>, then try again."; exit 1; }

# Run CMake, then make, and finally install the stripped binary
sudo -u $real_user cmake -DCMAKE_BUILD_TYPE=Release .. && make
make install/strip
