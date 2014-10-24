/*
 * sqlserverconnector.cpp
 *
 *  Created on: Oct 9, 2014
 *      Author: Chen Liu liu@srch2.com
 */

#include "sqlserverconnector.h"
#include <stdlib.h>
#include "Logger.h"
#include <sstream>
#include <algorithm>    // std::max
#include "io.h"
#include <unistd.h>
#include "json/json.h"
#include <fstream>

#define SQLSERVER_DEFAULT_MAX_COLUMN_LEN (1024)

using namespace std;
using srch2::util::Logger;

SQLServerConnector::SQLServerConnector() {
    serverHandle = NULL;
    listenerWaitTime = 1;
    henv = 0;
    hdbc = 0;
    lastAccessedLogRecordChangeVersion = -1;
    sqlServerMaxColumnLength = SQLSERVER_DEFAULT_MAX_COLUMN_LEN;
}

//Initialize the connector. Establish a connection to the SQL Server.
int SQLServerConnector::init(ServerInterface *serverHandle) {
    this->serverHandle = serverHandle;
    //Get listenerWaitTime value from the config file.
    std::string listenerWaitTimeStr;
    this->serverHandle->configLookUp("listenerWaitTime", listenerWaitTimeStr);
    listenerWaitTime = static_cast<int>(strtol(listenerWaitTimeStr.c_str(),
    NULL, 10));
    if (listenerWaitTimeStr.size() == 0 || listenerWaitTime == 0) {
        listenerWaitTime = 1;
    }

    //Get SQLSERVER_MAX_RECORD_LEN from the config file.
    std::string sqlServerMaxColumnLengthStr;
    this->serverHandle->configLookUp("sqlServerMaxColumnLength",
            sqlServerMaxColumnLengthStr);
    sqlServerMaxColumnLength = static_cast<int>(strtol(
            sqlServerMaxColumnLengthStr.c_str(),
            NULL, 10));
    if (sqlServerMaxColumnLengthStr.size() == 0
            || sqlServerMaxColumnLength == 0) {
        sqlServerMaxColumnLength = SQLSERVER_DEFAULT_MAX_COLUMN_LEN;
    }
    //Add one for '\0'
    sqlServerMaxColumnLength++;

    /*
     * 1. Check if the config file has all the required parameters.
     * 2. Check if the SRCH2 engine can connect to SQL Server.
     *
     * If one of these checks failed, the init() fails and does not continue.
     */
    if (!checkConfigValidity() || !connectToDB()) {
        Logger::error("SQLSERVERCONNECTOR: exiting...");
        return -1;
    }

    //Get the schema information from the SQL Server.
    string tableName;
    this->serverHandle->configLookUp("tableName", tableName);
    if (!populateTableSchema(tableName)) {
        Logger::error("SQLSERVERCONNECTOR: exiting...");
        return -1;
    }

    return 0;
}

/*
 * Connect to the SQL Server database by using Data Source and SQL Server Driver
 * for Linux. Data Source is the name in /usr/local/etc/odbcinst.ini
 */
