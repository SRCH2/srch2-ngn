
Compiling and running Oracle connector
Author: Chen Liu, Jan. 2015

This connector allows the SRCH2 engine to index data from Oracle db.
Currently it supports Linux (CentOS) only.

1. Install Oracle database:

 a) Install unixODBC by running the following commands:

   shell> sudo yum install unixODBC unixODBC-devel libtool-ltdl libtool-ltdl-devel
   
    More information about these instructions is available at
    http://www.asteriskdocs.org/en/3rd_Edition/asterisk-book-html-chunk/installing_configuring_odbc.html

 b) Install Oracle by following the instructions at
    http://www.tecmint.com/oracle-database-11g-release-2-installation-in-linux/

 c) Add the Oracle library path to the path to look for dynamic libraries:

   shell> sudo sh -c "echo /u01/app/oracle/product/11.2.0/dbhome_1/lib/ > /etc/ld.so.conf.d/oracle.conf"
   shell> sudo ldconfig

 d) Run the following command to get the odbcinst.ini path:

   shell> odbcinst -j

 e) Add the following lines into the file odbcinst.ini found above:

   [ORACLE]
   Description = Oracle ODBC Connection
   Driver      = /u01/app/oracle/product/11.2.0/dbhome_1/lib/libsqora.so.11.1
   Threading               = 1
   UsageCount              = 1

 f) Run the following command to check the dependency of the file
   /u01/app/oracle/product/11.2.0/dbhome_1/lib/libsqora.so.11.1

   shell> ldd /u01/app/oracle/product/11.2.0/dbhome_1/lib/libsqora.so.11.1

   If "libodbcinst.so.1" is not found, then create a soft link for it
   by using these commands:

   shell> find / -name "libodbcinst.so"
   Go to the library path
   shell> ln -s libodbcinst.so libodbcinst.so.1 

2. Install third-party libraries:

   shell> sudo yum install python-devel
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

5. Run a system test case by following the instructions in the file

    srch2-ngn/test/wrapper/system_tests/adapter_oracle/readme.txt
