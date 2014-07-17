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

int createIndex_callback(void * dbConnector, int argc, char ** argv,
        char **azColName);
int populateTableSchema_callback(void * dbConnector, int argc, char ** argv,
        char **azColName);

class SQLiteConnector: public DataConnector {
public:
    SQLiteConnector();
    virtual ~SQLiteConnector();
    virtual bool init(ServerInterface *serverHandle);
    virtual bool runListener();
    virtual bool createNewIndexes();

    ServerInterface *serverHandle;
    std::map<std::string,std::string> tableSchema;
    //Save the time last oplog record accessed
    const char * getLogTableDateAttr();
    const char * getLogTableOpAttr();
    const char * getLogTableIdAttr();
    void setPrimaryKeyType(const std::string& pkType);
    void setPrimaryKeyName(const std::string& pkName);
    void setLastAccessedLogRecordTime(const char* t);
private:
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

    sqlite3 *db;
    bool connectToDB();
    bool checkConfigValidity();
    bool checkCollectionExistence();
    bool populateTableSchema();

    sqlite3_stmt *selectStmt;
    sqlite3_stmt *deleteLogStmt;
    bool createPreparedStatement();
	bool createTriggerIfNotExistence();
	bool createLogTableIfNotExistence();
	bool registerTrigger();

    //Load the last time last oplog record accessed
	std::string lastAccessedLogRecordTime;
	bool logRecordTimeChangedFlag;
    void loadLastAccessedLogRecordTime();
    void saveLastAccessedLogRecordTime();

    bool deleteExpiredLog();
};


#endif /* __SQLITECONNECTOR_H__ */
