/*
 * mysqlconnector.cpp
 *
 *  Created on: Sep 16, 2014
 *      Author: Chen Liu liu@srch2.com
 */

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "mysqlconnector.h"
#include "Logger.h"
#include "json/json.h"
#include "binlog_api.h"
#include "mysqlEventHandler.h"
#include "io.h"
using namespace std;
using srch2::util::Logger;

MySQLConnector::MySQLConnector() {
    serverHandle = NULL;
    listenerWaitTime = 1;
    stmt = NULL;
}

//Initialize the connector. Establish a connection to MySQL.
int MySQLConnector::init(ServerInterface *serverHandle) {
    this->serverHandle = serverHandle;
    //Get listenerWaitTime value from the config file.
    std::string listenerWaitTimeStr;
    this->serverHandle->configLookUp("listenerWaitTime", listenerWaitTimeStr);
    listenerWaitTime = static_cast<int>(strtol(listenerWaitTimeStr.c_str(),
    NULL, 10));
    if (listenerWaitTimeStr.size() == 0 || listenerWaitTime == 0) {
        listenerWaitTime = 1;
    }

    /*
     * 1. Check if the config file has all the required parameters.
     * 2. Check if the SRCH2 engine can connect to MySQL.
     *
     * If one of these checks failed, the init() fails and does not continue.
     */
    if (!checkConfigValidity() || !connectToDB()) {
        Logger::error("MYSQLCONNECTOR: exiting...");
        return -1;
    }

    string tableName;
    this->serverHandle->configLookUp("tableName", tableName);
    if(!populateFieldName(tableName)){
        Logger::error("MYSQLCONNECTOR: exiting...");
        return -1;
    }

    return 0;
}

//Connect to the MySQL database
bool MySQLConnector::connectToDB() {
    string host, port, user, password, dbName;
    this->serverHandle->configLookUp("host", host);
    this->serverHandle->configLookUp("port", port);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("password", password);
    this->serverHandle->configLookUp("dbName", dbName);

    string hostAndport = host;
    if (port.size()) {
        hostAndport.append(":").append(port);
    }
    try {
        sql::Driver * driver = get_driver_instance();
        std::auto_ptr<sql::Connection> con(
                driver->connect(hostAndport, user, password));
        stmt = con->createStatement();

        stmt->execute("USE " + dbName);

    } catch (sql::SQLException &e) {
        Logger::error(
                "MYSQLCONNECTOR: SQL error %d while connecting to the database : %s",
                e.getErrorCode(), e.getSQLState().c_str());
        return false;
    } catch (std::runtime_error &e) {
        Logger::error(
                "MYSQLCONNECTOR: Unknown SQL error while connecting to the database: %s",
                e.what());
        return false;
    }
    return true;
}

/*
 * Check if the config file has all the required parameters.
 * e.g. if it contains dbname, table etc.
 * The config file must indicate the database host, port, user, database name,
 * table name and the primary key. Otherwise, the check fails.
 */
bool MySQLConnector::checkConfigValidity() {
    string host, port, user, dbName, uniqueKey, tableName, logName;
    this->serverHandle->configLookUp("host", host);
    this->serverHandle->configLookUp("port", port);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("dbName", dbName);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    this->serverHandle->configLookUp("tableName", tableName);
    this->serverHandle->configLookUp("logName", logName);

    bool ret = (host.size() != 0) && (port.size() != 0) && (user.size() != 0)
            && (dbName.size() != 0) && (uniqueKey.size() != 0)
            && (tableName.size() != 0 && (logName.size() != 0));
    if (!ret) {
        Logger::error("MYSQLCONNECTOR: database host, port, user, "
                "database name(dbName), table name(tableName), "
                "binlog name(logName) and the primary key must be set.");
        return false;
    }

    return true;
}

/*
 * Retrieve records from the table records and insert them into the SRCH2 engine.
 * Query: SELECT * FROM table;
 */
int MySQLConnector::createNewIndexes() {
    std::string tableName;
    this->serverHandle->configLookUp("tableName", tableName);

    int indexedRecordsCount = 0;
    int totalRecordsCount = 0;

    Json::Value record;
    Json::FastWriter writer;

    try {
        sql::ResultSet * res = stmt->executeQuery("SELECT * FROM " + tableName);

        while (res->next()) {
            for (vector<string>::iterator it = fieldName.begin();
                    it != fieldName.end(); ++it) {
                record[*it] = res->getString(*it).c_str();
            }

            std::string jsonString = writer.write(record);

            totalRecordsCount++;
            if (serverHandle->insertRecord(jsonString) == 0) {
                indexedRecordsCount++;
            }

            if (indexedRecordsCount && (indexedRecordsCount % 1000) == 0)
                Logger::info("MYSQLCONNECTOR: Indexed %d records so far ...",
                        indexedRecordsCount);

        }
        Logger::info("MYSQLCONNECTOR: Total indexed %d / %d records. ",
                indexedRecordsCount, totalRecordsCount);
        //Save the time right after create new indexes.
        saveLastSavingIndexTime(time(NULL));
        this->serverHandle->saveChanges();

    } catch (sql::SQLException &e) {
        Logger::error(
                "MYSQLCONNECTOR: SQL error %d while creating new indexes : %s",
                e.getErrorCode(), e.getSQLState().c_str());
        stmt->close();/* free the object inside  */
        return -1;
    }
    stmt->close();/* free the object inside  */
    return 0;
}

