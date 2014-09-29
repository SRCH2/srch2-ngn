#!/bin/bash
echo "BUILDING JSONCPP..."
cd ./json
if [ ! -d "jsoncpp-src" ]; then
 tar -xf jsoncpp-src-0.6.0.tar.gz
 mv jsoncpp-src-0.6.0 jsoncpp-src
fi
if [ ! -f "jsoncpp-src/CMakeLists.txt" ]; then
 tar -xf jsoncpp-src-0.6.0.tar.gz -C /tmp/
 cp /tmp/jsoncpp-src-0.6.0/CMakeLists.txt ./jsoncpp-src/CMakeLists.txt
fi
cd jsoncpp-src/
if [ ! -d "android" ]; then
 mkdir android 
fi
cd android
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_CMAKE_HOME/toolchain/android.toolchain.cmake -DANDROID_STANDALONE_TOOLCHAIN=$ANDROID_STANDALONE_TOOLCHAIN -DLIBRARY_OUTPUT_PATH_ROOT=. ..
make

echo "BUILDING LIBEVENT..."
echo "***  PREREQUISITE **** "
echo "1. you have following path <NDK_HOME>/toolchains/<target arch>/prebuilt/<host arch>/bin/"
echo "   appended to PATH env variable"
echo "2. you have defined ANDROID_SYSROOT environment variable. It is a base path for /usr/lib/"
echo "   and /usr/include/ "
echo "----------------------------------------------------------------------------------------"
echo "ANDROID_SYSROOT = " $ANDROID_SYSROOT
echo "PATH = " $PATH
read -s -n 1 key


cd ../../../event
if [ ! -d "android" ]; then
 mkdir android
fi
CURRENTDIR=$(pwd)
if [ ! -d "libevent-2.0.21-stable" ]; then
  tar -xvf libevent-2.0.21-stable.tar.gz
fi
cd libevent-2.0.21-stable
./configure --prefix=$CURRENTDIR/android --host=arm-linux-androideabi CC="arm-linux-androideabi-gcc --sysroot=$ANDROID_SYSROOT" CFLAGS=--sysroot=$ANDROID_SYSROOT  LDFLAGS=--sysroot=$ANDROID_SYSROOT
make install
:
