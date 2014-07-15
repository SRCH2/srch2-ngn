/*
 * sqliteconnector.cpp
 *
 *  Created on: Jul 3, 2014
 *      Author: liusrch2
 */

#include "sqliteconnector.h"
#include <stdio.h>
#include <sstream>
#include "json/json.h"

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

SQLiteConnector::SQLiteConnector() {
    serverHandle = NULL;
    db = NULL;
    logRecordTimeChangedFlag = false;
}

bool SQLiteConnector::init(ServerInterface * serverHandle) {
    this->serverHandle = serverHandle;

    std::string db_collection;
    this->serverHandle->configLookUp("collection", db_collection);

    LOG_TABLE_NAME = "SRCH2_LOG_";
    LOG_TABLE_NAME.append(db_collection.c_str());
    LOG_TABLE_NAME_DATE = LOG_TABLE_NAME + "_DATE";
    LOG_TABLE_NAME_OP = LOG_TABLE_NAME + "_OP";
    TRIGGER_INSERT_NAME = "SRCH2_INSERT_LOG_TRIGGER_";
    TRIGGER_INSERT_NAME.append(db_collection.c_str());
    TRIGGER_DELETE_NAME = "SRCH2_DELETE_LOG_TRIGGER_";
    TRIGGER_DELETE_NAME.append(db_collection.c_str());
    TRIGGER_UPDATE_NAME = "SRCH2_UPDATE_LOG_TRIGGER_";
    TRIGGER_UPDATE_NAME.append(db_collection.c_str());

    if (!connectToDB()) {
        return false;
    }

    createLogTableIfNotExistence();
    createTriggerIfNotExistence();

    return true;
}

bool SQLiteConnector::connectToDB() {
    std::string db_name, db_path, srch2Home;
    this->serverHandle->configLookUp("db", db_name);
    this->serverHandle->configLookUp("dbPath", db_path);
    this->serverHandle->configLookUp("srch2Home", srch2Home);

    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open((srch2Home + "/" + db_path + "/" + db_name).c_str(), &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }
    return true;
}

void SQLiteConnector::createNewIndexes() {
    std::string collection;
    this->serverHandle->configLookUp("collection", collection);

    /* Create SQL statement */
    char *zErrMsg = 0;
    std::stringstream sql;
    sql << "SELECT * from " << collection << ";";
    int rc = sqlite3_exec(db, sql.str().c_str(), createIndex_callback, (void *)this,
            &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }

    this->serverHandle->saveChanges();

}

int createIndex_callback(void *dbConnector, int argc, char **argv,
        char **azColName) {
    SQLiteConnector * sqliteConnector = (SQLiteConnector *) dbConnector;

    Json::Value record;
    Json::FastWriter writer;

    for (int i = 0; i < argc; i++) {
        record[azColName[i]] = argv[i] ? argv[i] : "NULL";
    }
    std::string jsonString = writer.write(record);
    printf("SQLITECONNECTOR INSERTING : %s \n", jsonString.c_str());
    sqliteConnector->serverHandle->insertRecord(jsonString);
    return 0;
}

void * SQLiteConnector::runListener() {
    std::string collection;
    this->serverHandle->configLookUp("collection", collection);
    loadLastAccessedLogRecordTime();

    std::string pk, listenerWaitTimeStr;
    this->serverHandle->configLookUp("uniqueKey",pk);
    this->serverHandle->configLookUp("listenerWaitTime",listenerWaitTimeStr);
    int listenerWaitTime = atoi(listenerWaitTimeStr.c_str());
    if(listenerWaitTimeStr.size()==0){
        listenerWaitTime = 3;
    }

    std::stringstream sql;
    while (1) {
        logRecordTimeChangedFlag = false;
        /* Create SQL statement */
        char *zErrMsg = 0;
        sql.str("");
        sql << "SELECT * from " << LOG_TABLE_NAME << " LEFT OUTER JOIN "
                << collection << " ON " << LOG_TABLE_NAME << "."
                << LOG_TABLE_NAME << "_ID = " << collection << "." << pk
                << " WHERE " << LOG_TABLE_NAME << "_DATE > "
                << lastAccessedLogRecordTime << " ORDER BY " << LOG_TABLE_NAME
                << "." << LOG_TABLE_NAME << "_DATE ASC; ";
        int rc = sqlite3_exec(db, sql.str().c_str(), runListener_callback, (void *)this,
                &zErrMsg);

        if (rc != SQLITE_OK) {
            if (rc == 5) {
                //SQL error 5 : database is locked
                sleep(listenerWaitTime + 1);
            }
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
            sqlite3_free(zErrMsg);
        }

        if (logRecordTimeChangedFlag) {
            this->serverHandle->saveChanges();
            saveLastAccessedLogRecordTime();
            printf("SQLITECONNECTOR: waiting for updates ...\n");
        }
        sleep(listenerWaitTime);

    }

    return NULL;
}

