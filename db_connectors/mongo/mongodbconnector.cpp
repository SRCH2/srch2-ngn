/*
 * mongodbconnector.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: Chen Liu at SRCH2
 */
#include "mongodbconnector.h"
#include <iostream>
#include <unistd.h>
#include "mongo/client/dbclient.h"
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include "io.h"
#include "Logger.h"

using namespace std;
using srch2::util::Logger;

MongoDBConnector::MongoDBConnector() {
    serverHandle = NULL;
    oplogConnection = NULL;
    mongoConnector = NULL;
    lastAccessedLogRecordTime = 0;
}

//Init the connector, call connect
int MongoDBConnector::init(ServerInterface *serverHandle) {
    this->serverHandle = serverHandle;

    // For MongoDB, the primary key should always be "_id".
    std::string uniqueKey;
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    if (uniqueKey.compare("_id") != 0) {
        Logger::error(
                "MOGNOLISTENER: The PrimaryKey in the config file for the "
                        "MongoDB adapter should always be \"_id\", not %s .",
                uniqueKey.c_str());
        return -1;
    }

    if (!checkConfigValidity() || !connectToDB()) {
        return -1;
    }
    return 0;
}

//Check config validity. e.g. if contains port, dbname, etc.
bool MongoDBConnector::checkConfigValidity() {
    std::string host, port, db, uniqueKey, collection;
    this->serverHandle->configLookUp("host", host);
    this->serverHandle->configLookUp("port", port);
    this->serverHandle->configLookUp("db", db);
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);
    this->serverHandle->configLookUp("collection", collection);

    bool ret = (host.size() != 0) && (port.size() != 0) && (db.size() != 0)
            && (uniqueKey.size() != 0) && (collection.size() != 0);
    if (!ret) {
        Logger::error(
                "MOGNOLISTENER: database host, port, db, collection, uniquekey must be set.");
        return false;
    }

    int value = strtol(port.c_str(), NULL, 10);
    if (value <= 0 || value > USHRT_MAX) {
        Logger::error("MOGNOLISTENER: database port must be between 1 and %d .",
        USHRT_MAX);
        return false;
    }

    return true;
}

//Connect to the mongodb
bool MongoDBConnector::connectToDB() {
    string mongoNamespace = "local.oplog.rs";

    string host, port, listenerWaitTimeStr;
    this->serverHandle->configLookUp("host", host);
    this->serverHandle->configLookUp("port", port);
    this->serverHandle->configLookUp("listenerWaitTime", listenerWaitTimeStr);

    int listenerWaitTime = 1;
    if (listenerWaitTimeStr.size() != 0) {
        listenerWaitTime = static_cast<int>(strtol(listenerWaitTimeStr.c_str(),
        NULL, 10));
    }

    string hostAndport = host;
    if (port.size()) {
        hostAndport.append(":").append(port);
    }

    while (1) {
        try {
            if (mongoConnector != NULL) {
                mongoConnector->done();
                delete mongoConnector;
                mongoConnector = NULL;
            }

            mongoConnector = new mongo::ScopedDbConnection(hostAndport);
            oplogConnection = &mongoConnector->conn();

            // first check whether the replication is
            // enabled and the host is primary of the replica set
            if (!oplogConnection->exists(mongoNamespace)) {
                Logger::error(
                        "MOGNOLISTENER: oplog does not exist on host = %s .",
                        host.c_str());
                Logger::error(
                        "MOGNOLISTENER: either replication is not enabled on"
                                " the host instance or the host is not a primary "
                                "member of replica set .");
                Logger::info("MOGNOLISTENER: trying again ... ");

                sleep(listenerWaitTime);
                continue;
            }
            return true;
        } catch (const mongo::DBException &e) {
            Logger::error("MOGNOLISTENER: MongoDb Exception %s ", e.what());
        } catch (const exception& ex) {
            Logger::error("MOGNOLISTENER: Unknown exception %s ", ex.what());
        }
        Logger::info("MONGOLISTENER: trying again ...");

        // sleep...do not hog the CPU
        sleep(listenerWaitTime);
    }

    /*
     * If all retries failed then exit the thread
     * TODO engine will crash under MAC_OS at point of pthread_tsd_cleaup if the
     * code reaches this line.
     */
    Logger::error("MONGOLISTENER: exiting...");
    return false;
}