bool SQLServerConnector::connectToDB() {
    string dataSource, user, password, dbName, server;
    this->serverHandle->configLookUp("server", server);
    this->serverHandle->configLookUp("dataSource", dataSource);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("password", password);
    this->serverHandle->configLookUp("dbName", dbName);

    std::stringstream sql;
    std::stringstream connectionString;
    SQLRETURN retcode;
    SQLHSTMT hstmt;

    while (1) {
        // Allocate environment handle
        retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

        if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
            printSQLError(hstmt);
            Logger::error(
                    "SQLSERVERCONNECTOR: Allocate environment handle failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Set the ODBC version environment attribute
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                (SQLPOINTER*) SQL_OV_ODBC3, 0);

        if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
            printSQLError(hstmt);
            Logger::error("SQLSERVERCONNECTOR: Set the ODBC version "
                    "environment attribute failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Allocate connection handle
        retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

        if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
            printSQLError(hstmt);
            Logger::error(
                    "SQLSERVERCONNECTOR: Allocate connection handle failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Set login timeout to 5 seconds
        SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER) 5, 0);
        // Connect to data source
        connectionString << "DRIVER={" << dataSource << "};SERVER=" << server
                << ";DATABASE=" << dbName << ";UID=" << user << ";PWD="
                << password << ";";

        retcode = SQLDriverConnect(hdbc, NULL,
                (SQLCHAR*) connectionString.str().c_str(),
                connectionString.str().size(), NULL, 0, NULL,
                SQL_DRIVER_NOPROMPT);

        if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
            printSQLError(hstmt);
            Logger::error(
                    "SQLSERVERCONNECTOR: Connect to the sql server database failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Allocate statement handle
        if (!allocateSQLHandle(hstmt)) {
            sleep(listenerWaitTime);
            continue;
        }
        sql.str("");
        sql << "USE " << dbName;
        if (!executeQuery(hstmt, sql.str())) {
            sleep(listenerWaitTime);
            continue;
        }
        deallocateSQLHandle(hstmt);

        break;
    }

    return true;
}

//Log the SQL Server error msg if exists
void SQLServerConnector::printSQLError(SQLHSTMT & hstmt) {
    unsigned char szSQLSTATE[10];
    SDWORD nErr;
    unsigned char msg[SQL_MAX_MESSAGE_LENGTH + 1];
    SWORD cbmsg;

    while (SQLError(henv, hdbc, hstmt, szSQLSTATE, &nErr, msg, sizeof(msg),
            &cbmsg) == SQL_SUCCESS) {
        Logger::error(
                "SQLSERVERCONNECTOR: SQLSTATE=%s, Native error=%ld, msg='%s'\n",
                szSQLSTATE, nErr, msg);
    }
}

//execute the query
bool SQLServerConnector::executeQuery(SQLHSTMT & hstmt,
        const std::string & query) {
    SQLRETURN retcode = SQLExecDirect(hstmt, (SQLCHAR *) query.c_str(),
            static_cast<short int>(query.size()));

    if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
        Logger::error("SQLSERVERCONNECTOR: Execute query '%s' failed",
                query.c_str());
        printSQLError(hstmt);
        return false;
    }
    return true;
}

/*
 * Check if the config file has all the required parameters.
 * e.g. if it contains dbname, table etc.
 * The config file must indicate the Data Source configuration name, user, database name,
 * table name and the primary key. Otherwise, the check fails.
 */
bool SQLServerConnector::checkConfigValidity() {
    string dataSource, user, dbName, uniqueKey, tableName, server;
    this->serverHandle->configLookUp("server", server);
    this->serverHandle->configLookUp("dataSource", dataSource);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("dbName", dbName);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    this->serverHandle->configLookUp("tableName", tableName);

    bool ret = (server.size() != 0) && (dataSource.size() != 0)
            && (user.size() != 0) && (dbName.size() != 0)
            && (uniqueKey.size() != 0) && (tableName.size() != 0);
    if (!ret) {
        Logger::error(
                "SQLSERVERCONNECTOR: Server Name(server), Data Source Config Name(dataSource),"
                        " user, database name(dbName), table name(tableName), "
                        "and the primary key(uniqueKey) must be set.");
        return false;
    }

    return true;
}

