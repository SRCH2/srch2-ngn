/*
 * sqliteconnector.cpp
 *
 *  Created on: Jul 3, 2014
 *      Author: Chen Liu at SRCH2
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
    maxRetryOnFailure = 0;
    listenerWaitTime = 0;
    selectStmt = NULL;
    deleteLogStmt = NULL;
}

//Initialize the connector. Establish a connection to Sqlite.
int SQLiteConnector::init(ServerInterface * serverHandle) {
    this->serverHandle = serverHandle;

    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    //Set the default log table name and default log attributes name.
    LOG_TABLE_NAME = "SRCH2_LOG_";
    LOG_TABLE_NAME.append(tableName.c_str());
    LOG_TABLE_NAME_DATE = LOG_TABLE_NAME + "_DATE";
    LOG_TABLE_NAME_OP = LOG_TABLE_NAME + "_OP";
    //LOG_TABLE_NAME_ID column will store the old primary id
    //in case that the record primary id changed.
    LOG_TABLE_NAME_ID = LOG_TABLE_NAME + "_ID";
    TRIGGER_INSERT_NAME = "SRCH2_INSERT_LOG_TRIGGER_";
    TRIGGER_INSERT_NAME.append(tableName.c_str());
    TRIGGER_DELETE_NAME = "SRCH2_DELETE_LOG_TRIGGER_";
    TRIGGER_DELETE_NAME.append(tableName.c_str());
    TRIGGER_UPDATE_NAME = "SRCH2_UPDATE_LOG_TRIGGER_";
    TRIGGER_UPDATE_NAME.append(tableName.c_str());

    //Get listenerWaitTime and maxRetryOnFailure value from the config file.
    std::string listenerWaitTimeStr, maxRetryOnFailureStr;
    this->serverHandle->configLookUp("listenerWaitTime", listenerWaitTimeStr);
    this->serverHandle->configLookUp("maxRetryOnFailure", maxRetryOnFailureStr);
    listenerWaitTime = atoi(listenerWaitTimeStr.c_str());
    if (listenerWaitTimeStr.size() == 0 || listenerWaitTime == 0) {
        listenerWaitTime = 1;
    }
    maxRetryOnFailure = atoi(maxRetryOnFailureStr.c_str());
    if (maxRetryOnFailureStr.size() == 0) {
        maxRetryOnFailure = 3;
    }

    /*
     * 1. Check if the config file has all the required parameters.
     * 2. Check if the SRCH2 engine can connect to Sqlite.
     * 3. Check if Sqlite contains the table that is specified in the config file
     *
     * If one of these checks failed, the init() fails and does not continue.
     */
    if (!checkConfigValidity() || !connectToDB() || !checkTableExistence()) {
        printf("SQLITECONNECTOR: exiting...\n");
        return -1;
    }

    /*
     * 1. Get the table's schema and save them into a map<schema_name, schema_type>
     * 2. Create the log table to keep track of the changes in the original table.
     * 3. Create the triggers that can be invoked when the original table has changes.
     * 4. Create the prepared statements for the select/delete queries used in the listener.
     *
     * If one of these checks failed , the init() fails and does not continue.
     */
    if (!populateTableSchema() || !createLogTableIfNotExistence()
            || !createTriggerIfNotExistence() || !createPreparedStatement()) {
        printf("SQLITECONNECTOR: exiting...\n");
        return -1;
    }

    return 0;
}

//Connect to the sqlite database
bool SQLiteConnector::connectToDB() {
    std::string db_name, db_path, srch2Home;
    this->serverHandle->configLookUp("db", db_name);
    this->serverHandle->configLookUp("dbPath", db_path);
    this->serverHandle->configLookUp("srch2Home", srch2Home);

    //Try to connect to the database.
    int rc;
    for (int i = maxRetryOnFailure; i >= 0; --i) {
        rc = sqlite3_open((srch2Home + "/" + db_path + "/" + db_name).c_str(),
                &db);
        if (rc) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            sleep(listenerWaitTime);
        } else {
            return true;
        }
    }
    return false;
}

/*
 * Check if the config file has all the required parameters.
 * e.g. if it contains dbname, table etc.
 * The config file must indicate the database path, db name, table name and
 * the primary key. Otherwise, the check fails.
 */