//Get the table's schema and save them into a vector<schema_name>
//Query: DESCRIBE table_name;
bool MySQLConnector::populateFieldName(std::string & tableName) {
    try {
        std::auto_ptr<sql::ResultSet> res(
                stmt->executeQuery("DESCRIBE " + tableName));

        while (res->next()) {
            fieldName.push_back(res->getString("Field"));
        }
    } catch (sql::SQLException &e) {
        Logger::error(
                "MYSQLCONNECTOR: SQL error %d while populating the table schema : %s",
                e.getErrorCode(), e.getSQLState().c_str());
        return false;
    }
    return true;
}

//Load the lastSavingIndexTime from disk
bool MySQLConnector::loadLastSavingIndexTime(time_t & lastSavingIndexTime) {
    std::string dataDir, srch2Home;

    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", dataDir);
    std::string path = srch2Home + "/" + dataDir + "/mysql_data/"
            + "data.bin";

    if (checkFileExisted(path.c_str())) {
        ifstream a_file(path.c_str(), ios::in | ios::binary);
        a_file >> lastSavingIndexTime;
        a_file.close();
        return true;
    } else {
        Logger::warn("MYSQLCONNECTOR: Warning. Can not find %s."
                " The connector will use the current time.", path.c_str());
        lastSavingIndexTime = time(NULL);
        return false;
    }
}

//Save lastSavingIndexTime to disk
void MySQLConnector::saveLastSavingIndexTime(
        const time_t & lastSavingIndexTime) {
    std::string path, srch2Home;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", path);
    path = srch2Home + "/" + path + "/" + "mysql_data/";
    if (!checkDirExisted(path.c_str())) {
        // S_IRWXU : Read, write, and execute by owner.
        // S_IRWXG : Read, write, and execute by group.
        mkdir(path.c_str(), S_IRWXU | S_IRWXG);
    }

    std::string pt = path + "data.bin";
    std::ofstream a_file(pt.c_str(), std::ios::trunc | std::ios::binary);
    a_file << lastSavingIndexTime;
    a_file.flush();
    a_file.close();
}

/*
 * Periodically check updates in the MySQL log table, and send
 * corresponding requests to the SRCH2 engine
 */
int MySQLConnector::runListener() {
    string host, port, user, password, dbName, logName, pk;
    this->serverHandle->configLookUp("host", host);
    this->serverHandle->configLookUp("port", port);
    this->serverHandle->configLookUp("user", user);
    this->serverHandle->configLookUp("password", password);
    this->serverHandle->configLookUp("dbName", dbName);
    this->serverHandle->configLookUp("logName", logName);
    this->serverHandle->configLookUp("uniqueKey", pk);

    time_t lastSavingIndexTime;
    loadLastSavingIndexTime(lastSavingIndexTime);

    //Connect to the MySQL binlog by using MySQL replication listener
    stringstream url;
    url << "mysql://" << user << ":" << password << "@" << host << ":" << port;
    printf("In File: %s , Line: %d\n",__FILE__,__LINE__);
    mysql::Binary_log binlog(
            mysql::system::create_transport(url.str().c_str()));
    printf("In File: %s , Line: %d\n",__FILE__,__LINE__);
    //Register the handlers to listen the binlog event
    Incident_handler incident_hdlr;
    Table_index table_event_hdlr;
    Applier replay_hdlr(&table_event_hdlr, serverHandle, &fieldName,
            lastSavingIndexTime, pk);
    printf("In File: %s , Line: %d\n",__FILE__,__LINE__);
    binlog.content_handler_pipeline()->push_back(&table_event_hdlr);
    binlog.content_handler_pipeline()->push_back(&incident_hdlr);
    binlog.content_handler_pipeline()->push_back(&replay_hdlr);
    printf("In File: %s , Line: %d\n",__FILE__,__LINE__);
    if (binlog.connect()) {
        printf("In File: %s , Line: %d\n",__FILE__,__LINE__);
        Logger::error(
                "MYSQLCONNECTOR: Can't connect to the master MySQL server.");
        return -1;
    }
    printf("In File: %s , Line: %d\n",__FILE__,__LINE__);
    if (binlog.set_position(logName + ".000001", 4)) {
        Logger::error("MYSQLCONNECTOR: Can't reposition the binary log reader");
        return -1;
    }
    printf("In File: %s , Line: %d\n",__FILE__,__LINE__);
    bool quit = false;
    while (!quit) {
        /*
         Pull events from the master. This is the heart beat of the event listener.
         */
        Binary_log_event *event;
        binlog.wait_for_next_event(&event);

        /*
         Perform a special action based on event type
         */
        switch (event->header()->type_code) {
        case mysql::ROTATE_EVENT: {
            mysql::Rotate_event *rot = static_cast<mysql::Rotate_event *>(event);
            Logger::info(
                    "MYSQLCONNECTOR: Event type: Rotate, filename= %s pos= %d",
                    rot->binlog_file.c_str(), rot->binlog_pos);
        }
            break;
        case mysql::WRITE_ROWS_EVENT:
        case mysql::WRITE_ROWS_EVENT_V1:
        case mysql::UPDATE_ROWS_EVENT:
        case mysql::UPDATE_ROWS_EVENT_V1:
        case mysql::DELETE_ROWS_EVENT:
        case mysql::DELETE_ROWS_EVENT_V1: {
            time_t rowEventTimestamp = event->header()->timestamp;
            printf("Current ts %lu : Last ts %lu\n", rowEventTimestamp,
                    lastSavingIndexTime);
            if (rowEventTimestamp - lastSavingIndexTime >= listenerWaitTime) {
                lastSavingIndexTime = rowEventTimestamp;
                this->serverHandle->saveChanges();
            }
        }
            break;
        }
        delete event;
    }

    return -1;
}

MySQLConnector::~MySQLConnector() {

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
    return new MySQLConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}
