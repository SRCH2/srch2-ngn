/*
 * oracleconnector.cpp
 *
 *  Created on: Jan 21, 2015
 *      Author: Chen Liu liu@srch2.com
 */
#include <cstring>
#include "oracleconnector.h"
#include <stdlib.h>
#include "Logger.h"
#include <sstream>
#include <algorithm>    // std::max
#include "io.h"
#include <unistd.h>
#include "json/json.h"
#include <fstream>

#define ORACLE_DEFAULT_MAX_COLUMN_LEN (1024)

using namespace std;
using srch2::util::Logger;

OracleConnector::OracleConnector() {
    serverHandle = NULL;
    listenerWaitTime = 1;
    henv = 0;
    hdbc = 0;
    lastAccessedLogRecordRSID = -1;
    oracleMaxColumnLength = ORACLE_DEFAULT_MAX_COLUMN_LEN;
}

/*
 * Initialize the connector. Establish a connection to the Oracle Database.
 * The init() function should implement the following things :
 *
 * 1. Pass the ServerInterface handle from the engine to the connector so
 * that the connector can use function "insertRecord", "deleteRecord",
 * "updateRecord" and "configLookUp".
 *
 * 2. Check if the config file contains all the required parameters.
 *
 * 3. Connect to the database.
 *
 * 4. Get the schema information from the database.
 */
int OracleConnector::init(ServerInterface *serverHandle) {
    //1. Pass the ServerInterface handle from the engine to the connector.
    this->serverHandle = serverHandle;

    //Get listenerWaitTime value from the config file .
    std::string listenerWaitTimeStr;
    this->serverHandle->configLookUp("listenerWaitTime", listenerWaitTimeStr);
    listenerWaitTime = static_cast<int>(strtol(listenerWaitTimeStr.c_str(),
    NULL, 10));
    if (listenerWaitTimeStr.size() == 0 || listenerWaitTime == 0) {
        listenerWaitTime = 1;
    }

    //Get ORACLE_MAX_RECORD_LEN from the config file.
    std::string oracleMaxColumnLengthStr;
    this->serverHandle->configLookUp("oracleMaxColumnLength",
            oracleMaxColumnLengthStr);
    oracleMaxColumnLength = static_cast<int>(strtol(
            oracleMaxColumnLengthStr.c_str(),
            NULL, 10));
    if (oracleMaxColumnLengthStr.size() == 0 || oracleMaxColumnLength == 0) {
        oracleMaxColumnLength = ORACLE_DEFAULT_MAX_COLUMN_LEN;
    }
    //Add one for '\0'
    oracleMaxColumnLength++;

    /*
     * 2. Check if the config file has all the required parameters.
     * 3. Check if the SRCH2 engine can connect to Oracle.
     *
     * If one of these checks failed, the init() fails and does not continue.
     */
    if (!checkConfigValidity() || !connectToDB()) {
        Logger::error("ORACLECONNECTOR: exiting...");
        return -1;
    }

    //4. Get the schema information from the Oracle.
    string tableName;
    this->serverHandle->configLookUp("tableName", tableName);
    if (!populateTableSchema(tableName)) {
        Logger::error("ORACLECONNECTOR: exiting...");
        return -1;
    }

    return 0;
}

/*
 * Connect to the Oracle database by using unixODBC.
 * Data Source is the name in /etc/odbcinst.ini
 */
bool OracleConnector::connectToDB() {
    string dataSource, user, password, server;
    this->serverHandle->configLookUp("server", server);
    this->serverHandle->configLookUp("dataSource", dataSource);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("password", password);

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
                    "ORACLECONNECTOR: Allocate environment handle failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Set the ODBC version environment attribute
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
                (SQLPOINTER*) SQL_OV_ODBC3, 0);

        if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
            printSQLError(hstmt);
            Logger::error("ORACLECONNECTOR: Set the ODBC version "
                    "environment attribute failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Allocate connection handle
        retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

        if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
            printSQLError(hstmt);
            Logger::error(
                    "ORACLECONNECTOR: Allocate connection handle failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Set login timeout to 5 seconds
        SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER) 5, 0);
        // Connect to data source
        connectionString << "DRIVER={" << dataSource << "};SERVER=" << server
                << ";UID=" << user << ";PWD=" << password << ";";

        retcode = SQLDriverConnect(hdbc, NULL,
                (SQLCHAR*) connectionString.str().c_str(),
                connectionString.str().size(), NULL, 0, NULL,
                SQL_DRIVER_NOPROMPT);

        if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
            printSQLError(hstmt);
            Logger::error(
                    "ORACLECONNECTOR: Connect to the Oracle database failed.");
            sleep(listenerWaitTime);
            continue;
        }
        // Allocate statement handle
        if (!allocateSQLHandle(hstmt)) {
            sleep(listenerWaitTime);
            continue;
        }
        deallocateSQLHandle(hstmt);

        break;
    }
    return true;
}

