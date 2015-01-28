Set up the test environment for adapter_oracle 

1. Create test user "cdcpub" before the system test.
conn / as sysdba
startup
create tablespace ts_cdcpub datafile '/tmp/cdcpubdata.dbf' size 100m;
CREATE USER cdcpub IDENTIFIED BY cdcpub DEFAULT TABLESPACE ts_cdcpub
QUOTA UNLIMITED ON SYSTEM
QUOTA UNLIMITED ON SYSAUX;
GRANT DBA TO cdcpub;

2. Run the system test case

3. Drop test user "cdcpub" if needed.
conn / as sysdba
DROP USER CDCPUB CASCADE;
DROP TABLESPACE ts_cdcpub INCLUDING CONTENTS AND DATAFILES;