//Get the table's schema and save them into a vector<schema_name>
//Query: SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS
//WHERE TABLE_NAME = [TABLE]
//ORDER BY ORDINAL_POSITION ASC
//For example: table emp(id, name, age, salary).
//The schema vector will contain {id, name, age, salary}
bool SQLServerConnector::populateTableSchema(std::string & tableName) {
    SQLCHAR * sSchema = new SQLCHAR[sqlServerMaxColumnLength];
    SQLRETURN retcode;
    SQLHSTMT hstmt;

    std::stringstream sql;
    sql << "SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
            << "WHERE TABLE_NAME = '" << tableName << "' "
            << "ORDER BY ORDINAL_POSITION ASC";

    do {
        //Clear the vector, redo the query and bind the columns
        //For swap: http://www.cplusplus.com/reference/vector/vector/clear/
        vector<string>().swap(fieldNames);
        // Allocate statement handle
        if (!allocateSQLHandle(hstmt)) {
            sleep(listenerWaitTime);
            continue;
        }
        if (!executeQuery(hstmt, sql.str())) {
            sleep(listenerWaitTime);
            continue;
        }

        //Bind columns
        retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, sSchema,
                sqlServerMaxColumnLength,
                NULL);

        //Fetch and save each row of schema. On an error, display a message and exit.
        bool sqlErrorFlag = false;
        while (1) {
            retcode = SQLFetch(hstmt);
            if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                printSQLError(hstmt);
                sleep(listenerWaitTime);
                sqlErrorFlag = true;
                break;
            } else if (retcode == SQL_SUCCESS
                    || retcode == SQL_SUCCESS_WITH_INFO)
                fieldNames.push_back(string((char *) sSchema));
            else
                break;
        }

        if (!sqlErrorFlag) {
            deallocateSQLHandle(hstmt);
            break;
        }

    } while (connectToDB());

    delete sSchema;
    return true;
}

/*
 * Retrieve records from the table records and insert them into the SRCH2 engine.
 * Query : SELECT * FROM [TABLE];
 */
int SQLServerConnector::createNewIndexes() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    int indexedRecordsCount = 0;
    int totalRecordsCount = 0;

    //Initialize the record buffer.
    vector<SQLCHAR *> sqlRecord;

    for (int i = 0; i < fieldNames.size(); i++) {
        SQLCHAR * sqlCol = new SQLCHAR[sqlServerMaxColumnLength];
        sqlRecord.push_back(sqlCol);
    }

    Json::Value record;
    Json::FastWriter writer;

    SQLRETURN retcode;
    SQLHSTMT hstmt;
    do {
        totalRecordsCount = 0;
        int colPosition = 1;

        // Allocate statement handle
        if (!allocateSQLHandle(hstmt)) {
            sleep(listenerWaitTime);
            continue;
        }
        if (!executeQuery(hstmt, string("SELECT * FROM " + tableName))) {
            sleep(listenerWaitTime);
            continue;
        }

        //Bind columns
        for (vector<SQLCHAR *>::iterator it = sqlRecord.begin();
                it != sqlRecord.end(); ++it) {
            retcode = SQLBindCol(hstmt, colPosition++, SQL_C_CHAR, *it,
                    sqlServerMaxColumnLength,
                    NULL);
        }
        bool sqlErrorFlag = false;
        while (1) {
            retcode = SQLFetch(hstmt);
            if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                printSQLError(hstmt);
                sleep(listenerWaitTime);
                sqlErrorFlag = true;
                break;
            } else if (retcode == SQL_SUCCESS
                    || retcode == SQL_SUCCESS_WITH_INFO) {
                vector<string>::iterator itName = fieldNames.begin();
                vector<SQLCHAR *>::iterator itValue = sqlRecord.begin();
                while (itName != fieldNames.end()) {
                    //Right trim the string
                    string val(reinterpret_cast<const char*>(*itValue));
                    val.erase(val.find_last_not_of(" \n\r\t") + 1);
                    record[*itName] = val;

                    itName++;
                    itValue++;
                }

                string jsonString = writer.write(record);

                totalRecordsCount++;
                if (serverHandle->insertRecord(jsonString) == 0) {
                    indexedRecordsCount++;
                }
                Logger::debug("SQLSERVERCONNECTOR: Line %d %s", __LINE__,
                        jsonString.c_str());
                if (indexedRecordsCount && (indexedRecordsCount % 1000) == 0)
                    Logger::info(
                            "SQLSERVERCONNECTOR: Indexed %d records so far ...",
                            indexedRecordsCount);
            } else
                break;
        }

        /*
         * If error exists while creating the new indexes, the connector will
         * re-try connecting to the SQL Server database.
         */

        if (!sqlErrorFlag) {
            populateLastAccessedLogRecordTime();
            deallocateSQLHandle(hstmt);
            break;
        }

    } while (connectToDB());

    Logger::info("SQLSERVERCONNECTOR: Total indexed %d / %d records. ",
            indexedRecordsCount, totalRecordsCount);

    //Deallocate the record buffer.
    for (int i = 0; i < fieldNames.size(); i++) {
        delete sqlRecord[i];
    }

    return 0;
}

