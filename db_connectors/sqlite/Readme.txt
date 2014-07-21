How to make the sqlite test case works.

1. Install the sqlite3

How to install the sqlite3: http://www.tutorialspoint.com/sqlite/sqlite_installation.htm

Follow the following steps to install the sqlite:

mkdir ~/tmp/sqlite
cd ~/tmp/sqlite
wget http://www.sqlite.org/2014/sqlite-autoconf-3080500.tar.gz 
tar xvfz sqlite-autoconf-*.tar.gz
cd sqlite-autoconf-*
./configure
make
sudo make install

2. Install the thirdparty
cd srch2-ngn/thirdparty/
./thirdparty-build.sh

3. Compile the engine.
cd srch2-ngn
mkdir build
cd build
cmake -DBUILD_RELEASE=OFF ..
make -j 4

4.Compile the sqlite connector. 
cd srch2-ngn/db_connectors
mkdir build
cd build
cmake ..
make -j4

5.Run the test case
cd srch2-ngn/test/wrapper/system_tests

To run all the test case:
./runme.sh

or to run this sqlite adapter test case:
python ./adapter_sqlite/adapter_sqlite.py ../../../build/src/server/srch2-search-server ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt

After the test, remove the temp files
rm -rf data/ *.idx
rm -rf data/sqlite_data
rm -rf ./adapter_sqlite/srch2Test.db
