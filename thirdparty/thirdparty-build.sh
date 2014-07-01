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

cd ../../mongo-cxx-driver
tar -xvf mongo-cxx-driver-legacy-0.0-26compat-2.6.2.tar.gz
mv mongo-cxx-driver-legacy-0.0-26compat-2.6.2 mongo-cxx-driver
cd mongo-cxx-driver
CURRENTDIR=$(pwd)
echo "Building mongodb driver in $CURRENTDIR"
python ../../json/jsoncpp-src-0.5.0/scons.py --prefix=srch2 --use-system-boost --sharedclient --full install-mongoclient

cd ../../pymongo
tar -xvf pymongo.tar.gz
cd pymongo
CURRENTDIR=$(pwd)
echo "Building python mongodb driver in $CURRENTDIR"