/*
 * Wait for the updates from the SQL Server, and send
 * corresponding requests to the SRCH2 engine
 */
int SQLServerConnector::runListener() {
    std::string tableName, uniqueKey;
    this->serverHandle->configLookUp("tableName", tableName);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);

    loadLastAccessedLogRecordTime();

    Json::Value record;
    Json::FastWriter writer;

    std::stringstream sql;
    SQLRETURN retcode;
    SQLHSTMT hstmt;

    //Initialize the record buffer.
    vector<SQLCHAR *> sqlRecord;
    vector<SQLLEN> sqlCallBack;
    for (int i = 0; i < fieldNames.size() + 3; i++) {
        SQLCHAR * sqlCol = new SQLCHAR[sqlServerMaxColumnLength];
        sqlRecord.push_back(sqlCol);
        sqlCallBack.push_back(0);
    }

    //Loop for never return from the runListener
    do {
        // Allocate statement handle
        if (!allocateSQLHandle(hstmt)) {
            sleep(listenerWaitTime);
            continue;
        }

        /*
         * Create the prepared select statement. Because the SRCH2 engine only
         * supports atomic primary keys but not compound primary keys, here we
         * assume the primary key of the table only has one attribute.
         *
         * A example of this query is :
         * SELECT SYS_CHANGE_VERSION, SYS_CHANGE_OPERATION, CT.ID, t.ID, t.Name
         * , t.Address, t.Salary
         * FROM CHANGETABLE(CHANGES COMPANY , ?)CT left outer join COMPANY as t
         * on CT.ID = t.ID
         *
         * SYS_CHANGE_VERSION is the "timestamp" which increase automatically
         * for each transaction happens in table COMPANY.
         *
         * SYS_CHANGE_OPERATION has 3 options, 'I', 'D', 'U', which represent
         * INSERT, DELETE, and UPDATE sequentially.
         *
         * CT.[uniqueKey] is the primary key of record changed in the table COMPANY.
         *
         * The reason using LEFT OUTER JOIN is to handle the DELETE operation,
         * since the record will be removed from the table COMPANY.
         */
        sql.str("");
        sql << "SELECT SYS_CHANGE_VERSION, SYS_CHANGE_OPERATION, CT."
                << uniqueKey;

        for (vector<string>::iterator it = fieldNames.begin();
                it != fieldNames.end(); ++it) {
            sql << ", t." << *it;
        }

        sql << " FROM CHANGETABLE(CHANGES " << tableName << ", ?)CT"
                << " left outer join " << tableName << " as t " << "on CT."
                << uniqueKey << " = t." << uniqueKey;

        retcode = SQLPrepare(hstmt, (SQLCHAR*) sql.str().c_str(),
        SQL_NTS);

        //Bind the result set columns
        int colPosition = 1;
        vector<SQLLEN>::iterator itCallBack = sqlCallBack.begin();
        for (vector<SQLCHAR *>::iterator it = sqlRecord.begin();
                it != sqlRecord.end(); ++it) {
            retcode = SQLBindCol(hstmt, colPosition++,
            SQL_C_CHAR, *it, sqlServerMaxColumnLength, &(*itCallBack++));
        }
        /*
         * Loop for the run listener, the loop will be executed every
         * "listenerWaitTime" seconds.
         */
        Logger::info("SQLSERVERCONNECTOR: waiting for updates ...");
        while (1) {
            //Bind the "lastAccessedLogRecordChangeVersion"
            std::string lastAccessedLogRecordChangeVersionStr;
            std::stringstream strstream;
            strstream << lastAccessedLogRecordChangeVersion;
            strstream >> lastAccessedLogRecordChangeVersionStr;

            retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
            SQL_C_CHAR,
            SQL_CHAR, sqlServerMaxColumnLength, 0,
                    (SQLPOINTER) lastAccessedLogRecordChangeVersionStr.c_str(),
                    lastAccessedLogRecordChangeVersionStr.size(), NULL);

            //Execute the prepared statement.
            retcode = SQLExecute(hstmt);
            if ((retcode != SQL_SUCCESS)
                    && (retcode != SQL_SUCCESS_WITH_INFO)) {
                Logger::error("SQLSERVERCONNECTOR: Executing prepared "
                        "statement failed in runListener().");
                printSQLError(hstmt);
                //Re-connect to the SQL Server, and start again.
                sleep(listenerWaitTime);
                break;
            }

            bool sqlErrorFlag = false;
            long int maxChangeVersion = -1;
            while (1) {
                retcode = SQLFetch(hstmt);
                if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                    sqlErrorFlag = true;
                    Logger::error("SQLSERVERCONNECTOR: Fetching records failed"
                            " in runListener().");
                    printSQLError(hstmt);
                    sleep(listenerWaitTime);
                    break;
                } else if (retcode == SQL_SUCCESS
                        || retcode == SQL_SUCCESS_WITH_INFO) {
                    vector<string>::iterator itName = fieldNames.begin();
                    vector<SQLCHAR *>::iterator itValue = sqlRecord.begin();

                    long int changeVersion = strtol(
                            reinterpret_cast<const char*>(*itValue++), NULL,
                            10);

                    //Keep the max change version
                    maxChangeVersion = max(maxChangeVersion, changeVersion);

                    char operation = (*itValue[0]);
                    itValue++;

                    string oldPk(reinterpret_cast<const char*>(*itValue++));
                    oldPk.erase(oldPk.find_last_not_of(" \n\r\t") + 1);

                    while (itName != fieldNames.end()) {
                        //Right trim the string
                        string val(reinterpret_cast<const char*>(*itValue));
                        val.erase(val.find_last_not_of(" \n\r\t") + 1);

                        record[*itName] = val;
                        itName++;
                        itValue++;
                    }
                    string jsonString = writer.write(record);
                    Logger::debug(
                            "SQLSERVERCONNECTOR: Line %d change version : %d ,operation : %c, old pk: %s Record: %s",
                            __LINE__, changeVersion, operation, oldPk.c_str(),
                            jsonString.c_str());
                    switch (operation) {
                    case 'I': {
                        serverHandle->insertRecord(jsonString);
                    }
                        break;
                    case 'D': {
                        serverHandle->deleteRecord(oldPk);
                    }
                        break;
                    case 'U': {
                        serverHandle->updateRecord(oldPk, jsonString);
                    }
                        break;
                    default:
                        Logger::error(
                                "SQLSERVERCONNECTOR: Error while parsing the "
                                        "SQL record %s with operation %c",
                                jsonString.c_str(), operation);
                    }

                } else {
                    break;
                }
            }

            /*
             * Error happened while fetching the columns from SQL Server,
             * try re-connecting to the database.
             */
            if (sqlErrorFlag) {
                break;
            } else {
                if (maxChangeVersion != -1) {
                    Logger::info("SQLSERVERCONNECTOR: waiting for updates ...");
                    lastAccessedLogRecordChangeVersion = maxChangeVersion;
                }
            }
            SQLCloseCursor(hstmt);
            sleep(listenerWaitTime);
        }

        deallocateSQLHandle(hstmt);
    } while (connectToDB());

    //Deallocate the record buffer.
    for (int i = 0; i < sqlRecord.size(); i++) {
        delete sqlRecord[i];
    }

    return -1;
}

