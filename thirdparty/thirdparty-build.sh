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

