
Instructions on using the Sqlite connector for SRCH2 engine.
Author: Chen Liu

1. Install sqlite3:

In particular, run the following steps to install the sqlite (based on
instruction at http://www.tutorialspoint.com/sqlite/sqlite_installation.htm):

shell> mkdir ~/sqlite
shell> cd ~/sqlite
shell> wget http://www.sqlite.org/2014/sqlite-autoconf-3080500.tar.gz 
shell> tar xvfz sqlite-autoconf-*.tar.gz
shell> cd sqlite-autoconf-*
shell> ./configure 
shell> make
shell> sudo make install

2. Install thirdparty packages (only if if they are not compiled yet):

shell> cd srch2-ngn/thirdparty/
shell> ./thirdparty-build.sh

3. Compile the SRCH2 engine:

shell> cd srch2-ngn
shell> mkdir build
shell> cd build
shell> cmake -DBUILD_RELEASE=OFF ..
shell> make -j 4

4. Compile the sqlite connector:

shell> cd srch2-ngn/db_connectors
shell> mkdir build
shell> cd build
shell> cmake ..
shell> make -j4

5. The following step is needed tentatively:

shell> sudo ln -s /lib64/ld-linux-x86-64.so.2 /lib/ld64.so.1

6. Run the system test case for Sqlite:

shell> cd srch2-ngn/test/wrapper/system_tests
shell> ./runme.sh

To run the system test case for the Sqlite connector:

shell> python ./adapter_sqlite/adapter_sqlite.py ../../../build/src/server/srch2-search-server ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt

After running the system test case, remove the temp files:
shell> rm -rf data/ *.idx
shell> rm -rf data/sqlite_data
shell> rm -rf ./adapter_sqlite/srch2Test.db
