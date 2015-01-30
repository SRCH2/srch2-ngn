cd thirdparty

#Download and Install JSONCpp
cd json

wget http://downloads.sourceforge.net/project/scons/scons-local/2.1.0/scons-local-2.1.0.tar.gz
wget http://downloads.sourceforge.net/project/jsoncpp/jsoncpp/0.6.0-rc2/jsoncpp-src-0.6.0-rc2.tar.gz

tar -xvf jsoncpp-src-0.6.0-rc2.tar.gz
rm -rf jsoncpp-src
mv jsoncpp-src-0.6.0-rc2 jsoncpp-src
cp scons-local-2.1.0.tar.gz jsoncpp-src
cp CMakeLists.txt jsoncpp-src
cd jsoncpp-src
tar -xvf scons-local-2.1.0.tar.gz
python scons.py platform=linux-gcc

if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake ..
make

cd ../../../
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