//Load the table records and insert into the engine
int MongoDBConnector::createNewIndexes() {
    string mongoNamespace = "local.oplog.rs";
    string dbname, collection;
    this->serverHandle->configLookUp("db", dbname);
    this->serverHandle->configLookUp("collection", collection);
    string filterNamespace = dbname + "." + collection;

    do {
        try {
            unsigned collectionCount = oplogConnection->count(filterNamespace);
            // We fetch data from mongo db only
            // if there are some records to be processed
            unsigned indexedRecordsCount = 0;
            if (collectionCount > 0) {
                // query the mongo database for all the objects in collection.
                // mongo::BSONObj() means get all records from mongo db
                auto_ptr<mongo::DBClientCursor> cursor = oplogConnection->query(
                        filterNamespace, mongo::BSONObj());
                // get data from cursor
                while (cursor->more()) {
                    mongo::BSONObj obj = cursor->next();
                    // parse BSON object returned by cursor
                    // and fill in record object
                    string recNS = obj.getStringField("ns");

                    string jsonRecord = obj.jsonString();
                    if (this->serverHandle->insertRecord(jsonRecord) == 0) {
                        ++indexedRecordsCount;
                    }

                    if (indexedRecordsCount
                            && (indexedRecordsCount % 1000) == 0)
                        Logger::info(
                                "MOGNOLISTENER: Indexed %d records so far ...",
                                indexedRecordsCount);
                }
                Logger::info("MOGNOLISTENER: Total indexed %d / %d records. ",
                        indexedRecordsCount, collectionCount);

            } else {
                Logger::info(
                        "MOGNOLISTENER: No data found in the collection %s .",
                        filterNamespace.c_str());
            }

            lastAccessedLogRecordTime = time(NULL);
            return 0;

        } catch (const mongo::DBException &e) {
            Logger::error("MOGNOLISTENER: MongoDb Exception %s ", e.what());
        } catch (const exception& ex) {
            Logger::error("MOGNOLISTENER: Unknown exception %s ", ex.what());
        }
    } while (connectToDB());

    return -1;
}

//Load the last time last oplog record accessed
bool MongoDBConnector::loadLastAccessedLogRecordTime() {
    std::string dataDir, srch2Home;

    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", dataDir);
    std::string path = srch2Home + "/" + dataDir + "/mongodb_data/"
            + "data.bin";

    if (checkFileExisted(path.c_str())) {
        ifstream a_file(path.c_str(), ios::in | ios::binary);
        a_file >> lastAccessedLogRecordTime;
        a_file.close();
        return true;
    } else {
        if (lastAccessedLogRecordTime == 0) {
            Logger::error("MONGOLISTENER: Can not find %s. The data may be"
                    "inconsistent. Please rebuild the indexes.", path.c_str());
            Logger::debug("MONGOLISTENER: The connector will use "
                    "the current time.", path.c_str());
            lastAccessedLogRecordTime = time(NULL);
        }
        return false;
    }
}

//Save the time last oplog record accessed
void MongoDBConnector::saveLastAccessedLogRecordTime() {
    std::string srch2Home;
    std::string dataDir;
    this->serverHandle->configLookUp("srch2Home", srch2Home);
    this->serverHandle->configLookUp("dataDir", dataDir);
    std::string path = srch2Home + "/" + dataDir + "/mongodb_data/";

    if (!checkDirExisted(path.c_str())) {
        // S_IRWXU : Read, write, and execute by owner.
        // S_IRWXG : Read, write, and execute by group.
        mkdir(path.c_str(), S_IRWXU | S_IRWXG);
    }

    std::string pt = path + "data.bin";
    ofstream a_file(pt.c_str(), ios::trunc | ios::binary);
    a_file << lastAccessedLogRecordTime;
    a_file.flush();
    a_file.close();
}