int runListener_callback(void *dbConnector, int argc, char **argv, char **azColName) {
    SQLiteConnector * sqliteConnector = (SQLiteConnector *) dbConnector;
    std::string pk;
    sqliteConnector->serverHandle->configLookUp("uniqueKey",pk);

    Json::Value record;
    Json::FastWriter writer;

    int i;
    char* op;
    std::string oldId;
    for (i = 0; i < argc; i++) {

        if (strcmp(azColName[i], sqliteConnector->getLogTableDateAttr()) == 0) {
            sqliteConnector->setLastAccessedLogRecordTime(argv[i]);
        }else if (strcmp(azColName[i], pk.c_str()) == 0){
            oldId = argv[i];
        } else if (strcmp(azColName[i], sqliteConnector->getLogTableOpAttr())
                == 0) {
            op = argv[i];
        } else {
            record[azColName[i]] = argv[i] ? argv[i] : "NULL";
        }
    }
    std::string jsonString = writer.write(record);
    printf("SQLITECONNECTOR PROCESSING : %s \n", jsonString.c_str());


    if(strcmp(op,"i")==0){
        sqliteConnector->serverHandle->insertRecord(jsonString);
    }else if (strcmp(op,"d")==0){
        sqliteConnector->serverHandle->deleteRecord(oldId);
    }else if (strcmp(op,"u")==0){
        sqliteConnector->serverHandle->updateRecord(oldId,jsonString);
    }


    return 0;
}

void SQLiteConnector::createLogTableIfNotExistence() {
    /* Create SQL statement */
    char *zErrMsg = 0;
    printf("SQLITECONNECTOR: Creating log table %s if not existence.\n",
            LOG_TABLE_NAME.c_str());
    std::stringstream sql;
    sql << "CREATE TABLE " << LOG_TABLE_NAME << "("  << LOG_TABLE_NAME_DATE
            << " TIMESTAMP NOT NULL, " << LOG_TABLE_NAME_OP
            << " CHAR(1) NOT NULL,";

    for(std::map<std::string,std::string>::iterator it = this->tableSchema.begin();it!=this->tableSchema.end();it++){
        sql<<" "<<it->first<<" "<<it->second<<" ,";
    }
    sql<<'\b';
    sql<<");";

    printf("createLogTableIfNotExistence:\n %s \n",sql.str().c_str());

    int rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

const char* SQLiteConnector::getLogTableDateAttr(){
    return this->LOG_TABLE_NAME_DATE.c_str();
}

const char* SQLiteConnector::getLogTableOpAttr(){
    return this->LOG_TABLE_NAME_OP.c_str();
}

void SQLiteConnector::populateTableSchema() {
    std::string collection;
    this->serverHandle->configLookUp("collection", collection);
    /* Create SQL statement */
    char *zErrMsg = 0;
    std::stringstream sql;
    sql << "PRAGMA table_info(" << collection << ");";
    int rc = sqlite3_exec(db, sql.str().c_str(), getTableSchema_callback,
            (void *) this, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

int populateTableSchema_callback(void * dbConnector, int argc, char ** argv,
        char **azColName) {
    SQLiteConnector * sqliteConnector = (SQLiteConnector *) dbConnector;
    std::string key, value;
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "name") == 0) {
            key = argv[i] ? argv[i] : "NULL";
        } else if (strcmp(azColName[i], "type") == 0) {
            value = argv[i] ? argv[i] : "NULL";
        }
    }
    sqliteConnector->tableSchema[key] = value;
    return 0;
}

