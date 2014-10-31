
Compiling and running SQL Server connector
Author: Chen Liu, Oct. 2014

This connector allows the SRCH2 engine to index data from Microsoft
SQL Server.  It supports Linux only.

1. Install SQL Server ODBC driver and driver manager:

 1.1) For Redhat and SUSE linux distributions: follow instructions on
     http://msdn.microsoft.com/en-us/library/hh568451%28v=sql.110%29.aspx  

 1.2) For Ubuntu: Follow instructions at
    http://www.codesynthesis.com/~boris/blog/2011/12/02/microsoft-sql-server-odbc-driver-linux/
    Here are the details of the steps.

    To install unixODBC (ODBC Driver Manager for Linux) :
      shell> sudo apt-get remove libodbc1 unixodbc unixodbc-dev
      shell> wget ftp://ftp.unixodbc.org/pub/unixODBC/unixODBC-2.3.2.tar.gz
      shell> tar -xvf unixODBC-2.3.2.tar.gz
      shell> cd unixODBC-2.3.2

    Open the file "configure" and search for the line:
      LIB_VERSION="2:0:0"
    It's the version of the unixODBC library. Change it to:
      LIB_VERSION="1:0:0"  

    Then run the following commands to install unixODBC:
      shell> ./configure --disable-gui --disable-drivers --enable-iconv --with-iconv-char-enc=UTF8 --with-iconv-ucode-enc=UTF16LE
      shell> make
      shell> sudo make install

    To install MS ODBC Driver 11 for SQL Server for Ubuntu, download the driver for RedHat5 Linux at:
      http://www.microsoft.com/en-us/download/details.aspx?id=36437
      In particular:

      shell> wget http://download.microsoft.com/download/B/C/D/BCDD264C-7517-4B7D-8159-C99FC5535680/RedHat5/msodbcsql-11.0.2270.0.tar.gz
      shell> tar -xvf msodbcsql*.tar.gz
      shell> cd msodbcsql*
      shell> cd lib64
      shell> ldd libmsodbcsql* | grep -i "not found"

    It shows the missing libraries. The next step is to install
    these missing libraries. The general approach is to enter the
    library name in the Ubuntu Package Search (packages.ubuntu.com).

    For instance, suppose the following three libraries are missing:
      libcrypto.so.6
      libssl.so.6
      libodbcinst.so.1

    To install the first two, download and install the package libssl-dev for Ubuntu or openssl-devel for CentOS.
    For CentOS 64bit, run the following commands :
      shell> sudo yum install openssl-devel
      shell> sudo ln -s /usr/lib64/libssl.so /usr/lib64/libssl.so.6
      shell> sudo ln -s /usr/lib64/libcrypto.so /usr/lib64/libcrypto.so.6
      
    For Ubuntu 64bit, run the following commands :
      shell> sudo apt-get install libssl-dev 
      shell> sudo ln -s /usr/lib/x86_64-linux-gnu/libssl.so /usr/lib/x86_64-linux-gnu/libssl.so.6
      shell> sudo ln -s /usr/lib/x86_64-linux-gnu/libcrypto.so /usr/lib/x86_64-linux-gnu/libcrypto.so.6

    To find the libodbcinst.so.1, add a file "unixODBC-2.3.2.conf" including 
    path "/usr/loacl/lib" to the folder "/etc/ld.so.conf.d/":
      shell> sudo sh -c "echo /usr/local/lib > /etc/ld.so.conf.d/unixODBC-2.3.2.conf"
      shell> sudo ldconfig

    Once all the dependencies are met, run the following command to 
    install the the MS SQL Server Driver and add the info to the file "/usr/local/etc/odbcinst.ini" :
      shell> sudo bash ./install.sh install --force
      shell> sudo sh -c "echo [ODBC Driver 11 for SQL Server] > /usr/local/etc/odbcinst.ini"
      shell> sudo sh -c "echo Driver=$MSSQLDIR/lib64/libmsodbcsql-11.0.so.2270.0 >> /usr/local/etc/odbcinst.ini"
      shell> sudo sh -c "echo Threading=1 >> /usr/local/etc/odbcinst.ini"
      shell> sudo sh -c "echo UsageCount=5 >> /usr/local/etc/odbcinst.ini"
    In particular, ```$MSSQLDIR``` is the path of the folder ```msodbcsql-11.0.2270.0```.  

    To test the installation, run the following command to try to
    connect a Microsoft SQL Server on a Windows server:
      shell> sqlcmd -S [MSSERVER-HOST] -U admin
      [Type in admin password]

    To Create a test account for the system test:
      sql@admin 1> CREATE LOGIN srch2 WITH PASSWORD = 'srch2'
      sql@admin 2> go
      sql@admin 1> CREATE DATABASE demo
      sql@admin 2> go
      sql@admin 1> USE demo
      sql@admin 2> go
      sql@admin 1> CREATE USER srch2 FOR LOGIN srch2
      sql@admin 2> go
      sql@admin 1> GRANT ALTER, CONTROL TO srch2
      sql@admin 2> go
      sql@admin 1> use master
      sql@admin 2> go
      sql@admin 1> GRANT CREATE ANY DATABASE to srch2
      sql@admin 2> go