//Log the SQL Server error msg if exists.
void OracleConnector::printSQLError(SQLHSTMT & hstmt) {
    unsigned char szSQLSTATE[10];
    SDWORD nErr;
    unsigned char msg[SQL_MAX_MESSAGE_LENGTH + 1];
    SWORD cbmsg;

    while (SQLError(henv, hdbc, hstmt, szSQLSTATE, &nErr, msg, sizeof(msg),
            &cbmsg) == SQL_SUCCESS) {
        Logger::error(
                "ORACLECONNECTOR: SQLSTATE=%s, Native error=%ld, msg='%s'\n",
                szSQLSTATE, nErr, msg);
    }
}

//Execute the query.
bool OracleConnector::executeQuery(SQLHSTMT & hstmt,
        const std::string & query) {
    SQLRETURN retcode = SQLExecDirect(hstmt, (SQLCHAR *) query.c_str(),
            static_cast<short int>(query.size()));

    if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
        Logger::error("ORACLECONNECTOR: Execute query '%s' failed",
                query.c_str());
        printSQLError(hstmt);
        return false;
    }
    return true;
}

/*
 * Check if the config file has all the required parameters.
 * e.g. if it contains dataSource, table etc.
 * The config file must indicate the Data Source configuration name, host address,
 * user, table name, change table name and the primary key. Otherwise, the check fails.
 */
bool OracleConnector::checkConfigValidity() {
    string dataSource, user, uniqueKey, tableName, server, changeTableName, ownerName;
    this->serverHandle->configLookUp("server", server);
    this->serverHandle->configLookUp("dataSource", dataSource);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    this->serverHandle->configLookUp("tableName", tableName);
    this->serverHandle->configLookUp("ownerName", ownerName);
    this->serverHandle->configLookUp("changeTableName", changeTableName);

    bool ret = (server.size() != 0) && (dataSource.size() != 0)
            && (user.size() != 0) && (uniqueKey.size() != 0)
            && (tableName.size() != 0) && (changeTableName.size() != 0);
    if (!ret) {
        Logger::error(
                "ORACLECONNECTOR: Host Address(server), Data Source Config Name(dataSource),"
                        " user name(user), table name(tableName), change table name(changeTableName),"
                        " owner name(ownerName) and the primary key(uniqueKey) must be set.");
        return false;
    }

    return true;
}