//Listen to the oplog and do modification to the engine
int MongoDBConnector::runListener() {
    string mongoNamespace = "local.oplog.rs";
    string dbname, collection, listenerWaitTimeStr;
    this->serverHandle->configLookUp("db", dbname);
    this->serverHandle->configLookUp("collection", collection);
    this->serverHandle->configLookUp("listenerWaitTime", listenerWaitTimeStr);
    string filterNamespace = dbname + "." + collection;

    int listenerWaitTime = 1;
    if (listenerWaitTimeStr.size() != 0) {
        listenerWaitTime = static_cast<int>(strtol(listenerWaitTimeStr.c_str(),
        NULL, 10));
    }

    loadLastAccessedLogRecordTime();

    do {
        bool printOnce = true;
        time_t opLogTime = 0;
        try {
            mongo::BSONElement _lastValue = mongo::BSONObj().firstElement();

            mongo::Query query = mongo::Query().hint(BSON("$natural" << 1));
            while (1) {
                /*
                 * open the tail cursor on the capped collection oplog.rs
                 * the cursor will wait for more data when it reaches at
                 * the end of the collection. For more info please see the
                 * following link.
                 * http://docs.mongodb.org/manual/tutorial/create-tailable-cursor/
                 */
                auto_ptr<mongo::DBClientCursor> tailCursor =
                        oplogConnection->query(mongoNamespace, query, 0, 0, 0,
                                mongo::QueryOption_CursorTailable
                                        | mongo::QueryOption_AwaitData);
                while (1) {
                    if (tailCursor->more()) {
                        mongo::BSONObj obj = tailCursor->next();
                        string recNS = obj.getStringField("ns");
                        if (recNS.compare(filterNamespace) == 0) {
                            mongo::BSONElement timestampElement = obj.getField(
                                    "ts");
                            /*
                             * No timestampElement.timestampTime().toTimeT()
                             * in mongo-cxx-driver-legacy-0.0-26compat-2.6.2
                             * This is a temporarily solution
                             */
                            std::stringstream ss;
                            ss << timestampElement.timestampTime();
                            ss >> opLogTime;
                            opLogTime = opLogTime / 1000;
                            if (opLogTime >= lastAccessedLogRecordTime) {
                                parseOpLogObject(obj, filterNamespace,
                                        *oplogConnection);
                                lastAccessedLogRecordTime = opLogTime;
                            }
                        }
                        _lastValue = obj["_id"];
                        printOnce = true;
                    } else {
                        /*
                         *  Cursor is either dead or does not have more records
                         *  store the timestamp of the last record processed,
                         *  so that when the cursor fetches more data we can
                         *  filter out any records processed earlier.
                         *  Alternative is to store current time.
                         */
                        if (tailCursor->isDead())
                            break;
                        if (printOnce) {
                            Logger::info(
                                    "MOGNOLISTENER: waiting for updates ... ");
                            printOnce = false;
                        }

                        // sleep...do not hog the CPU
                        sleep(listenerWaitTime);
                    }
                }
                query = QUERY(
                        "_id" << mongo::GT << _lastValue).hint(
                        BSON( "$natural" << 1 ) );
            }
        } catch( const mongo::DBException &e ) {
            Logger::error("MOGNOLISTENER: MongoDb Exception %s ", e.what());
            lastAccessedLogRecordTime = opLogTime;
        } catch (const exception& ex) {
            Logger::error("MOGNOLISTENER: Unknown exception %s ", ex.what());
            lastAccessedLogRecordTime = opLogTime;
        }
        sleep(listenerWaitTime);
    } while (connectToDB());	//Retry connecting to the mongodb

    return -1;
}