bool SQLiteConnector::checkConfigValidity() {
    std::string dbPath, db, uniqueKey, tableName;
    this->serverHandle->configLookUp("dbPath", dbPath);
    this->serverHandle->configLookUp("db", db);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    this->serverHandle->configLookUp("tableName", tableName);

    bool ret = (dbPath.size() != 0) && (db.size() != 0)
            && (uniqueKey.size() != 0) && (tableName.size() != 0);
    if (!ret) {
        printf("SQLITECONNECTOR: database path, db, tableName,"
                " uniquekey must be set.\n");
        return false;
    }

    return true;
}

//Check if database contains the table.
//Sqlite will return an error if the table doesn't exist.
//Query: SELECT COUNT(*) FROM table;
bool SQLiteConnector::checkTableExistence() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    /* Create SQL statement */
    char *zErrMsg = 0;
    std::stringstream sql;
    sql << "SELECT count(*) from " << tableName << ";";

    for (int i = maxRetryOnFailure; i >= 0; --i) {
        int rc = sqlite3_exec(db, sql.str().c_str(), NULL, NULL, &zErrMsg);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
            sqlite3_free(zErrMsg);
            sleep(listenerWaitTime);
        } else {
            return true;
        }
    }
    sqlite3_close(db);
    return false;
}

/*
 * Retrieve records from the table records and insert them into the SRCH2 engine.
 * Use the callback function "createIndex_callback" to process the records.
 * Each record will call "createIndex_callback" once.
 * Query: SELECT * FROM table;
 */
int SQLiteConnector::createNewIndexes() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    /* Create SQL statement */
    char *zErrMsg = 0;
    std::stringstream sql;
    sql << "SELECT * from " << tableName << ";";

    for (int i = maxRetryOnFailure; i >= 0; --i) {
        int rc = sqlite3_exec(db, sql.str().c_str(), addRecord_callback,
                (void *) this, &zErrMsg);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
            sqlite3_free(zErrMsg);
            sleep(listenerWaitTime);
        } else {
            this->serverHandle->saveChanges();
            return 0;
        }
    }
    return -1;
}

/*
 * The callback function of createIndex. Each row of the table will call
 * this function once and do the insertion by the server handler.
 */
int addRecord_callback(void *dbConnector, int argc, char **argv,
        char **azColName) {
    SQLiteConnector * sqliteConnector = (SQLiteConnector *) dbConnector;

    Json::Value record;
    Json::FastWriter writer;

    for (int i = 0; i < argc; i++) {
        record[azColName[i]] = argv[i] ? argv[i] : "NULL";
    }
    std::string jsonString = writer.write(record);
    sqliteConnector->serverHandle->insertRecord(jsonString);
    return 0;
}

/*
 * Periodically check updates in the Sqlite log table, and send
 * corresponding requests to the SRCH2 engine
 */
