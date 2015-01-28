
Compiling and running Oracle connector
Author: Chen Liu, Jan. 2015

This connector allows the SRCH2 engine to index data from Oracle db.  It supports Linux only.

1. Install Oracle database: 
   Follow the instruction : http://www.tecmint.com/oracle-database-11g-release-2-installation-in-linux/

   Add library path :
   sudo sh -c "echo /u01/app/oracle/product/11.2.0/dbhome_2/lib/ > /etc/ld.so.conf.d/oracle.conf"
   sudo ldconfig

2. Install third-party libraries:

   shell> cd srch2-ngn/thirdparty/
   shell> ./thirdparty-build.sh

3. Compile the SRCH2 engine:

    shell> cd srch2-ngn
    shell> mkdir build
    shell> cd build
    shell> cmake -DBUILD_RELEASE=ON ..
    shell> make -j 4

4. Compile the Oracle connector:

    shell> cd srch2-ngn/db_connectors
    shell> mkdir build
    shell> cd build
    shell> cmake ..
    shell> make

5. Start the oracle and create test user 'cdcpub'
   Follow the srch2-ngn/test/wrapper/system_tests/adapter_oracle/readme.txt
   
6. Run the system test case.

  shell> cd srch2-ngn/test/wrapper/system_tests
  shell> vim adapter_oracle/conf.xml
 
  Under the "dbParameters" section, make necessary changes to
  parameters related to the Oracle, such as data source, server, 
  user name, password and table name.

  shell> python ./adapter_oracle/adapter_oracle.py ../../../build/src/server/srch2-search-server ./adapter_oracle/testCreateIndexes_sql.txt ./adapter_oracle/testCreateIndexes.txt ./adapter_oracle/testRunListener_sql.txt ./adapter_oracle/testRunListener.txt ./adapter_oracle/testOfflineLog_sql.txt ./adapter_oracle/testOfflineLog.txt

   If everything passes, we will see a "Successful" message.  Congratulations!
