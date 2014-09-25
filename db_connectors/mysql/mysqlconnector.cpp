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

//Initialize the connector. Establish a connection to the MySQL database.
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

    //Get the schema information from the MySQL.
    string tableName;
    this->serverHandle->configLookUp("tableName", tableName);
    if (!populateFieldName(tableName)) {
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

    while (1) {
        try {
            //Create the connection to the MySQL by using the MySQL C++ Connector.
            sql::Driver * driver = get_driver_instance();
            std::auto_ptr<sql::Connection> con(
                    driver->connect(hostAndport, user, password));
            stmt = con->createStatement();

            //Select the target database.
            stmt->execute("USE " + dbName);

            return true;
        } catch (sql::SQLException &e) {
            Logger::error(
                    "MYSQLCONNECTOR: SQL error %d while connecting to the database : %s %s",
                    e.getErrorCode(), e.getSQLState().c_str(), e.what());
            if (stmt != NULL) {
                stmt->close();
            }
            sleep(listenerWaitTime);
        } catch (std::runtime_error &e) {
            Logger::error(
                    "MYSQLCONNECTOR: Unknown SQL error while connecting to the database: %s",
                    e.what());
            if (stmt != NULL) {
                stmt->close();
            }
            sleep(listenerWaitTime);
        }
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

    while (1) {
        try {
            sql::ResultSet * res = stmt->executeQuery(
                    "SELECT * FROM " + tableName);

            //Iterate all the selected records.
            while (res->next()) {
                //Iterate the fields of one record.
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
                    Logger::info(
                            "MYSQLCONNECTOR: Indexed %d records so far ...",
                            indexedRecordsCount);

            }
            Logger::info("MYSQLCONNECTOR: Total indexed %d / %d records. ",
                    indexedRecordsCount, totalRecordsCount);
            //Save the time right after creating the new indexes.
            saveLastSavingIndexTime(time(NULL));
            this->serverHandle->saveChanges();

            stmt->close();/* free the object inside  */
            return 0;
        } catch (sql::SQLException &e) {
            Logger::error(
                    "MYSQLCONNECTOR: SQL error %d while creating new indexes : %s",
                    e.getErrorCode(), e.getSQLState().c_str());
            sleep(listenerWaitTime);
        }
    }

    stmt->close();/* free the object inside  */
    return 0;
}

//Get the table's schema and save them into a vector<schema_name>
//Query: DESCRIBE table_name;
bool MySQLConnector::populateFieldName(std::string & tableName) {
    while (1) {
        try {
            std::auto_ptr<sql::ResultSet> res(
                    stmt->executeQuery("DESCRIBE " + tableName));

            while (res->next()) {
                fieldName.push_back(res->getString("Field"));
            }
            return true;
        } catch (sql::SQLException &e) {
            Logger::error(
                    "MYSQLCONNECTOR: SQL error %d while populating the table schema : %s",
                    e.getErrorCode(), e.getSQLState().c_str());
            sleep(listenerWaitTime);
        }
    }
    return true;
}

//Load the lastSavingIndexTime from the disk
bool MySQLConnector::loadLastSavingIndexTime(time_t & lastSavingIndexTime) {
    std::string dataDir, srch2Home;

    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", dataDir);
    std::string path = srch2Home + "/" + dataDir + "/mysql_data/" + "data.bin";

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

//Save lastSavingIndexTime to the disk
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
 * Wait for the updates from the MySQL replication listener, and send
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
    mysql::Binary_log binlog(
            mysql::system::create_transport(url.str().c_str()));

    //Register the handlers to listen the binlog event
    Incident_handler incident_hdlr;
    Table_index table_event_hdlr;
    Applier replay_hdlr(&table_event_hdlr, serverHandle, &fieldName,
            lastSavingIndexTime, pk);

    binlog.content_handler_pipeline()->push_back(&table_event_hdlr);
    binlog.content_handler_pipeline()->push_back(&incident_hdlr);
    binlog.content_handler_pipeline()->push_back(&replay_hdlr);

    while (binlog.connect()) {
        Logger::error(
                "MYSQLCONNECTOR: Can't connect to the master MySQL server. Please Check if the binlog mode is enabled.");
        sleep(listenerWaitTime);
    }

    //Initialize the binlog pointer.
    while (binlog.set_position(logName + ".000001", 4)) {
        Logger::error("MYSQLCONNECTOR: Can't reposition the binary log reader. Please check if the binlog mode is enabled");
        sleep(listenerWaitTime);
    }

    bool quit = false;
    while (!quit) {
        Binary_log_event *event;
        binlog.wait_for_next_event(&event);
        /*
         * Perform a special action based on event type.
         * This action happens after the content_handler processing the event.
         */
        switch (event->header()->type_code) {
        /*
         * When a binary log file exceeds a size limit, a ROTATE_EVENT is
         * written at the end of the file that points to the next file in
         * the sequence. This event is information for the slave to know the
         * name of the next binary log it is going to receive.
         */
        case mysql::ROTATE_EVENT: {
            mysql::Rotate_event *rot = static_cast<mysql::Rotate_event *>(event);
            Logger::debug(
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
            if (rowEventTimestamp - lastSavingIndexTime >= listenerWaitTime) {
                lastSavingIndexTime = rowEventTimestamp;
                saveLastSavingIndexTime(lastSavingIndexTime);
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
