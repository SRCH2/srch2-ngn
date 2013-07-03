#!/bin/sh

cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug .
find . -name 'CMakeFiles' -prune -or -name 'cmake_install.cmake' -or -name 'CMakeCache.txt' -or -name 'Makefile' | xargs rm -rf



