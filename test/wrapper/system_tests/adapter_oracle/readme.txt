Set up the test environment for adapter_oracle 

1. Create test user "cdcpub" before the system test.
shell> conn / as sysdba
SQL> startup

If you get error: 
ORA-01078: failure in processing system parameters
LRM-00109: could not open parameter file '/u01/app/oracle/product/11.2.0/dbhome_1/dbs/initDB11G.ora'
Go to foler "/u01/app/oracle/product/11.2.0/dbhome_1/dbs/" find the spfileXXX.ora
SQL> create pfile from spfile='/u01/app/oracle/product/11.2.0/dbhome_1/dbs/spfileXXX.ora'

SQL> create tablespace ts_cdcpub datafile '/tmp/cdcpubdata.dbf' size 100m;
SQL> CREATE USER cdcpub IDENTIFIED BY cdcpub DEFAULT TABLESPACE ts_cdcpub
     QUOTA UNLIMITED ON SYSTEM
     QUOTA UNLIMITED ON SYSAUX;
SQL> GRANT DBA TO cdcpub;

2. Run the system test case

3. Drop test user "cdcpub" if needed.
SQL> conn / as sysdba
SQL> DROP USER CDCPUB CASCADE;
SQL> DROP TABLESPACE ts_cdcpub INCLUDING CONTENTS AND DATAFILES;
