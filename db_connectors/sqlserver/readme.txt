
Compiling and running SQL Server connector
Author: Chen Liu, Oct. 2014

1. Install SQL Server by the following instructions on http://msdn.microsoft.com/en-us/library/hh568451%28v=sql.110%29.aspx
   
 1.1) For Ubuntu (http://www.codesynthesis.com/~boris/blog/2011/12/02/microsoft-sql-server-odbc-driver-linux/):
    To install unixODBC(ODBC Driver Manager for Linux) :
      shell> sudo apt-get remove libodbc1 unixodbc unixodbc-dev
      shell> wget ftp://ftp.unixodbc.org/pub/unixODBC/unixODBC-2.3.2.tar.gz
      shell> tar -xvf unixODBC-2.3.2.tar.gz
      shell> cd unixODBC-2.3.2
    Open the configure file and search for the LIB_VERSION="2:0:0", change it to LIB_VERSION="1:0:0"   
      shell> ./configure --disable-gui --disable-drivers --enable-iconv --with-iconv-char-enc=UTF8 --with-iconv-ucode-enc=UTF16LE
      shell> make
      shell> sudo make install
    To install MS ODBC Driver 11 for SQL Server for Ubuntu, download the driver for ReadHat5 Linux on the website : http://www.microsoft.com/en-us/download/details.aspx?id=36437
      shell> tar -xvf msodbcsql*.tar.gz
      shell> cd msodbcsql*
      shell> lib64
      shell> ldd libmsodbcsql*
    Find the lines that have "not found". The next step is to install these missing dependencies. The general approach is to enter the library name in the Ubuntu Package Search(packages.ubuntu.com)
    To find the libcrypto.so.6 and libssl.so.6, download and install the package libss10.9.8.
      shell> cd /usr/lib
      shell> sudo ln -s libssl.so.0.9.8 libssl.so.6
      shell> sudo ln -s libcrypto.so.0.9.8 libcrypto.so.6
    To find the libodbc.so.1, add path /usr/loacl/lib to the end of file /etc/ld.so.conf
      shell> sudo ldconfig
    Once all the dependencies are met. Run the following command to install MS SQL Server Driver 
      shell> sudo bash ./install.sh install --force
    To test the installation,
      shell> sqlcmd -S localhost
    You should get an error message indicating a network error.

2. Install thirdparty libraries:

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

5. Enable the Change Tracking for the target Database and the Table (for eache table , this step is needed only once):

   For MS SQL Server Management Studio:
      On windows, open the MS SQL Server Management Studio.
      Select the target database, right click on the database, choose Properties->Change Tracking, enable the Change Tracking for this database.
      Selete the target table, right click on the table, choose Properties->Change Tracking, enable the Change Tracking for this table.
   For MS SQL Shell: 
      sql@admin 1> CREATE LOGIN srch2 WITH PASSWORD = 'srch2'
      sql@admin 2> go
      sql@admin 1> CREATE DATABASE demo
      sql@admin 2> go
      sql@admin 1> USE demo
      sql@admin 2> go
      sql@admin 1> CREATE USER srch2 FOR LOGIN srch2
      sql@admin 2> go
      sql@admin 1> GRANT ALTER,CONTROL TO srch2
      sql@admin 2> go

6. shell> cd srch2-ngn/test/wrapper/system_tests
   Set the SQL Server dataSource, server, database name, table name, username and password in adapter_sqlserver/conf.xml.

7. Run the system test case:

   shell> cd srch2-ngn/test/wrapper/system_tests
   shell> python ./adapter_sqlserver/adapter_sqlserver.py ../../../build/src/server/srch2-search-server ./adapter_sqlserver/testCreateIndexes_sql.txt ./adapter_sqlite/testCreateIndexes.txt ./adapter_sqlite/testRunListener_sql.txt ./adapter_sqlite/testRunListener.txt ./adapter_sqlite/testOfflineLog_sql.txt ./adapter_sqlite/testOfflineLog.txt

   If everything passes, we will see a "Successful" message.