2. Install third-party libraries:

   shell> cd srch2-ngn/thirdparty/
   shell> ./thirdparty-build.sh

3. Compile the SRCH2 engine:

    shell> cd srch2-ngn
    shell> mkdir build
    shell> cd build
    shell> cmake -DBUILD_RELEASE=ON ..
    shell> make -j 4

4. Compile the MS SQL Server connector:

    shell> cd srch2-ngn/db_connectors
    shell> mkdir build
    shell> cd build
    shell> cmake ..
    shell> make

5. For each SQL Server table used by the SRCH2 engine, we need to
   enable the "Change Tracking" feature for both this table and the
   database it belongs to.  To enable this feature, do the following
   steps on the Windows machine of SQL Server: 

   Option 1: use MS SQL Server Management Studio.
      - On Windows, open the MS SQL Server Management Studio. 
      - Select the target database, right click on the database,
        choose "Properties"->"Change Tracking", enable the "Change Tracking"
	for this database.
      - Select the target table, right click on the table, choose
        "Properties"->"Change Tracking", enable the "Change Tracking" for this
        table. 

   Option 2: use a MS SQL Server Shell.
      shell> sqlcmd -S [MSSERVER-HOST] -U srch2
      sql@srch2 1> ALTER DATABASE demo
      		   SET CHANGE_TRACKING = ON
		   (CHANGE_RETENTION = 2 DAYS, AUTO_CLEANUP = ON)

      sql@srch2 1> use demo
      sql@srch2 2> go

      sql@srch2 1> CREATE TABLE COMPANY(ID CHAR(50) PRIMARY KEY NOT NULL,
      		    NAME CHAR(50) NOT NULL, AGE CHAR(50) NOT NULL,
		    ADDRESS CHAR(50), SALARY CHAR(50))
      sql@srch2 2> go

      sql@srch2 1> ALTER TABLE COMPANY ENABLE CHANGE_TRACKING WITH
      		   (TRACK_COLUMNS_UPDATED = ON)
      sql@srch2 2> go

6. Run the system test case.

  shell> cd srch2-ngn/test/wrapper/system_tests
  shell> vim adapter_sqlserver/conf.xml
 
  Under the "dbParameters" section, make necessary changes to
  parameters related to the SQL Server, such as data source, server, 
  user name, password, database name, and table name.

  shell> python ./adapter_sqlserver/adapter_sqlserver.py ../../../build/src/server/srch2-search-server ./adapter_sqlserver/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt

   If everything passes, we will see a "Successful" message.  Congratulations!