void SQLServerConnector::loadLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);

    path = srch2Home + "/" + path + "/" + "sqlserver_data/data.bin";
    if (checkFileExisted(path.c_str())) {
        std::ifstream a_file(path.c_str(), std::ios::in | std::ios::binary);
        a_file >> lastAccessedLogRecordChangeVersion;
        a_file.close();
    } else {
        if (lastAccessedLogRecordChangeVersion == -1) {
            Logger::error("SQLSERVERCONNECTOR: Can not find %s. The data may be"
                    "inconsistent. Please rebuild the indexes.", path.c_str());
            populateLastAccessedLogRecordTime();
        }
    }
}

//Get the largest Change Version from SQL Server.
//Query : "SELECT MAX(SYS_CHANGE_VERSION) FROM CHANGETABLE(CHANGES table", 0)CT";
void SQLServerConnector::populateLastAccessedLogRecordTime() {
    string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    std::stringstream sql;
    sql << "SELECT MAX(SYS_CHANGE_VERSION) FROM CHANGETABLE(CHANGES "
            << tableName << ", 0)CT";

    SQLHSTMT hstmt;
    SQLCHAR * changeVersion = new SQLCHAR[sqlServerMaxColumnLength];
    do {
        // Allocate statement handle
        if (!allocateSQLHandle(hstmt)) {
            sleep(listenerWaitTime);
            continue;
        }
        if (!executeQuery(hstmt, sql.str())) {
            sleep(listenerWaitTime);
            continue;
        }

        SQLLEN callBack = 0;
        SQLRETURN retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, changeVersion,
                sqlServerMaxColumnLength, &callBack);

        retcode = SQLFetch(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            lastAccessedLogRecordChangeVersion = strtol((char*) changeVersion,
            NULL, 10);
            deallocateSQLHandle(hstmt);
            break;
        } else if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
            printSQLError(hstmt);
            sleep(listenerWaitTime);
        } else {
            lastAccessedLogRecordChangeVersion = 0;
            break;
        }
    } while (connectToDB());

    delete changeVersion;
}

