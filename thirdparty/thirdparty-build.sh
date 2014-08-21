#!/bin/bash


echo "BUILDING JSONCPP..."

cd ./json
tar -xvf jsoncpp-src-0.6.0.tar.gz
mv jsoncpp-src-0.6.0 jsoncpp-src
cp scons-local-2.1.0.tar.gz jsoncpp-src
cd jsoncpp-src
tar -xvf scons-local-2.1.0.tar.gz
python scons.py platform=linux-gcc

echo "BUILDING LIBEVENT..."

cd ../../event
tar -xvf libevent-2.0.21-stable.tar.gz
cd libevent-2.0.21-stable

CURRENTDIR=$(pwd)
echo "INSTALLING LIBEVENT in $CURRENTDIR/../"

./configure --prefix=$CURRENTDIR/../
make install

cd ../../libunwind
tar -xvf libunwind-0.99-beta.tar.gz
cd libunwind-0.99-beta

CURRENTDIR=$(pwd)
echo "INSTALLING libunwind in $CRRENTDIR/../"
CFLAGS=-fPIC ./configure --prefix=$CURRENTDIR/../
make CFLAGS=-fPIC install

cd ../../gperftools
tar -xvf gperftools-2.0.tar.gz
cd gperftools-2.0

CURRENTDIR=$(pwd)
echo "INSTALLING google perftools in $CURRENTDIR/../"
LDFLAGS=-L$CURRENTDIR/../../libunwind/lib/ ./configure --prefix=$CURRENTDIR/../
make && make install

cd ../..
tar -xf mongodb-linux-x86_64-v2.4-latest.tgz
cd mongo-cxx-driver-v2.4
CURRENTDIR=$(pwd)
echo "Building mongo driver in $CURRENTDIR"
python ../json/jsoncpp-src/scons.py 
