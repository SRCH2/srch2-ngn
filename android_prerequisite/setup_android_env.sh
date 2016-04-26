set -e
set -x
#---------------------------------------------------------------------------------------
API_LEVEL=21
ARCH=arm
ANDROID_INSTALL_DIR=$HOME/android
#---------------------------------------------------------------------------------------
NDK_URL=http://dl.google.com/android/repository/android-ndk-r11c-linux-x86_64.zip
ANDROID_CMAKE_URL=https://storage.googleapis.com/google-code-archive-source/v2/code.google.com/android-cmake/source-archive.zip
BOOST_VERSION=46
BOOST_URL=http://sourceforge.net/projects/boost/files/boost/1.${BOOST_VERSION}.0/boost_1_${BOOST_VERSION}_0.tar.gz/download
#---------------------------------------------------------------------------------------

BASE_DIR=`pwd`

NDK_ZIP=`basename $NDK_URL`

[[ -d $ANDROID_INSTALL_DIR ]] || mkdir $ANDROID_INSTALL_DIR

cd $ANDROID_INSTALL_DIR

if [ ! -d android-ndk-r11c ]; then
    #install NDK
    wget $NDK_URL
    unzip $NDK_ZIP > /dev/null
fi

export ANDROID_NDK_HOME=$ANDROID_INSTALL_DIR/android-ndk-r11c

#install cmake toolchain for android
if [ ! -d android-cmake ]; then
    wget $ANDROID_CMAKE_URL 
    unzip `basename $ANDROID_CMAKE_URL` > /dev/null
fi

export CMAKE_TOOLCHAIN_HOME=$ANDROID_INSTALL_DIR/android-cmake

if [ ! -d $ANDROID_INSTALL_DIR/android-toolchain-arm-$API_LEVEL ]; then
    $ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh --platform=android-$API_LEVEL --install-dir=$ANDROID_INSTALL_DIR/android-toolchain-arm-$API_LEVEL
fi

export ANDROID_STANDALONE_TOOLCHAIN="$ANDROID_INSTALL_DIR/android-toolchain-arm-$API_LEVEL"
export CMAKE_TOOLCHAIN=$CMAKE_TOOLCHAIN_HOME/toolchain/android.toolchain.cmake
alias android-cmake='cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN -DLIBRARY_OUTPUT_PATH_ROOT=.'

cd $CMAKE_TOOLCHAIN_HOME/common-libs/boost/
cp $BASE_DIR/settings/CMakeLists.Boost_add_serialization.txt CMakeLists.txt
if [[ ! -d boost_1_${BOOST_VERSION}_0 ]]; then
    wget $BOOST_URL 
    tar -xf `basename $BOOST_URL`
fi
./patch_boost.sh boost_1_${BOOST_VERSION}_0
patch boost_1_${BOOST_VERSION}_0/boost/config/stdlib/libstdcpp3.hpp < $BASE_DIR/settings/libstdcpp3.patch
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN ..
make 
make install

#install open-ssl
OPENSSL_HOME="$ANDROID_INSTALL_DIR/openssl-android"
if [ ! -d $OPENSSL_HOME ];then
    git clone https://github.com/guardianproject/openssl-android.git $OPENSSL_HOME
    cd $OPENSSL_HOME
    # Change the version of 4.* into the exist one by checking 
    # $ANDROID_NDK_HOME/toolchain/*-androideabi-4.*
    $ANDROID_NDK_HOME/ndk-build NDK_TOOLCHAIN_VERSION=4.9
    cp -r libs/armeabi/*.so $ANDROID_STANDALONE_TOOLCHAIN/user/lib/
    cp -r include/openssl   $ANDROID_STANDALONE_TOOLCHAIN/user/include/
fi


if grep -q $ANDTOOLCHAIN $HOME/.bashrc;
then
    echo "skip alias"
else
    echo "export ANDROID_STANDALONE_TOOLCHAIN=$ANDROID_INSTALL_DIR/android-toolchain-arm-$API_LEVEL" >> $HOME/.bashrc
    echo "alias android-cmake='cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN -DLIBRARY_OUTPUT_PATH_ROOT=.'" >> $HOME/.bashrc
fi