int SQLiteConnector::runListener() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);
    loadLastAccessedLogRecordTime();

    Json::Value record;
    Json::FastWriter writer;

    printf("SQLITECONNECTOR: waiting for updates ...\n");
    bool fatal_error = false;
    for (int retryCount = maxRetryOnFailure; retryCount >= 0; --retryCount){
        /*
         * While loop of the listener. In each iteration it binds the prepared
         * statement value, fetch the record, save the indexes and update the
         * time stamp.
         */
        while (1) {
            logRecordTimeChangedFlag = false;

            int rc = sqlite3_bind_text(selectStmt, 1,
                    lastAccessedLogRecordTime.c_str(),
                    lastAccessedLogRecordTime.size(), SQLITE_STATIC);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "SQL error %d : %s\n", rc, sqlite3_errmsg(db));
                sleep(listenerWaitTime);
                break;
            }

            /*
             * While loop of the query result. In each iteration it will
             *  handle only one record.
             */
            int ctotal = sqlite3_column_count(selectStmt);
            int res = 0;
            while (1) {
                res = sqlite3_step(selectStmt);
                if (res == SQLITE_ROW) {
                    /*
                     * Get old id, operation and time stamp of the log record.
                     * The order is same with the order when we create the log table
                     * 1 -> old id
                     * 2 -> time stamp
                     * 3 -> operation
                     */
                    std::string oldId = (char*) sqlite3_column_text(selectStmt,
                            0);
                    lastAccessedLogRecordTime = (char*) sqlite3_column_text(
                            selectStmt, 1);
                    logRecordTimeChangedFlag = true;
                    char* op = (char*) sqlite3_column_text(selectStmt, 2);

                    //For loop of the attributes of one record.
                    //Create a Json format string for each record.
                    std::map<std::string, std::string>::iterator it =
                            tableSchema.begin();
                    int i = 3;
                    for (i = 3; i < ctotal && it != tableSchema.end();
                            i++, it++) {
                        char * val = (char*) sqlite3_column_text(selectStmt, i);
                        record[it->first.c_str()] = val ? val : "NULL";
                    }

                    /*
                     * The column count of the log table should be equal to
                     * the original table column count + 3 (LOG_TABLE_NAME_ID,
                     * LOG_TABLE_NAME_DATE,LOG_TABLE_NAME_OP)
                     * This error may happen if Log table/original
                     * table schema changed
                     */
                    if (it != tableSchema.end() || i != ctotal) {
                        printf("SQLITECONNECTOR : Fatal Error. Table %s and"
                                " log table %s are not consistent!\n",
                                tableName.c_str(), LOG_TABLE_NAME.c_str());
                        fatal_error = true;
                        break;
                    }

                    /*
                     * Generate the Json format string and call the
                     * corresponding operation based on the "op" field.
                     *
                     * 'i' -> Insertion
                     * 'd' -> Deletion
                     * 'u' -> Update
                     */
                    std::string jsonString = writer.write(record);

                    if (strcmp(op, "i") == 0) {
                        serverHandle->insertRecord(jsonString);
                    } else if (strcmp(op, "d") == 0) {
                        serverHandle->deleteRecord(oldId);
                    } else if (strcmp(op, "u") == 0) {
                        serverHandle->updateRecord(oldId, jsonString);
                    }
                } else if (res == SQLITE_BUSY) {
                    //Retry if the database is busy.
                    //Wait for the next time to check the database
                    sleep(listenerWaitTime);
                } else {
                    break;
                }
            }

            //If fatal error happens, exit the listener immediately.
            if (fatal_error) {
                break;
            }

            //Retry the connection if the sql error happens.
            if (sqlite3_errcode(db) != SQLITE_DONE
                    && sqlite3_errcode(db) != SQLITE_OK) {
                fprintf(stderr, "Error code SQL error %d : %s\n", rc,
                        sqlite3_errmsg(db));
                sqlite3_reset(selectStmt);
                sleep(listenerWaitTime);
                break;
            }

            //Reset the prepared statement
            if (sqlite3_reset(selectStmt) != SQLITE_OK) {
                fprintf(stderr, "SQL error %d : %s\n", rc, sqlite3_errmsg(db));
                sleep(listenerWaitTime);
                break;
            }

            /*
             * Every record processed will change the flag to true. So the
             * connector could save the changes and update the log table.
             */
            if (logRecordTimeChangedFlag) {
                this->serverHandle->saveChanges();
                deleteProcessedLog();
                saveLastAccessedLogRecordTime();
                printf("SQLITECONNECTOR: waiting for updates ...\n");
            }
            sleep(listenerWaitTime);
        }
        if (fatal_error) {
            break;
        }
    }
    printf("SQLITECONNECTOR: exiting...\n");

    sqlite3_finalize(selectStmt);
    sqlite3_finalize(deleteLogStmt);
    sqlite3_close(db);
    return -1;
}

/*
 * Create the log table to keep track of the changes in the original table.
 * Query example: CREATE TABLE SRCH2_LOG_COMPANY(SRCH2_LOG_COMPANY_ID INT NOT NULL,
 *  SRCH2_LOG_COMPANY_DATE TIMESTAMP NOT NULL,
 *  SRCH2_LOG_COMPANY_OP CHAR(1) NOT NULL,
 *  ADDRESS CHAR(50) , AGE INT , ID INT , NAME TEXT , SALARY REAL );
 */
