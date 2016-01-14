# Run this file to initialize the development environment 
# by installing used packages

#!/bin/sh

echo "Install required libraries..."

#CENTOS
sudo yum install gcc make gcc-c++ cmake kernel-devel libevent-devel openssl-devel boost-devel boost-static
sudo yum install libboost-dev libboost-filesystem-dev libboost-program-options-dev libboost-regex-dev libboost-thread-dev libboost-serialization-dev libboost-system-dev mysql-devel python-devel unixodbc unixodbc-devel

#UBUNTU
sudo apt-get install gcc make cmake build-essential cpp libevent-dev libssl-dev
sudo apt-get install libboost-dev libboost-filesystem-dev libboost-program-options-dev libboost-regex-dev libboost-thread-dev libboost-serialization-dev libboost-system-dev libmysqlclient-dev python-dev unixodbc unixodbc-dev

echo "Press a key to continue..."
read abc

echo "Building thirdparty libraries - jsoncpp and libevent"
cd  thirdparty/
sh thirdparty-build.sh 

cd ..
mkdir build && cd build
cmake .. && make

