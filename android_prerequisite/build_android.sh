#!/bin/bash - 
#===============================================================================
#
#          FILE: build_android.sh
# 
#         USAGE: ./build_android.sh 
# 
#   DESCRIPTION: 
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Jianfeng Jia ()
#       CREATED: 08/20/2013 09:50:03 PM PDT
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

export ANDROID_SDK_HOME="$HOME/software/android-sdk"
export ANDROID_NDK_HOME="$HOME/software/android-ndk"
export ANDROID_CMAKE_HOME="$HOME/software/android-cmake"
SCRIPT_PWD=$PWD

# download android-cmake 
[ ! -d $ANDROID_CMAKE_HOME ] && hg clone https://code.google.com/p/android-cmake/ $ANDROID_CMAKE_HOME

./install_sdk_ndk.sh
[ $? -ne 0 ] && { echo "install sdk and ndk error"; exit -1;}

export ANDROID_STANDALONE_TOOLCHAIN="$HOME/software/android-toolchain-arm"
# default arm:  --toolchain=arm-linux-androideabi-4.6
# set to x86:   --toolchain=x86-4.4.3 --arch=x86
# set to mips:  --toolchain=mipsel-linux-android-4.6 --arch=mips
# TODO: make it work for each chip set automaticlly
#       hint: also can be set using android-cmake by setting ANDROID_ABI
$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh --platform=android-8 \
        --install-dir=$ANDROID_STANDALONE_TOOLCHAIN

ANDTOOLCHAIN=$HOME/software/android-cmake/toolchain/android.toolchain.cmake
if grep -q $ANDTOOLCHAIN $HOME/.bashrc; 
then 
    echo "skip alias"
else
    echo "export ANDROID_STANDALONE_TOOLCHAIN=$ANDROID_STANDALONE_TOOLCHAIN" >> $HOME/.bashrc
    echo "alias android-cmake='cmake -DCMAKE_TOOLCHAIN_FILE=$ANDTOOLCHAIN'" >> $HOME/.bashrc
fi

#Install Boost
if [ `ls -l $ANDROID_STANDALONE_TOOLCHAIN/user/lib/libboost_* | wc -l` -lt 9 ];then
    cd $ANDROID_CMAKE_HOME/common-libs/boost
    ./get_boost.sh
    mv CMakeLists.txt CMakeLists.txt.orig
    cp $SCRIPT_PWD/settings/CMakeLists.Boost_add_serialization.txt CMakeLists.txt
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=$ANDTOOLCHAIN ..
    make -j3
    make install
    cd $SCRIPT_PWD
else
    echo "Boost already installed"
fi

#Install open-ssl
OPENSSL_HOME="$HOME/software/openssl-android"
if [ ! -d $OPENSSL_HOME ];then
    git clone https://github.com/guardianproject/openssl-android.git $OPENSSL_HOME
    cd $OPENSSL_HOME
    # Change the version of 4.* into the exist one by checking 
    # $ANDROID_NDK_HOME/toolchain/*-androideabi-4.*
    $ANDROID_NDK_HOME/ndk-build NDK_TOOLCHAIN_VERSION=4.6
    cp -r libs/armeabi/*.so $ANDROID_STANDALONE_TOOLCHAIN/user/lib/
    cp -r include/openssl   $ANDROID_STANDALONE_TOOLCHAIN/user/include/
fi
