
Compiling and running MySQL connector
Author: Chen Liu, Sep. 2014

1. Install MySQL by the following instructions on http://dev.mysql.com/doc/refman/5.7/en/installing.html
   
 1.1) For Ubuntu:
      shell> sudo apt-get install mysql-server

 1.2) For MAC OS:
   Follow the instructions on http://dev.mysql.com/doc/refman/5.5/en/macosx-installation-pkg.html

   To add env parameter on MAC OS:
   export PATH=$PATH:/usr/local/mysql/bin

2. Install thirdparty libraries:

   shell> cd srch2-ngn/thirdparty/
   shell> ./thirdparty-build.sh

3. Compile the SRCH2 engine:

    shell> cd srch2-ngn
    shell> mkdir build
    shell> cd build
    shell> cmake -DBUILD_RELEASE=ON ..
    shell> make -j 4

4. Compile the MySQL connector:

    shell> cd srch2-ngn/db_connectors
    shell> mkdir build
    shell> cd build
    shell> cmake ..
    shell> make

5. Start MySQL with binlog enabled (needed only once):

  5.1 Edit the my.cnf to enable binlog mode to make sure we are using
      row-based binary log for MySQL.

    Ubuntu:
       shell> sudo vi /etc/mysql/my.cnf

    MacOS: 
       shell> cd /usr/local/mysql/support-files/
       shell> sudo cp my-huge.cnf /etc/my.cnf

    For both platforms, find the following lines in the my.cnf file:
        #server-id               = 1
        #log_bin                 = /var/log/mysql/mysql-bin.log

    Change them to:
        server-id               = 1
        log_bin                 = /var/log/mysql/mysql-bin.log
        binlog-format            = ROW

  5.2 Stop/start the mysql service:

   Ubuntu:
     shell> sudo /etc/init.d/mysql stop
     shell> sudo /etc/init.d/mysql start

   MacOS
     shell> sudo /usr/local/mysql/support-files/mysql.server stop 
     shell> sudo /usr/local/mysql/support-files/mysql.server start
 
  5.3 Reset the row-based binlog:
        shell> mysql -u root -p
        mysql> reset master;

   Check the binlog status:
        mysql> show variables like 'binlog_format';
        mysql> show master status;
        mysql> show binlog events;

   These commands will show the status of the binlog.

6. shell> cd srch2-ngn/test/wrapper/system_tests
   Set the MySQL username and password in adapter_mysql/conf.xml.

7. Run the system test case:

   shell> cd srch2-ngn/test/wrapper/system_tests
   shell> python ./adapter_mysql/adapter_mysql.py ../../../build/src/server/srch2-search-server ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt

   If everything passes, we will see a "Successful" message.
