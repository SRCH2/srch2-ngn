/*
 * mysqlconnector.h
 *
 *  Created on: Sep 16, 2014
 *      Author: Chen Liu liu@srch2.com
 */

#ifndef __MYSQLCONNECTOR_H__
#define __MYSQLCONNECTOR_H__

#include "DataConnector.h"
#include <string>
#include <vector>

//For the MySQL C++ Connector
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

class MySQLConnector: public DataConnector {
public:
    MySQLConnector();
    ~MySQLConnector();

    //Initialize the connector. Establish a connection to the MySQL database.
    virtual int init(ServerInterface *serverHandle);
    //Retrieve records from the table and insert them into the SRCH2 engine.
    virtual int createNewIndexes();
    /*
     * Wait for the updates from the MySQL replication listener, and send
     * corresponding requests to the SRCH2 engine
     */
    virtual int runListener();

    //Save the lastAccessedLogRecordTime to the disk
    virtual void saveLastAccessedLogRecordTime();

private:
    ServerInterface * serverHandle;
    int listenerWaitTime;
    time_t lastAccessedLogRecordTime;
    unsigned nextPosition;
    std::string currentLogFile;

    //Storing the table schema information
    std::vector<std::string> fieldNames;

    //Connection handler for the MySQL C++ Connector
    sql::Statement * stmt;

    //Connect to the MySQL database
    bool connectToDB();
    //Check the config validity. e.g. if contains dbname, tables etc.
    bool checkConfigValidity();

    //Fetch the table schema and store into tableSchema
    bool populateFieldName(std::string & tableName);

    //Get the first log file name.
    bool getFirstLogFileName(std::string & logFileName);

    //Load the lastSavingIndexTime from the disk
    bool loadLastAccessedLogRecordTime();

};

#endif /* __MYSQLCONNECTOR_H__ */
