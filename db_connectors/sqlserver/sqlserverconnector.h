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
 * sqlserverconnector.h
 *
 *  Created on: Oct 9, 2014
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
    virtual int init(ServerInterface *serverInterface);
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
    ServerInterface *serverInterface;

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