void SQLiteConnector::createTriggerIfNotExistence() {
    std::string collection;
    this->serverHandle->configLookUp("collection", collection);
    /* Create SQL statement */
    char *zErrMsgInsert = 0;
    int rc = 0;
    std::stringstream sql;

    printf(
            "SQLITECONNECTOR: Creating insert trigger %s if not existence.\n %s \n",
            TRIGGER_INSERT_NAME.c_str(), sql.str().c_str());
    sql << "CREATE TRIGGER " << TRIGGER_INSERT_NAME << " AFTER INSERT ON "
            << collection << " BEGIN "
                    "INSERT INTO " << LOG_TABLE_NAME << "(" << LOG_TABLE_NAME
            << "_ID, " << LOG_TABLE_NAME << "_DATE," << LOG_TABLE_NAME
            << "_OP) "
                    "VALUES (new.ID, strftime('%s', 'now'),'i');"
                    "END;";
    rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsgInsert);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsgInsert);
        sqlite3_free(zErrMsgInsert);
    }

    sql.str("");
    char *zErrMsgDelete = 0;
    printf(
            "SQLITECONNECTOR: Creating delete trigger %s if not existence. \n %s \n",
            TRIGGER_DELETE_NAME.c_str(), sql.str().c_str());
    sql << "CREATE TRIGGER " << TRIGGER_DELETE_NAME << " AFTER DELETE ON "
            << collection << " BEGIN "
                    "INSERT INTO " << LOG_TABLE_NAME << "(" << LOG_TABLE_NAME
            << "_ID, " << LOG_TABLE_NAME << "_DATE," << LOG_TABLE_NAME
            << "_OP) "
                    "VALUES (old.ID, strftime('%s', 'now'),'d');"
                    "END;";
    rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsgDelete);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsgDelete);
        sqlite3_free(zErrMsgDelete);
    }

    sql.str("");
    char *zErrMsgUpdate = 0;
    printf(
            "SQLITECONNECTOR: Creating update trigger %s if not existence.\n %s \n",
            TRIGGER_UPDATE_NAME.c_str(), sql.str().c_str());
    sql << "CREATE TRIGGER " << TRIGGER_UPDATE_NAME << " AFTER UPDATE ON "
            << collection << " BEGIN "
                    "INSERT INTO " << LOG_TABLE_NAME << "(" << LOG_TABLE_NAME
            << "_ID, " << LOG_TABLE_NAME << "_DATE," << LOG_TABLE_NAME
            << "_OP) "
                    "VALUES (new.ID, strftime('%s', 'now'),'u');"
                    "END;";
    rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsgUpdate);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsgUpdate);
        sqlite3_free(zErrMsgUpdate);
    }
}

//Load the last time last oplog record accessed
void SQLiteConnector::loadLastAccessedLogRecordTime() {
    //Keep the time stamp start running the listener
    std::stringstream currentTime;
    currentTime << time(NULL);

    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);
    path = srch2Home + "/" + path + "/" + "sqlite_data/data.bin";
    if (access(path.c_str(), F_OK) == 0) {
        std::ifstream a_file(path.c_str(), std::ios::in | std::ios::binary);
        a_file >> lastAccessedLogRecordTime;
        a_file.close();
    } else {
        lastAccessedLogRecordTime = currentTime.str();
    }
}

//Save the time last oplog record accessed to the variable lastAccessedLogRecordTime
void SQLiteConnector::setLastAccessedLogRecordTime(const char* t) {
    lastAccessedLogRecordTime = t;
    logRecordTimeChangedFlag = true;
}

//Save the time last oplog record accessed to the disk
void SQLiteConnector::saveLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);
    path = srch2Home + "/" + path + "/" + "sqlite_data/";
    if (access(path.c_str(), F_OK) != 0) {
        boost::filesystem::create_directories(path);
    }
    std::string pt = path + "data.bin";
    std::ofstream a_file(pt.c_str(), std::ios::trunc | std::ios::binary);
    a_file << lastAccessedLogRecordTime;
    a_file.flush();
    a_file.close();
}

SQLiteConnector::~SQLiteConnector() {

}

// the class factories
extern "C" DataConnector* create() {
    return new SQLiteConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}