//Save the lastAccessedLogRecordTime to the disk
//For SQL Server, we save the SYS_CHANGE_VERSION instead.
void SQLServerConnector::saveLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);
    path = srch2Home + "/" + path + "/" + "sqlserver_data/";
    if (!checkDirExisted(path.c_str())) {
        // S_IRWXU : Read, write, and execute by owner.
        // S_IRWXG : Read, write, and execute by group.
        mkdir(path.c_str(), S_IRWXU | S_IRWXG);
    }

    std::string pt = path + "data.bin";
    std::ofstream a_file(pt.c_str(), std::ios::trunc | std::ios::binary);
    a_file << lastAccessedLogRecordChangeVersion;
    a_file.flush();
    a_file.close();
}

//Allocate a SQL Server statement handle to execute the query
bool SQLServerConnector::allocateSQLHandle(SQLHSTMT & hstmt) {
    SQLRETURN retcode;

    // Allocate statement handle
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
        printSQLError(hstmt);
        Logger::error("SQLSERVERCONNECTOR: Allocate connection handle failed.");
        return false;
    }
    return true;
}

//Deallocate a SQL Server statement handle to free the memory
void SQLServerConnector::deallocateSQLHandle(SQLHSTMT & hstmt) {
    SQLCancel(hstmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
}

//Disconnect from the SQL Server
SQLServerConnector::~SQLServerConnector() {
    SQLDisconnect(hdbc);

    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
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
    return new SQLServerConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}