//Get the table's schema and save them into a vector<schema_name>
//Query: SELECT COLUMN_NAME FROM ALL_TAB_COLUMNS
//WHERE TABLE_NAME = [TABLE] AND OWNER = [OWNERNAME];
//For example: table emp(id, name, age, salary).
//The schema vector will contain {id, name, age, salary}
bool OracleConnector::populateTableSchema(std::string & tableName) {
    SQLCHAR * sSchema = new SQLCHAR[oracleMaxColumnLength];
    SQLRETURN retcode;
    SQLHSTMT hstmt;

    std::string ownerName;
    this->serverHandle->configLookUp("ownerName",
            ownerName);

    std::stringstream sql;
    sql << "SELECT COLUMN_NAME FROM ALL_TAB_COLUMNS WHERE TABLE_NAME = UPPER('"
            << tableName << "') AND OWNER = UPPER('" + ownerName + "');";

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
                oracleMaxColumnLength,
                NULL);

        //Fetch and save each row of schema. On an error, display a message and exit.
        bool sqlErrorFlag = false;
        int i = 0;
        while (1) {
            retcode = SQLFetch(hstmt);
            if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                printSQLError(hstmt);
                sleep(listenerWaitTime);
                sqlErrorFlag = true;
                break;
            } else if (retcode == SQL_SUCCESS
                    || retcode == SQL_SUCCESS_WITH_INFO) {
                fieldNames.push_back(string((char *) sSchema));
            } else
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
int OracleConnector::createNewIndexes() {
    std::string tableName, ownerName;
    this->serverHandle->configLookUp("tableName", tableName);
    this->serverHandle->configLookUp("ownerName",
            ownerName);

    int indexedRecordsCount = 0;
    int totalRecordsCount = 0;

    //Initialize the record buffer.
    vector<SQLCHAR *> sqlRecord;
    for (int i = 0; i < fieldNames.size(); i++) {
        SQLCHAR * sqlCol = new SQLCHAR[oracleMaxColumnLength];
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
        if (!executeQuery(hstmt, string("SELECT * FROM " + ownerName + "." + tableName))) {
            sleep(listenerWaitTime);
            continue;
        }

        //Bind columns
        for (vector<SQLCHAR *>::iterator it = sqlRecord.begin();
                it != sqlRecord.end(); ++it) {
            retcode = SQLBindCol(hstmt, colPosition++, SQL_C_CHAR, *it,
                    oracleMaxColumnLength,
                    NULL);
        }

        //Each loop will fetch one record.
        bool sqlErrorFlag = false;
        while (1) {
            retcode = SQLFetch(hstmt);  //Fetch one record
            if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                printSQLError(hstmt);
                sleep(listenerWaitTime);
                sqlErrorFlag = true;
                break;
            } else if (retcode == SQL_SUCCESS
                    || retcode == SQL_SUCCESS_WITH_INFO) {
                //Generate a JSON string from the record.
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
                Logger::debug("ORACLECONNECTOR: Line %d %s", __LINE__,
                        jsonString.c_str());
                if (indexedRecordsCount && (indexedRecordsCount % 1000) == 0)
                    Logger::info(
                            "ORACLECONNECTOR: Indexed %d records so far ...",
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

    Logger::info("ORACLECONNECTOR: Total indexed %d / %d records. ",
            indexedRecordsCount, totalRecordsCount);

    //Deallocate the record buffer.
    for (int i = 0; i < fieldNames.size(); i++) {
        delete sqlRecord[i];
    }

    return 0;
}

/*
 * Periodically Pull the updates from the Oracle change table, and send
 * corresponding requests to the SRCH2 engine.
 * For example: table emp(id, name, age, salary).
 * Change table name : emp_ct
 * Query : SELECT RSID$, OPERATION$, id, name, age, salary
 *         FROM emp_ct
 *         WHERE RSID$ > ?;
 * Query is using prepared statement where "?" is the last accessed record RSID$.
 * The connector always keeps the latest RSID$ so that the connector can skip the
 * processed record.
 */
int OracleConnector::runListener() {
    std::string changeTableName, uniqueKey, ownerName;
    this->serverHandle->configLookUp("changeTableName", changeTableName);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    this->serverHandle->configLookUp("ownerName", ownerName);

    /*
     * Load the last accessed record RSID$ from the file.
     */
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
        SQLCHAR * sqlCol = new SQLCHAR[oracleMaxColumnLength];
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
         * For example: table emp(id, name, age, salary).
         * Change table name : emp_ct
         * Query : SELECT RSID$, OPERATION$, id, name, age, salary
         *         FROM emp_ct
         *         WHERE RSID$ > ?;
         *
         * RSID$ is the "timestamp" which increases automatically
         * for each transaction that happens in table emp.
         *
         * OPERATION$ has 4 options, "I ", "D ", "UO"(or "UU"), and "UN" which represent
         * INSERT, DELETE, UPDATE Old Value, UPDATE New Value respectively.
         *
         */
        sql.str("");
        sql << "SELECT RSID$, OPERATION$";

        for (vector<string>::iterator it = fieldNames.begin();
                it != fieldNames.end(); ++it) {
            sql << ", " << *it;
        }

        sql << " FROM " << ownerName << "." << changeTableName
                << " WHERE RSID$ > ? ORDER BY RSID$ ASC;";

        retcode = SQLPrepare(hstmt, (SQLCHAR*) sql.str().c_str(),
        SQL_NTS);

        //Bind the result set columns
        int colPosition = 1;
        vector<SQLLEN>::iterator itCallBack = sqlCallBack.begin();
        for (vector<SQLCHAR *>::iterator it = sqlRecord.begin();
                it != sqlRecord.end(); ++it) {
            retcode = SQLBindCol(hstmt, colPosition++,
            SQL_C_CHAR, *it, oracleMaxColumnLength, &(*itCallBack++));
        }

        /*
         * Loop for the run listener, the loop will be executed every
         * "listenerWaitTime" seconds.
         */
        Logger::info("ORACLECONNECTOR: waiting for updates ...");
        while (1) {
            //Bind the "lastAccessedLogRecordRSID"
            std::string lastAccessedLogRecordRSIDStr;
            std::stringstream strstream;
            strstream << lastAccessedLogRecordRSID;
            strstream >> lastAccessedLogRecordRSIDStr;

            retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT,
            SQL_C_CHAR,
            SQL_CHAR, oracleMaxColumnLength, 0,
                    (SQLPOINTER) lastAccessedLogRecordRSIDStr.c_str(),
                    lastAccessedLogRecordRSIDStr.size(), NULL);

            //Execute the prepared statement.
            retcode = SQLExecute(hstmt);
            if ((retcode != SQL_SUCCESS)
                    && (retcode != SQL_SUCCESS_WITH_INFO)) {
                Logger::error("ORACLECONNECTOR: Executing prepared "
                        "statement failed in runListener().");
                printSQLError(hstmt);
                //Re-connect to the SQL Server, and start again.
                sleep(listenerWaitTime);
                break;
            }

            bool sqlErrorFlag = false;
            long int maxRSID = -1;
            string oldPk;
            //Each loop will fetch one record.
            while (1) {
                retcode = SQLFetch(hstmt);
                if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
                    sqlErrorFlag = true;
                    Logger::error("ORACLECONNECTOR: Fetching records failed"
                            " in runListener().");
                    printSQLError(hstmt);
                    sleep(listenerWaitTime);
                    break;
                } else if (retcode == SQL_SUCCESS
                        || retcode == SQL_SUCCESS_WITH_INFO) {
                    //Generate a JSON string from the record.
                    vector<string>::iterator itName = fieldNames.begin();
                    vector<SQLCHAR *>::iterator itValue = sqlRecord.begin();

                    //Get current RSID
                    long int RSID = strtol(
                            reinterpret_cast<const char*>(*itValue++), NULL,
                            10);

                    //Keep the max RSID
                    maxRSID = max(maxRSID, RSID);

                    //Get Operation
                    string operation(reinterpret_cast<const char*>(*itValue));
                    itValue++;

                    string pkValue; //Primary key
                    while (itName != fieldNames.end()) {
                        //Right trim the string
                        string val(reinterpret_cast<const char*>(*itValue));
                        val.erase(val.find_last_not_of(" \n\r\t") + 1);

                        record[*itName] = val;
                        if ((*itName).compare(uniqueKey) == 0) {
                            pkValue = val;
                        }
                        itName++;
                        itValue++;
                    }
                    string jsonString = writer.write(record);
                    Logger::debug(
                            "ORACLECONNECTOR: Line %d change version : %d ,operation : %s, Record: %s",
                            __LINE__, RSID, operation.c_str(),
                            jsonString.c_str());

                    //Make the changes to the SRCH2 indexes.
                    if (operation.compare("I ") == 0) {
                        serverHandle->insertRecord(jsonString);
                    } else if (operation.compare("D ") == 0) {
                        serverHandle->deleteRecord(pkValue);
                    } else if (operation.compare("UU") == 0
                            || operation.compare("UO") == 0) {
                        oldPk = pkValue;    //UU/UO is old value,
                                            //we only care about the primary key of old record.
                    } else if (operation.compare("UN") == 0) {
                        if (oldPk.size() != 0) {
                            serverHandle->updateRecord(oldPk, jsonString);
                            oldPk.clear();
                        } else {
                            Logger::error(
                                    "ORACLECONNECTOR: OPERATION$ in change "
                                            "table only contains UN but no UO/UU,Update failed.");
                        }
                    } else {
                        Logger::error(
                                "ORACLECONNECTOR: Error while parsing the "
                                        "SQL record %s with operation %s",
                                jsonString.c_str(), operation.c_str());
                    }
                } else {
                    break;
                }
            }

            /*
             * Error happened while fetching the columns from Oracle,
             * try re-connecting to the database.
             */
            if (sqlErrorFlag) {
                break;
            } else {
                if (maxRSID != -1) {
                    Logger::info("ORACLECONNECTOR: waiting for updates ...");
                    lastAccessedLogRecordRSID = maxRSID;
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

/*
 * Load the last accessed record RSID$ from the file.
 * If the file does not exist(The first time create the indexes),
 * it will query the database and fetch the latest record RSID$.
 */
void OracleConnector::loadLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);

    path = srch2Home + "/" + path + "/" + "oracle_data/data.bin";
    if (checkFileExisted(path.c_str())) {
        std::ifstream a_file(path.c_str(), std::ios::in | std::ios::binary);
        a_file >> lastAccessedLogRecordRSID;
        a_file.close();
    } else {
        //The file does not exist but the indexes already exists.
        if (lastAccessedLogRecordRSID == -1) {
            Logger::error("ORACLECONNECTOR: Can not find %s. The data may be"
                    "inconsistent. Please rebuild the indexes.", path.c_str());
            populateLastAccessedLogRecordTime();
        }
    }
}

//Get the MAX RSID$ from Oracle database.
//Query : "SELECT MAX(RSID$) FROM changeTable";
void OracleConnector::populateLastAccessedLogRecordTime() {
    string changeTableName, ownerName;
    this->serverHandle->configLookUp("changeTableName", changeTableName);
    this->serverHandle->configLookUp("ownerName", ownerName);

    std::stringstream sql;
    sql << "SELECT MAX(RSID$) FROM " << ownerName << "." << changeTableName << ";";
    SQLHSTMT hstmt;
    SQLCHAR * changeVersion = new SQLCHAR[oracleMaxColumnLength];
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
                oracleMaxColumnLength, &callBack);
        retcode = SQLFetch(hstmt);
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            lastAccessedLogRecordRSID = strtol((char*) changeVersion,
            NULL, 10);
            deallocateSQLHandle(hstmt);
            break;
        } else if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) {
            printSQLError(hstmt);
            sleep(listenerWaitTime);
        } else {
            lastAccessedLogRecordRSID = 0;
            break;
        }
    } while (connectToDB());

    delete changeVersion;
}

