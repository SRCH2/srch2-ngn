/*
 * oracleconnector.h
 *
 *  Created on: Jan 21, 2015
 *      Author: Chen Liu liu@srch2.com
 */

#ifndef __ORACLECONNECTOR_H__
#define __ORACLECONNECTOR_H__

#include "DataConnector.h"
#include <string>
#include <sqlext.h>
#include <sql.h>
#include <vector>

class OracleConnector: public DataConnector {
public:
    OracleConnector();
    virtual ~OracleConnector();

    /*
     * Initialize the connector. Establish a connection to the Oracle Database.
     * The init() function should implement the following things :
     *
     * 1. Pass the ServerInterface handle from the engine to the connector so
     * that the connector can use function "insertRecord", "deleteRecord",
     * "updateRecord" and "configLookUp".
     *
     * 2. Check if the config file contains all the required parameters.
     *
     * 3. Connect to the database.
     *
     * 4. Get the schema information from the database.
     */
    virtual int init(ServerInterface *serverHandle);

    /*
     * Retrieve records from the table records and insert them into the SRCH2 engine.
     * Query : SELECT * FROM [TABLE];
     */
    virtual int createNewIndexes();

    /*
     * Periodically Pull the updates from the Oracle change table, and send
     * corresponding requests to the SRCH2 engine.
     * For example: table emp(id, name, age, salary).
     * Change table name : emp_ct
     * Query : SELECT RSID$, OPERATION$, id, name, age, salary
     *         FROM emp_ct
     *         WHERE RSID$ > ?;
     * Query is using prepared statement where "?" is the last accessed record RSID$.
     * The connector always keeps the latest RSID$ so that the connector can skip the
     * processed record.
     */
    virtual int runListener();

    //Save the lastAccessedLogRecordRSID to the file
    //For Oracle, we save the RSID instead of time stamp.
    virtual void saveLastAccessedLogRecordTime();
private:
    //Config parameters
    SQLHENV henv;
    SQLHDBC hdbc;

    int listenerWaitTime;
    int oracleMaxColumnLength;
    ServerInterface *serverHandle;

    //Storing the table schema information
    std::vector<std::string> fieldNames;

    /*
     * The last time SQL Server Connector accessed the Change table version,
     * We are using the version instead of time stamp in SQL Server but
     * the idea is same.
     */
    long int lastAccessedLogRecordRSID;

    //Execute the query.
    bool executeQuery(SQLHSTMT & hstmt, const std::string & query);

    void printSQLError(SQLHSTMT & hstmt);   //Log the SQL Server error msg.

    /*
     * Connect to the Oracle database by using unixODBC.
     * Data Source is the name in /etc/odbcinst.ini
     */
    bool connectToDB();

    /*
     * Check if the config file has all the required parameters.
     * e.g. if it contains dataSource, table etc.
     * The config file must indicate the Data Source configuration name, host address,
     * user, table name, change table name and the primary key. Otherwise, the check fails.
     */
    bool checkConfigValidity();

    //Fetch the table schema and store into tableSchema
    bool populateTableSchema(std::string & tableName);

    /*
     * Load the last accessed record RSID$ from the file.
     * If the file does not exist(The first time create the indexes),
     * it will query the database and fetch the latest record RSID$.
     */
    void loadLastAccessedLogRecordTime();

    //Get the MAX RSID$ from Oracle database.
    //Query : "SELECT MAX(RSID$) FROM changeTable";
    void populateLastAccessedLogRecordTime();

    //Allocate/Deallocate a SQL Statement handle
    bool allocateSQLHandle(SQLHSTMT & hstmt);
    void deallocateSQLHandle(SQLHSTMT & hstmt);
};

#endif /* ORACLECONNECTOR_H_ */
