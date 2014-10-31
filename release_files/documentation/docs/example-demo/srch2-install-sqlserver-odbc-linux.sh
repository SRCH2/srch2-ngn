#!/bin/bash
#For CENTOS 64bit and UBUNTU 64bit only. 
#Run this file to install the sqlserver odbc for linux

echo "Install required libraries..."

#CENTOS
sudo yum group install "Development Tools"
sudo yum install openssl-devel wget

#UBUNTU
sudo apt-get install libssl-dev 

mkdir srch2-install-sqlserver-odbc-linux
cd srch2-install-sqlserver-odbc-linux

#Download and install unixODBC
rm -rf unixODBC-2.3.2
wget ftp://ftp.unixodbc.org/pub/unixODBC/unixODBC-2.3.2.tar.gz
tar -xvf unixODBC-2.3.2.tar.gz
rm unixODBC-2.3.2.tar.gz
cd unixODBC-2.3.2
sed -i 's/2:0:0/1:0:0/g' configure
./configure --disable-gui --disable-drivers --enable-iconv --with-iconv-char-enc=UTF8 --with-iconv-ucode-enc=UTF16LE
make 
sudo make install

#Download the MS SQL Server ODBC Driver
cd ..
wget http://download.microsoft.com/download/B/C/D/BCDD264C-7517-4B7D-8159-C99FC5535680/RedHat5/msodbcsql-11.0.2270.0.tar.gz
tar -xvf msodbcsql-11.0.2270.0.tar.gz
rm msodbcsql-11.0.2270.0.tar.gz
cd msodbcsql-11.0.2270.0
MSSQLDIR=$(pwd)

#Create soft link for the MS SQL Server ODBC Driver dependencies 
#UBUNTU 64bit
sudo ln -s /usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libssl.so.6
sudo ln -s /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/lib/x86_64-linux-gnu/libcrypto.so.6

#CENTOS 64bit
sudo ln -s /usr/lib64/libssl.so /usr/lib64/libssl.so.6
sudo ln -s /usr/lib64/libcrypto.so /usr/lib64/libcrypto.so.6

sudo sh -c "echo /usr/local/lib > /etc/ld.so.conf.d/unixODBC-2.3.2.conf"
sudo ldconfig

sudo bash ./install.sh install --force

#Set the unixODBC config file "odbcinst.ini"
sudo sh -c "echo [ODBC Driver 11 for SQL Server] > /usr/local/etc/odbcinst.ini"
sudo sh -c "echo Driver=$MSSQLDIR/lib64/libmsodbcsql-11.0.so.2270.0 >> /usr/local/etc/odbcinst.ini"
sudo sh -c "echo Threading=1 >> /usr/local/etc/odbcinst.ini"
sudo sh -c "echo UsageCount=5 >> /usr/local/etc/odbcinst.ini"