//Parse the record into json format and do the corresponding operation
void MongoDBConnector::parseOpLogObject(mongo::BSONObj& bobj,
        string filterNamespace, mongo::DBClientBase& oplogConnection) {
    Logger::debug("MOGNOLISTENER: Processing %s .", bobj.jsonString().c_str());
    string operation = bobj.getField("op").valuestrsafe();
    if (operation.size() == 0) {
        Logger::error("MOGNOLISTENER: oplog entry does not have op "
                "field or the value is not set.");
        operation = "x"; // undefined operation
    }

    string uniqueKey;
    this->serverHandle->configLookUp("uniqueKey", uniqueKey);

    switch (operation[0]) {
    case 'i':
    case 'I': {
        // Parse example data and insert
        mongo::BSONObj bsonData = bobj.getObjectField("o");
        string jsonRecord = bsonData.jsonString();
        this->serverHandle->insertRecord(jsonRecord);
        break;
    }
    case 'd':
    case 'D': {
        // Parse example data , find the pk and delete
        mongo::BSONElement _oElement = bobj.getField("o");
        if (_oElement.type() != mongo::Object) {
            Logger::error(
                    "MONGOLISTENER: \"o\" element is "
                            "not an Object type in the delete operation!! ..Cannot update engine.");
            break;
        }
        mongo::BSONElement pk = _oElement.Obj().getField(uniqueKey.c_str());
        string primaryKeyStringValue;
        if (pk.type() == mongo::jstOID && uniqueKey.compare("_id") == 0) {
            mongo::OID oid = pk.OID();
            primaryKeyStringValue = oid.str();
        } else {
            primaryKeyStringValue = pk.valuestrsafe();
        }

        Logger::debug("MOGNOLISTENER: Delete pk = %s  val =  %s .",
                uniqueKey.c_str(), primaryKeyStringValue.c_str());
        this->serverHandle->deleteRecord(primaryKeyStringValue);

        break;
    }
    case 'u':
    case 'U': {
        // Parse example data , find the pk and update
        mongo::BSONElement _o2Element = bobj.getField("o2");
        if (_o2Element.type() != mongo::Object) {
            Logger::error(
                    "MONGOLISTENER: o2 element is not an"
                            " ObjectId type in the update operation!! ..Cannot update engine.");
            break;
        }
        mongo::BSONObj _internalMongoId = bobj.getField("o2").Obj();
        auto_ptr<mongo::DBClientCursor> cursor = oplogConnection.query(
                filterNamespace, _internalMongoId);

        if (!cursor->more()) {
            /*
             * For update event, mongodb uses 2 log events to record it.
             * For example: for an update 'director' from 'Jim' to 'Joe', the 2 log
             * events will be:
             * 1.) { "ts" : { "$timestamp" : { "t" : 1412205101, "i" : 1 } },
             * "h" : { "$numberLong" : "-2184731050177232433" }, "v" : 2,
             * "op" : "u", "ns" : "srch2Test.movies",
             * "o2" : { "_id" : { "$oid" : "542c8a252ca24a0aeebacb85" } },
             * "o" : { "$set" : { "director" : "Joe" } } }
             *
             * 2.) { "_id" : { "$oid" : "542c8a252ca24a0aeebacb85" },
             * "banner_url" : "http://ia.media-imdb.com/images/M/MV5BMTk
             * 5NzM1ODgyN15BMl5BanBnXkFtZTcwMzA5MjAzMw@@._V1_SY317_CR0,0,
             * 214,317_.jpg", "title" : "Terminator 3: Rise of the Machines",
             * "director" : "Joe", "year" : 2003, "genre" : "drama",
             * "trailer_url" : "http://www.youtube.com/watch?v=QHhZK-g7wHo",
             * "id" : 765006 }
             *
             * We only need the 2nd log event, however, the 2nd log event doesn't
             * have a timestamp. So we have to access both log events. And call cursor->next()
             * again if there is one more log event.
             *
             * There is one case that the update event is the last one before
             * the engine shuts down. The next time the engine starts, the
             * connector will only load the 1st one and the 2nd cannot be loaded
             * correctly with Exception: DBClientCursor next() called but
             * more() is false.
             *
             * So we add one more check here to make sure the more() is true
             * before next() is called.
             */
            break;
        }
        mongo::BSONObj updateRecord = cursor->next();  // should return only one
        if (string(updateRecord.firstElementFieldName()).compare("$err") == 0) {
            Logger::error(
                    "MONGOLISTENER: updated record could"
                            " not be found in db in the update operation!! ..Cannot update engine .");
            break;
        }

        mongo::BSONElement pk = _o2Element.Obj().getField(uniqueKey.c_str());
        string primaryKeyStringValue;
        if (pk.type() == mongo::jstOID && uniqueKey.compare("_id") == 0) {
            mongo::OID oid = pk.OID();
            primaryKeyStringValue = oid.str();
        } else {
            primaryKeyStringValue = pk.valuestrsafe();
        }

        string jsonRecord = updateRecord.jsonString();
        this->serverHandle->updateRecord(primaryKeyStringValue, jsonRecord);

        break;
    }
    default:
        break;
        Logger::error("MOGNOLISTENER: The mongodb operation (ops='%c') is not"
                " supported by the engine .", operation[0]);
    }
}

MongoDBConnector::~MongoDBConnector() {
    if (mongoConnector != NULL) {
        mongoConnector->done();
        delete mongoConnector;
    }
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
    return new MongoDBConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}
