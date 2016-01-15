/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * mysqlconnector.h
 *
 *  Created on: Sep 16, 2014
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
    virtual int init(ServerInterface *serverInterface);
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
    ServerInterface * serverInterface;
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
