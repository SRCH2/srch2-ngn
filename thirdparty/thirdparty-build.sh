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
tar -xf mongo-cxx-driver-legacy-0.0-26compat-2.6.2.tar.gz
mv mongo-cxx-driver-legacy-0.0-26compat-2.6.2 mongo-cxx-driver-v2.4
cd mongo-cxx-driver-v2.4
CURRENTDIR=$(pwd)
echo "Building mongo driver in $CURRENTDIR"
python ../json/jsoncpp-src-0.5.0/scons.py --sharedclient --use-system-boost

