#!/bin/bash


echo "BUILDING JSONCPP..."

cd ./json
tar -xvf jsoncpp-src-0.5.0.tar.gz
cp scons-local-2.1.0.tar.gz jsoncpp-src-0.5.0
cd jsoncpp-src-0.5.0
tar -xvf scons-local-2.1.0.tar.gz
python scons.py platform=linux-gcc

echo "BUILDING LIBEVENT..."

cd ../../event
tar -xvf libevent-2.0.18-stable.tar.gz
cd libevent-2.0.18-stable

CURRENTDIR=$(pwd)
echo "INSTALLING LIBEVENT in $CURRENTDIR/../"

./configure --prefix=$CURRENTDIR/../
make install

cd ../../libunwind
tar -xvf libunwind-0.99-beta.tar.gz
cd libunwind-0.99-beta
CFLAGS=-fPIC ./configure
make CFLAGS=-fPIC
make CFLAGS=-fPIC install

cd ../../gperftools
tar -xvf gperftools-2.0.tar.gz
cd gperftools-2.0.tar.gz

CURRENTDIR=$(pwd)
echo "INSTALLING google perftools in $CURRENTDIR/../"
./configure --perfix=$CURRENTDIR/../
make && make install

