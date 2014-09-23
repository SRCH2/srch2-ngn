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

//For MySQL C++ Connector
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

class MySQLConnector: public DataConnector {
public:
    MySQLConnector();
    ~MySQLConnector();

    //Initialize the connector. Establish a connection to the MySQL.
    virtual int init(ServerInterface *serverHandle);
    //Retrieve records from the table records and insert them into the SRCH2 engine.
    virtual int createNewIndexes();
    //Periodically check updates in the MySQL log table,
    //and send corresponding requests to the SRCH2 engine.
    virtual int runListener();

private:
    ServerInterface * serverHandle;
    int listenerWaitTime;

    //Table's schema
    std::vector<std::string> fieldName;

    //connection handler for MySQL C++ Connector
    sql::Statement * stmt;

    //Connect to the MySQL database
    bool connectToDB();
    //Check the config validity. e.g. if contains dbname, tables etc.
    bool checkConfigValidity();

    //Fetch the table schema and store into tableSchema
    bool populateFieldName(std::string & tableName);

    //Save the lastSavingIndexTime to the disk
    void saveLastSavingIndexTime(const time_t & lastSavingIndexTime);
    //Load the lastSavingIndexTime from the disk
    bool loadLastSavingIndexTime(time_t & lastSavingIndexTime);

};

#endif /* __MYSQLCONNECTOR_H__ */
