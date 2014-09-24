Compiling and running MySQL connector
Author: Chen Liu

1. Install MySQL by following instructions on http://dev.mysql.com/doc/refman/5.7/en/installing.html
   For Ubuntu:
   shell> sudo apt-get install mysql-server

2. Install thirdparty libraries:
   To compile the MySQL python driver for MySQL system test case, please install python-dev.
   shell> sudo apt-get install python-dev 

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
   
   Edit the my.cnf to enable binlog mode
   shell> sudo vi /etc/mysql/my.cnf

   Find the following lines in the my.cnf
        #server-id               = 1
        #log_bin                 = /var/log/mysql/mysql-bin.log

   Change them to 
        server-id               = 1
        log_bin                 = /var/log/mysql/mysql-bin.log
        binlog-format		= ROW

   Restart the mysql service
   shell> sudo /etc/init.d/mysql stop
   shell> sudo /etc/init.d/mysql start

   Reset the row based binlog. 
   shell> mysql -u root -p
   mysql> reset master;

   To check binlog status
   mysql> show variables like 'binlog_format';
   mysql> show master status;
   mysql> show binlog events;

6. Set the MySQL password:
   
   shell> cd srch2-ngn/test/wrapper/system_tests
   Set your MySQL password in adapter_mysql/my.cnf and adapter_mysql/conf.xml.

7. Run the system test case:

   shell> cd srch2-ngn/test/wrapper/system_tests
   shell> python ./adapter_mysql/adapter_mysql.py ../../../build/src/server/srch2-search-server     ./adapter_sqlite/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt     ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt     ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt

   If everything works, we will see a "Successful" message.
