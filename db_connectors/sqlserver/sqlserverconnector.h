/*
 * sqlserverconnector.h
 *
 *  Created on: Oct 9, 2014
 *      Author: Chen Liu liu@srch2.com
 */

#ifndef __SQLSERVERCONNECTOR_H__
#define __SQLSERVERCONNECTOR_H__

#include "DataConnector.h"
#include <string>
#include <sqlext.h>
#include <sql.h>
#include <vector>

class SQLServerConnector: public DataConnector {
public:
    SQLServerConnector();
    virtual ~SQLServerConnector();

    //Initialize the connector. Establish a connection to the SQL Server.
    virtual int init(ServerInterface *serverHandle);
    //Retrieve records from the table records and insert them into the SRCH2 engine.
    virtual int createNewIndexes();
    //Periodically check updates in the SQL Server log table,
    //and send corresponding requests to the SRCH2 engine.
    virtual int runListener();

    //Save the lastAccessedLogRecordTime to the disk
    //For SQL Server, we save the SYS_CHANGE_VERSION instead.
    virtual void saveLastAccessedLogRecordTime();
private:
    //Config parameters
    SQLHENV henv;
    SQLHDBC hdbc;

    int listenerWaitTime;
    int sqlServerMaxColumnLength;
    ServerInterface *serverHandle;

    //Storing the table schema information
    std::vector<std::string> fieldNames;

    /*
     * The last time SQL Server Connector accessed the Change table version,
     * We are using the version instead of time stamp in SQL Server but
     * the idea is same.
     */
    long int lastAccessedLogRecordChangeVersion;

    bool executeQuery(SQLHSTMT & hstmt, const std::string & query);

    void printSQLError(SQLHSTMT & hstmt);   //Log the SQL Server error msg.

    //Connect to the sqlite database
    bool connectToDB();
    //Check the config validity. e.g. if contains dbname, tables etc.
    bool checkConfigValidity();
    //Fetch the table schema and store into tableSchema
    bool populateTableSchema(std::string & tableName);

    //Create prepared statements for the listener.
    bool createPreparedStatement();

    //Load the lastAccessedLogRecordTime from the disk
    void loadLastAccessedLogRecordTime();

    //Get the largest Change Version from SQL Server.
    void populateLastAccessedLogRecordTime();

    //Allocate/Deallocate a SQL Statement handle
    bool allocateSQLHandle(SQLHSTMT & hstmt);
    void deallocateSQLHandle(SQLHSTMT & hstmt);
};

#endif /* __SQLSERVERCONNECTOR_H__ */
