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
     * The init() function should do the following:
     *
     * 1. Pass the ServerInterface object from the engine to the connector so
     * that the connector can use functions "insertRecord", "deleteRecord",
     * "updateRecord", and "configLookUp" to interact with the engine.
     *
     * 2. Check if the config file contains all the required parameters.
     *
     * 3. Connect to the database.
     *
     * 4. Get the schema information from the database.
     */
    virtual int init(ServerInterface *serverInterface);

    /*
     * Retrieve records from the table and insert them into the SRCH2 engine.
     * Query : SELECT * FROM [TABLE];
     */
    virtual int createNewIndexes();

    /*
     * Periodically pull the updates from the Oracle change table, and send
     * corresponding requests to the SRCH2 engine.
     * For example: table emp(id, name, age, salary).
     * "Change table" name : emp_ct
     * Query : SELECT RSID$, OPERATION$, id, name, age, salary
     *         FROM emp_ct
     *         WHERE RSID$ > ?;
     * The query is using a prepared statement where "?" is the last accessed record RSID$.
     * The connector always keeps the latest RSID$ so that the connector can skip the
     * earlier processed record.
     */
    virtual int runListener();

    /* Save the lastAccessedLogRecordRSID to a file.
     * For Oracle, we save the "RSID$" value instead of the timestamp.
     */
    virtual void saveLastAccessedLogRecordTime();

private:
    /* Config parameters */
    SQLHENV henv;
    SQLHDBC hdbc;

    int listenerWaitTime;
    int oracleMaxColumnLength;
    ServerInterface *serverInterface;

    /* Store the table schema information */
    std::vector<std::string> fieldNames;

    /*
     * The last time the connector accessed the "change table" version,
     * We are using the version instead of timestamp.
     */
    long int lastAccessedLogRecordRSID;

    /* Execute a SQL query */
    bool executeQuery(SQLHSTMT & hstmt, const std::string & query);

    void printSQLError(SQLHSTMT & hstmt);   //Log the SQL Server error msg.

    /*
     * Connect to the Oracle database by using unixODBC.
     *
     * The unixODBC requires a configuration file to locate the
     * driver .so file.  By default the configuration file is
     * /etc/odbcinst.ini, and we can use 'odbcinst -j' to locate
     * it. The "dataSource" is the driver name defined in the
     * odbcinst.ini.
     */
    bool connectToDB();

    /*
     * Check if the config file has all the required parameters,
     * The config file must include parameters such name, host address,
     * user name, table name, "change table" name, and primary
     * key. Otherwise, the check fails. 
     */
    bool checkConfigValidity();

    /* Fetch the table schema and store into tableSchema */
    bool populateTableSchema(std::string & tableName);

    /*
     * Load the last accessed record RSID$ from the file.
     * If the file does not exist, which can happen when the first
     * time we create the indexes, it will query the database and
     * fetch the latest record RSID$. 
     */
    void loadLastAccessedLogRecordTime();

    /* Get the MAX RSID$ from Oracle database.
     * Query : "SELECT MAX(RSID$) FROM changeTable";
     */
    void populateLastAccessedLogRecordTime();

    /* Allocate/Deallocate a SQL Statement handle */
    bool allocateSQLHandle(SQLHSTMT & hstmt);
    void deallocateSQLHandle(SQLHSTMT & hstmt);

    void printLog(const std::string & log, const int logLevel);
};

#endif /* #ifndef __ORACLECONNECTOR_H__ */
