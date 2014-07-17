/*
 * sqliteconnector.h
 *
 *  Created on: Jul 3, 2014
 *      Author: liusrch2
 */

#ifndef __SQLITECONNECTOR_H__
#define __SQLITECONNECTOR_H__

#include "DataConnector.h"
#include <string>
#include <map>
#include <sqlite3.h>

//The callback function of createIndex.
int createIndex_callback(void * dbConnector, int argc, char ** argv,
        char **azColName);
//The callback function of populateTableSchema.
int populateTableSchema_callback(void * dbConnector, int argc, char ** argv,
        char **azColName);

class SQLiteConnector: public DataConnector {
public:
    SQLiteConnector();
    virtual ~SQLiteConnector();

    //Init the connector, call connect
    virtual int init(ServerInterface *serverHandle);
    //Listen to the log table and do modification to the engine
    virtual int runListener();
    //Load the table records and insert into the engine
    virtual int createNewIndexes();

    //Return LOG_TABLE_NAME_DATE
    const char * getLogTableDateAttr();
    //Return LOG_TABLE_NAME_OP
    const char * getLogTableOpAttr();
    //Return LOG_TABLE_NAME_ID
    const char * getLogTableIdAttr();

    //Set PRIMARY_KEY_TYPE
    void setPrimaryKeyType(const std::string& pkType);
    //Set PRIMARY_KEY_NAME
    void setPrimaryKeyName(const std::string& pkName);

    //Store the table schema. Key is schema name and value is schema type
    std::map<std::string,std::string> tableSchema;
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
    int maxRetryOnFailure;
    int listenerWaitTime;

    //Sqlite pointers
    sqlite3 *db;
    sqlite3_stmt *selectStmt;
    sqlite3_stmt *deleteLogStmt;

    //Timestamp and flag
    std::string lastAccessedLogRecordTime;
    bool logRecordTimeChangedFlag;

    //Connect to the sqlite database
    bool connectToDB();
    //Check the config validity. e.g. if contains dbname, collection etc.
    bool checkConfigValidity();
    //Check if database contains the table.
    bool checkCollectionExistence();
    //Fetch the table schema and store into tableSchema
    bool populateTableSchema();

    //Create prepared statements for the listener.
    bool createPreparedStatement();
    //Create triggers for the log table
	bool createTriggerIfNotExistence();
	//Create the log table
	bool createLogTableIfNotExistence();

	//Set the last time last log record accessed
	void setLastAccessedLogRecordTime(const char* t);
	//Load the last time last log record accessed from disk
    void loadLastAccessedLogRecordTime();
    //Save the last time last log record accessed from disk
    void saveLastAccessedLogRecordTime();

    //Delete the expired log, keep the log table small.
    bool deleteExpiredLog();
};


#endif /* __SQLITECONNECTOR_H__ */