bool SQLiteConnector::createLogTableIfNotExistence() {
    /* Create SQL statement */
    char *zErrMsg = 0;
    std::stringstream sql;
    sql << "CREATE TABLE " << LOG_TABLE_NAME << "(" << LOG_TABLE_NAME_ID << " "
            << PRIMARY_KEY_TYPE << " NOT NULL, " << LOG_TABLE_NAME_DATE
            << " TIMESTAMP NOT NULL, " << LOG_TABLE_NAME_OP
            << " CHAR(1) NOT NULL,";

    for (std::map<std::string, std::string>::iterator it =
            this->tableSchema.begin(); it != this->tableSchema.end(); it++) {
        sql << " " << it->first << " " << it->second << " ";
        if (++it != this->tableSchema.end()) {
            sql << ",";
        }
        --it;
    }
    sql << ");";

    for (int i = maxRetryOnFailure; i >= 0; --i){
        int rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsg);
        if ((rc != SQLITE_OK)
                && (std::string(zErrMsg).find("already exists")
                        == std::string::npos)) {
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
            sqlite3_free(zErrMsg);
            sleep(listenerWaitTime);
        } else {
            return true;
        }
    }
    printf("SQLITECONNECTOR: Create log table %s failed.\n",
            LOG_TABLE_NAME.c_str());
    return false;
}

//Return LOG_TABLE_NAME_DATE
const char* SQLiteConnector::getLogTableDateAttr() {
    return this->LOG_TABLE_NAME_DATE.c_str();
}

//Return LOG_TABLE_NAME_OP
const char* SQLiteConnector::getLogTableOpAttr() {
    return this->LOG_TABLE_NAME_OP.c_str();
}

//Return LOG_TABLE_NAME_ID
const char* SQLiteConnector::getLogTableIdAttr() {
    return this->LOG_TABLE_NAME_ID.c_str();
}

//Get the table's schema and save them into a map<schema_name, schema_type>
//Query: PRAGMA table_info(table_name);
bool SQLiteConnector::populateTableSchema() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);
    /* Create SQL statement */
    char *zErrMsg = 0;
    std::stringstream sql;
    sql << "PRAGMA table_info(" << tableName << ");";

    for (int i = maxRetryOnFailure; i >= 0; --i) {
        int rc = sqlite3_exec(db, sql.str().c_str(),
                populateTableSchema_callback, (void *) this, &zErrMsg);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsg);
            sqlite3_free(zErrMsg);
            sleep(listenerWaitTime);
        } else {
            return true;
        }
    }
    printf("SQLITECONNECTOR: Populate schema of table %s failed.\n",
            tableName.c_str());
    return false;
}

/*
 * The callback function of "populateTableSchema".
 * Only looking for the name, type, ifPrimaryKey of the attributes,
 * and save these information into a map<schema_name,schema_type>.
 */
int populateTableSchema_callback(void * dbConnector, int argc, char ** argv,
        char **azColName) {
    SQLiteConnector * sqliteConnector = (SQLiteConnector *) dbConnector;
    std::string key, value;
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "name") == 0) {
            key = argv[i] ? argv[i] : "NULL";
        } else if (strcmp(azColName[i], "type") == 0) {
            value = argv[i] ? argv[i] : "NULL";
        } else if ((strcmp(azColName[i], "pk") == 0)
                && (strcmp(argv[i], "1") == 0)) {
            sqliteConnector->setPrimaryKeyType(value);
            sqliteConnector->setPrimaryKeyName(key);
        }
    }
    sqliteConnector->tableSchema[key] = value;
    return 0;
}

void SQLiteConnector::setPrimaryKeyType(const std::string& pkType) {
    this->PRIMARY_KEY_TYPE = pkType;
}

void SQLiteConnector::setPrimaryKeyName(const std::string& pkName) {
    this->PRIMARY_KEY_NAME = pkName;
}

/*
 * Create the prepared statements for the select/delete queries used in the listener.
 *
 * Select Query: SELECT * FROM log_table WHERE log_table_date > ?
 * ORDER BY log_table_date ASC;
 *
 * Delete Query: DELETE * FROM log_table WHERE log_table_date <= ?;
 */
