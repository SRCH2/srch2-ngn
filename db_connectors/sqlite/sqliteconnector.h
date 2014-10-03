/*
 * sqliteconnector.h
 *
 *  Created on: Jul 3, 2014
 *      Author: Chen Liu at SRCH2
 */

#ifndef __SQLITECONNECTOR_H__
#define __SQLITECONNECTOR_H__

#include "DataConnector.h"
#include <string>
#include <map>
#include <sqlite3.h>

//The callback function of createIndex.
int addRecord_callback(void * dbConnector, int argc, char ** argv,
        char **azColName);
//The callback function of populateTableSchema.
int populateTableSchema_callback(void * dbConnector, int argc, char ** argv,
        char **azColName);

class SQLiteConnector: public DataConnector {
public:
    SQLiteConnector();
    virtual ~SQLiteConnector();

    //Initialize the connector. Establish a connection to the Sqlite.
    virtual int init(ServerInterface *serverHandle);
    //Retrieve records from the table records and insert them into the SRCH2 engine.
    virtual int createNewIndexes();
    //Periodically check updates in the Sqlite log table,
    //and send corresponding requests to the SRCH2 engine.
    virtual int runListener();

    //Save the lastAccessedLogRecordTime to the disk
    virtual void saveLastAccessedLogRecordTime();

    //Return LOG_TABLE_NAME_DATE
    const char * getLogTableDateAttr();
    //Return LOG_TABLE_NAME_OP
    const char * getLogTableOpAttr();
    //Return LOG_TABLE_NAME_ID
    const char * getLogTableIdAttr();

    void setPrimaryKeyType(const std::string& pkType);
    void setPrimaryKeyName(const std::string& pkName);

    //Store the table schema. Key is schema name and value is schema type
    std::map<std::string, std::string> tableSchema;
    ServerInterface *serverHandle;
private:
    //Config parameters
    std::string LOG_TABLE_NAME;
    std::string TRIGGER_INSERT_NAME;
    std::string TRIGGER_DELETE_NAME;
    std::string TRIGGER_UPDATE_NAME;
    std::string LOG_TABLE_NAME_DATE;
    std::string LOG_TABLE_NAME_OP;
    std::string LOG_TABLE_NAME_ID;
    std::string PRIMARY_KEY_TYPE;
    std::string PRIMARY_KEY_NAME;
    int listenerWaitTime;

    /* For the sqlite, the connector is using this timestamp on the
     * prepared statement, so we do not need to convert it to type time_t
     * in the runListener() for each loop. To be consistent with other connectors,
     * we use "lastAccessedLogRecordTimeStr" instead of "lastAccessedLogRecordTime"
     * in this connector.
     */
    std::string lastAccessedLogRecordTimeStr;

    //Parameters for Sqlite
    sqlite3 *db;
    sqlite3_stmt *selectStmt;
    sqlite3_stmt *deleteLogStmt;

    //The flag is true if there are new records in the log table.
    bool logRecordTimeChangedFlag;

    //Connect to the sqlite database
    bool connectToDB();
    //Check the config validity. e.g. if contains dbname, tables etc.
    bool checkConfigValidity();
    //Check if database contains the table.
    bool checkTableExistence();
    //Fetch the table schema and store into tableSchema
    bool populateTableSchema();

    //Create prepared statements for the listener.
    bool createPreparedStatement();
    //Create triggers for the log table
    bool createTriggerIfNotExistence();
    //Create the log table
    bool createLogTableIfNotExistence();

    //Load the lastAccessedLogRecordTime from the disk
    void loadLastAccessedLogRecordTime();

    //Delete the processed log from the table so that we can keep it small
    bool deleteProcessedLog();
};

#endif /* __SQLITECONNECTOR_H__ */
