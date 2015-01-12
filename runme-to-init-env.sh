# Run this file to initialize the development environment 
# by installing used packages

#!/bin/sh

echo "Install required libraries..."

#CENTOS
sudo yum install gcc make gcc-c++ libevent-devel openssl-devel boost-devel boost-static
sudo yum install libboost-dev libboost-program-options-dev libboost-regex-dev libboost-thread-dev libboost-serialization-dev libboost-system-dev mysql-devel python-devel

#UBUNTU
sudo apt-get install gcc make cpp libevent-dev libssl-dev
sudo apt-get install libboost-dev libboost-program-options-dev libboost-regex-dev libboost-thread-dev libboost-serialization-dev libboost-system-dev libmysqlclient-dev python-dev

echo "Press a key to continue..."
read abc

echo "Building thirdparty libraries - jsoncpp and libevent"
cd  thirdparty/
sh thirdparty-build.sh 

cd ..
mkdir build && cd build
cmake .. && make

