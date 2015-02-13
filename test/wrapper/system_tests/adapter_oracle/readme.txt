Set up the test environment for adapter_oracle 

1. Create test user "cdcpub" before the system test.
shell> sqlplus / as sysdba
SQL> startup

You may  get the following error: 
 ORA-01078: failure in processing system parameters
 LRM-00109: could not open parameter file '/u01/app/oracle/product/11.2.0/dbhome_1/dbs/initDB11G.ora'

If so, go to foler "/u01/app/oracle/product/11.2.0/dbhome_1/dbs/",
find the file spfileXXX.ora, then run this command:
 SQL> create pfile from spfile='/u01/app/oracle/product/11.2.0/dbhome_1/dbs/spfileXXX.ora'

2. Create a table space:

SQL> create tablespace ts_cdcpub datafile '/tmp/cdcpubdata.dbf' size 100m;
SQL> CREATE USER cdcpub IDENTIFIED BY cdcpub DEFAULT TABLESPACE ts_cdcpub
     QUOTA UNLIMITED ON SYSTEM
     QUOTA UNLIMITED ON SYSAUX;
SQL> GRANT DBA TO cdcpub;

3. Run the system test case.

   shell> cd srch2-ngn/test/wrapper/system_tests
   shell> vim adapter_oracle/conf.xml
 
   Under the "dbParameters" section, make necessary changes to
   the parameters related to Oracle, such as data source, server, 
   user name, password, and table name.

   shell> python ./adapter_oracle/adapter_oracle.py ../../../build/src/server/srch2-search-server ./adapter_oracle/testCreateIndexes_sql.txt ./adapter_oracle/testCreateIndexes.txt ./adapter_oracle/testRunListener_sql.txt ./adapter_oracle/testRunListener.txt ./adapter_oracle/testOfflineLog_sql.txt ./adapter_oracle/testOfflineLog.txt

   If everything passes, we will see a "Successful" message.  Congratulations!

4. (Optional) If you want to drop the table space created in step 2,
   run the following SQL commandds:

SQL> conn / as sysdba
SQL> DROP USER CDCPUB CASCADE;
SQL> DROP TABLESPACE ts_cdcpub INCLUDING CONTENTS AND DATAFILES;

   
