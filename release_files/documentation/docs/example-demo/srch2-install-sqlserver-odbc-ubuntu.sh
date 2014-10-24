#!/bin/bash

rm -rf unixODBC-2.3.2
wget ftp://ftp.unixodbc.org/pub/unixODBC/unixODBC-2.3.2.tar.gz
tar -xvf unixODBC-2.3.2.tar.gz
cd unixODBC-2.3.2
sed -i 's/2:0:0/1:0:0/g' configure
./configure --disable-gui --disable-drivers --enable-iconv --with-iconv-char-enc=UTF8 --with-iconv-ucode-enc=UTF16LE
make
sudo make install

cd ..
wget http://download.microsoft.com/download/B/C/D/BCDD264C-7517-4B7D-8159-C99FC5535680/RedHat5/msodbcsql-11.0.2270.0.tar.gz
tar -xvf msodbcsql-11.0.2270.0.tar.gz
cd msodbcsql-11.0.2270.0
MSSQLDIR=$(pwd)
cd ..

wget http://security.ubuntu.com/ubuntu/pool/universe/o/openssl098/libssl0.9.8_0.9.8o-7ubuntu3.2.14.04.1_amd64.deb
sudo dpkg -i libssl0.9.8_0.9.8o-7ubuntu3.2.14.04.1_amd64.deb
cd /usr/lib/x86_64-linux-gnu/
sudo ln -s libssl.so.0.9.8 libssl.so.6
sudo ln -s libcrypto.so.0.9.8 libcrypto.so.6
sudo sh -c "echo /usr/loacl/lib >> /etc/ld.so.conf"
sudo ldconfig

echo $MSSQLDIR
cd $MSSQLDIR
sudo bash ./install.sh install --force