bool SQLiteConnector::createPreparedStatement() {
    std::stringstream sql;

    //Create select prepared statement
    sql << "SELECT * from " << LOG_TABLE_NAME << " WHERE " << LOG_TABLE_NAME
            << "_DATE > ? ORDER BY " << LOG_TABLE_NAME << "_DATE ASC; ";

    int retryCount = 0;
    for (retryCount = maxRetryOnFailure; retryCount >= 0; --retryCount) {
        int rc = sqlite3_prepare_v2(db, sql.str().c_str(), -1, &selectStmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error %d : %s\n", rc, sqlite3_errmsg(db));
            sleep(listenerWaitTime);
        } else {
            break;
        }
    }

    if (retryCount == -1) {
        printf("SQLITECONNECTOR: Create listener select"
                " prepared statement failed.\n");
        return false;
    }

    //Create delete prepared statement.
    sql.str("");
    sql << "DELETE FROM " << LOG_TABLE_NAME << " WHERE " << LOG_TABLE_NAME
            << "_DATE <= ? ;";

    for (retryCount = maxRetryOnFailure; retryCount >= 0; --retryCount) {
        int rc = sqlite3_prepare_v2(db, sql.str().c_str(), -1, &deleteLogStmt,
                0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error %d : %s\n", rc, sqlite3_errmsg(db));
            sleep(listenerWaitTime);
        } else {
            break;
        }
    }

    if (retryCount == -1) {
        printf("SQLITECONNECTOR: Create delete expired log"
                " prepared statement failed.\n");
        return false;
    }

    return true;
}

/*
 * Create the triggers that can be invoked when the original table has changes.
 *
 * Insert trigger query example: CREATE TRIGGER SRCH2_INSERT_LOG_TRIGGER_COMPANY
 * AFTER INSERT ON COMPANY BEGIN INSERT INTO SRCH2_LOG_COMPANY
 * VALUES (new.ID , strftime('%s', 'now'),'i', new.ADDRESS , new.AGE ,
 *  new.ID , new.NAME , new.SALARY );END;
 *
 * Delete trigger query example: CREATE TRIGGER SRCH2_DELETE_LOG_TRIGGER_COMPANY
 * AFTER DELETE ON COMPANY BEGIN INSERT INTO SRCH2_LOG_COMPANY
 * VALUES (old.ID , strftime('%s', 'now'),'d', old.ADDRESS , old.AGE ,
 *  old.ID , old.NAME , old.SALARY );END;
 *
 *  Update trigger query example: CREATE TRIGGER SRCH2_UPDATE_LOG_TRIGGER_COMPANY
 *  AFTER UPDATE ON COMPANY BEGIN INSERT INTO SRCH2_LOG_COMPANY
 *  VALUES (old.ID , strftime('%s', 'now'),'u', new.ADDRESS , new.AGE ,
 *  new.ID , new.NAME , new.SALARY );END;
 */
