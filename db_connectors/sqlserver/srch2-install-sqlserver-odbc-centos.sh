#!/bin/bash
#Make sure to have openssl installed. The libssl.so and libcrypto.so are in the openssl package.
#yum install openssl-devel 

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

cd /usr/lib64/
sudo ln -s libssl.so libssl.so.6
sudo ln -s libcrypto.so libcrypto.so.6

sudo sh -c "echo /usr/local/lib >> /etc/ld.so.conf"
sudo ldconfig

echo $MSSQLDIR
cd $MSSQLDIR
sudo bash ./install.sh install --force