//Save the lastAccessedLogRecordRSID to the file
//For Oracle, we save the RSID instead of time stamp.
void OracleConnector::saveLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);
    path = srch2Home + "/" + path + "/" + "oracle_data/";
    if (!checkDirExisted(path.c_str())) {
        // S_IRWXU : Read, write, and execute by owner.
        // S_IRWXG : Read, write, and execute by group.
        mkdir(path.c_str(), S_IRWXU | S_IRWXG);
    }
    std::string pt = path + "data.bin";
    std::ofstream a_file(pt.c_str(), std::ios::trunc | std::ios::binary);
    a_file << lastAccessedLogRecordRSID;
    a_file.flush();
    a_file.close();
}

//Allocate a SQL Server statement handle to execute the query
bool OracleConnector::allocateSQLHandle(SQLHSTMT & hstmt) {
    SQLRETURN retcode;

    // Allocate statement handle
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

    if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
        printSQLError(hstmt);
        Logger::error("ORACLECONNECTOR: Allocate connection handle failed.");
        return false;
    }
    return true;
}

//Deallocate a Oracle statement handle to free the memory
void OracleConnector::deallocateSQLHandle(SQLHSTMT & hstmt) {
    SQLCancel(hstmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
}

//Disconnect from the Oracle
OracleConnector::~OracleConnector() {
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
    return new OracleConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}