bool SQLiteConnector::createTriggerIfNotExistence() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    /* Insert Trigger Create SQL statement */
    char *zErrMsgInsert = 0;
    int rc = 0;
    std::stringstream sql;

    sql << "CREATE TRIGGER " << TRIGGER_INSERT_NAME << " AFTER INSERT ON "
            << tableName << " BEGIN "
                    "INSERT INTO " << LOG_TABLE_NAME << " VALUES (new."
            << PRIMARY_KEY_NAME << " , strftime('%s', 'now'),'i',";

    for (std::map<std::string, std::string>::iterator it =
            this->tableSchema.begin(); it != this->tableSchema.end(); it++) {
        sql << " new." << it->first << " ";
        if (++it != this->tableSchema.end()) {
            sql << ",";
        }
        --it;
    }
    sql << ");END;";

    int i = 0;
    for (int i = maxRetryOnFailure; i >= 0; --i) {
        rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsgInsert);
        if ((rc != SQLITE_OK)
                && (std::string(zErrMsgInsert).find("already exists")
                        == std::string::npos)) {
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsgInsert);
            sqlite3_free(zErrMsgInsert);
            sleep(listenerWaitTime);
        } else {
            break;
        }
    }

    if (i == -1) {
        printf("SQLITECONNECTOR: Create insert trigger %s failed.\n",
                TRIGGER_INSERT_NAME.c_str());
        return false;
    }

    /* Delete Trigger Create SQL statement */
    sql.str("");
    char *zErrMsgDelete = 0;

    sql << "CREATE TRIGGER " << TRIGGER_DELETE_NAME << " AFTER DELETE ON "
            << tableName << " BEGIN "
                    "INSERT INTO " << LOG_TABLE_NAME << " VALUES (old."
            << PRIMARY_KEY_NAME << " , strftime('%s', 'now'),'d',";

    for (std::map<std::string, std::string>::iterator it =
            this->tableSchema.begin(); it != this->tableSchema.end(); it++) {
        sql << " old." << it->first << " ";
        if (++it != this->tableSchema.end()) {
            sql << ",";
        }
        --it;
    }
    sql << ");END;";

    for (int i = maxRetryOnFailure; i >= 0; --i) {
        rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsgDelete);
        if ((rc != SQLITE_OK)
                && (std::string(zErrMsgDelete).find("already exists")
                        == std::string::npos)) {
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsgDelete);
            sqlite3_free(zErrMsgDelete);
            sleep(listenerWaitTime);
        } else {
            break;
        }
    }

    if (i == -1) {
        printf("SQLITECONNECTOR: Create delete trigger %s failed.\n",
                TRIGGER_DELETE_NAME.c_str());
        return false;
    }

    /* Update Trigger Create SQL statement */
    sql.str("");
    char *zErrMsgUpdate = 0;
    sql << "CREATE TRIGGER " << TRIGGER_UPDATE_NAME << " AFTER UPDATE ON "
            << tableName << " BEGIN "
                    "INSERT INTO " << LOG_TABLE_NAME << " VALUES (old."
            << PRIMARY_KEY_NAME << " , strftime('%s', 'now'),'u',";

    for (std::map<std::string, std::string>::iterator it =
            this->tableSchema.begin(); it != this->tableSchema.end(); it++) {
        sql << " new." << it->first << " ";
        if (++it != this->tableSchema.end()) {
            sql << ",";
        }
        --it;
    }
    sql << ");END;";

    for (int i = maxRetryOnFailure; i >= 0; --i) {
        rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &zErrMsgUpdate);
        if ((rc != SQLITE_OK)
                && (std::string(zErrMsgUpdate).find("already exists")
                        == std::string::npos)) {
            fprintf(stderr, "SQL error %d : %s\n", rc, zErrMsgUpdate);
            sqlite3_free(zErrMsgUpdate);
            sleep(listenerWaitTime);
        } else {
            break;
        }
    }

    //If all 3 triggers created or existed, return true, otherwise return false.
    if (i == -1) {
        printf("SQLITECONNECTOR: Create update trigger %s failed.\n",
                TRIGGER_UPDATE_NAME.c_str());
        return false;
    }

    return true;
}

//Load the lastAccessedLogRecordTime from disk
void SQLiteConnector::loadLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);
    path = srch2Home + "/" + path + "/" + "sqlite_data/data.bin";
    if (access(path.c_str(), F_OK) == 0) {
        std::ifstream a_file(path.c_str(), std::ios::in | std::ios::binary);
        a_file >> lastAccessedLogRecordTime;
        a_file.close();
    } else {
        lastAccessedLogRecordTime = "0";
    }
}

//Save lastAccessedLogRecordTime to disk
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

//Delete the processed log from the table so that we can keep it small.
bool SQLiteConnector::deleteProcessedLog() {

    //Bind the lastAccessedLogRecordTime
    int rc = sqlite3_bind_text(deleteLogStmt, 1,
            lastAccessedLogRecordTime.c_str(), lastAccessedLogRecordTime.size(),
            SQLITE_STATIC);
    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error %d : %s\n", rc, sqlite3_errmsg(db));
        return false;
    }

    //Execute the delete query
    rc = sqlite3_step(deleteLogStmt);

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error %d : %s\n", rc, sqlite3_errmsg(db));
        sqlite3_reset(deleteLogStmt);
        return false;
    }

    //Reset the prepared statement
    rc = sqlite3_reset(deleteLogStmt);

    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error %d : %s\n", rc, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

SQLiteConnector::~SQLiteConnector() {

}

/*
 * "create_t()" and "destroy_t(DataConnector*)" is called to create/delete
 * the instance of the connector. A simple example of implementing these
 * two function is here.
 *
 * extern "C" DataConnector* create() {
 *     return new YourDBConnector;
 * }
 *
 * extern "C" void destroy(DataConnector* p) {
 *     delete p;
 * }
 *
 * These two C APIs are used by the srch2-engine to create/delete the instance
 * in the shared library.
 * The engine will call "create()" to get the connector and call
 * "destroy" to delete it.
 */
extern "C" DataConnector* create() {
    return new SQLiteConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}
