#include <cstring>
#include "dbconnector.h"
#include <stdlib.h>
#include <sstream>
#include <algorithm>    // std::max
#include "io.h"
#include <unistd.h>
#include "json/json.h"
#include <fstream>

using namespace std;

ExampleConnector::ExampleConnector() {
    serverHandle = NULL;
    listenerWaitTime = 1;
    lastAccessedLogRecordTS = -1;
}

int ExampleConnector::init(ServerInterface *serverHandle) {
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

    /*
     * 2. Check if the config file has all the required parameters.
     * 3. Check if the SRCH2 engine can connect to Oracle.
     *
     * If one of these checks failed, the init() fails and does not continue.
     */
    if (!checkConfigValidity() || !connectToDB()) {
        printf("ExampleConnector: exiting...\n");
        return -1;
    }

    //4. Get the schema information from the Oracle.
    string tableName;
    this->serverHandle->configLookUp("tableName", tableName);
    if (!populateTableSchema(tableName)) {
        printf("ExampleConnector: exiting...\n");
        return -1;
    }

    return 0;
}

bool ExampleConnector::connectToDB() {
    string user, password, server;
    this->serverHandle->configLookUp("server", server);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("password", password);

    /*
     * Try to connect to the database using the ODBC
     */
    return true;
}

bool ExampleConnector::checkConfigValidity() {
    string user, uniqueKey, tableName, server, changeTableName;
    this->serverHandle->configLookUp("server", server);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    this->serverHandle->configLookUp("tableName", tableName);
    this->serverHandle->configLookUp("changeTableName", changeTableName);

    bool ret = (server.size() != 0) && (user.size() != 0)
            && (uniqueKey.size() != 0) && (tableName.size() != 0)
            && (changeTableName.size() != 0);
    if (!ret) {
        printf(
                "ExampleConnector: Host Address(server), "
                        " user name(user), table name(tableName), change table name(changeTableName)"
                        " and the primary key(uniqueKey) must be set.\n");
        return false;
    }
    return true;
}

bool ExampleConnector::populateTableSchema(std::string & tableName) {
    /*
     * Connect to the database and get the table schema,
     * and save it in to the vector fieldNames for later use.
     */
    fieldNames.push_back("id");
    fieldNames.push_back("name");
    fieldNames.push_back("age");
    fieldNames.push_back("salary");
    return true;
}

int ExampleConnector::createNewIndexes() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    Json::Value record;
    Json::FastWriter writer;

    //Generate a JSON string from the record.
    vector<string>::iterator itName = fieldNames.begin();
    vector<string> sqlRecord;
    sqlRecord.push_back("1");   // ID
    sqlRecord.push_back("srch2");   // Name
    sqlRecord.push_back("10");  // Age
    sqlRecord.push_back("100000");  //Salary

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

    //Insert into SRCH2 indexes.
    if (serverHandle->insertRecord(jsonString) == 0) {
        printf("ExampleConnector: Insert successful!\n");
    }

    return 0;
}

int ExampleConnector::runListener() {
    std::string changeTableName, uniqueKey;
    this->serverHandle->configLookUp("changeTableName", changeTableName);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);

    /*
     * Load the last accessed record time stamp from the file.
     */
    loadLastAccessedLogRecordTime();

    Json::Value record;
    Json::FastWriter writer;

    /*
     * Loop for the run listener, the loop will be executed every
     * "listenerWaitTime" seconds.
     */
    while (1) {
        /*
         * Send SQL query to fetch the new changes.
         * Assume we get record:
         * {"000100", "I", "2", "NewChangeInsert", "20", "1000000"}
         * {"000101", "U", "2", "NewChangeUpdate", "30", "3000000"}
         */
        vector<string> sqlRecord;
        sqlRecord.push_back("000100");
        sqlRecord.push_back("I");
        sqlRecord.push_back("2");
        sqlRecord.push_back("NewChange");
        sqlRecord.push_back("20");
        sqlRecord.push_back("1000000");

        //Each loop will fetch one record of the query result.
        long int maxTS = -1;
        while (1) {
            //Generate a JSON string from the record.
            vector<string>::iterator itName = fieldNames.begin();
            vector<SQLCHAR *>::iterator itValue = sqlRecord.begin();

            //Get current TS
            long int currentTS = strtol(
                    reinterpret_cast<const char*>(*itValue++), NULL, 10);

            //Keep the max RSID
            maxTS = max(maxTS, currentTS);

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
            printf(
                    "ExampleConnector: Line %d change version : %d ,operation : %s, Record: %s\n",
                    __LINE__, currentTS, operation.c_str(), jsonString.c_str());

            //Make the changes to the SRCH2 indexes.
            if (operation.compare("I ") == 0) {
                serverHandle->insertRecord(jsonString);
            } else if (operation.compare("D ") == 0) {
                serverHandle->deleteRecord(pkValue);
            } else if (operation.compare("U") == 0) {
                //Old Primary key is the primary key before the record changed
                string oldPk = "dummy";
                serverHandle->updateRecord(oldPk, jsonString);
            } else {
                printf("ExampleConnector: Error while parsing the "
                        "SQL record %s with operation %s\n", jsonString.c_str(),
                        operation.c_str());
            }
        }
    }

    return -1;
}

void ExampleConnector::loadLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);

    path = srch2Home + "/" + path + "/" + "example_data/data.bin";
    if (checkFileExisted(path.c_str())) {
        std::ifstream a_file(path.c_str(), std::ios::in | std::ios::binary);
        a_file >> lastAccessedLogRecordTS;
        a_file.close();
    } else {
        //The file does not exist but the indexes already exists.
        if (lastAccessedLogRecordTS == -1) {
            printf("ExampleConnector: Can not find %s. The data may be"
                    "inconsistent. Please rebuild the indexes.\n", path.c_str());
            populateLastAccessedLogRecordTime();
        }
    }
}

void ExampleConnector::populateLastAccessedLogRecordTime() {
    string changeTableName;
    this->serverHandle->configLookUp("changeTableName", changeTableName);

    /*
     * Query the max time stamp from the change table, in our case,
     * the value we get should be 0 (The time stamp of record)
     * {"1", "srch2", "10", "100000"}
     */
    lastAccessedLogRecordTS = 0;
}

void ExampleConnector::saveLastAccessedLogRecordTime() {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);
    path = srch2Home + "/" + path + "/" + "example_data/";
    if (!checkDirExisted(path.c_str())) {
        // S_IRWXU : Read, write, and execute by owner.
        // S_IRWXG : Read, write, and execute by group.
        mkdir(path.c_str(), S_IRWXU | S_IRWXG);
    }
    std::string pt = path + "data.bin";
    std::ofstream a_file(pt.c_str(), std::ios::trunc | std::ios::binary);
    a_file << lastAccessedLogRecordTS;
    a_file.flush();
    a_file.close();
}

//Disconnect from the Oracle
ExampleConnector::~ExampleConnector() {
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
    return new ExampleConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}

