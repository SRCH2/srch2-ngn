/*
 * MongodbAdapter.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: sbisht
 */
#include "MongodbAdapter.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/client/dbclientinterface.h"
#include "util/Logger.h"
#include "json/json.h"
#include "AnalyzerFactory.h"
#include "JSONRecordParser.h"
#include "boost/algorithm/string.hpp"
#include <instantsearch/Constants.h>
#include "Srch2Server.h"
#include "IndexWriteUtil.h"

using namespace std;

namespace srch2 {
namespace httpwrapper {

pthread_t * MongoDataSource::mongoListenerThread = new pthread_t;
time_t MongoDataSource::bulkLoadEndTime = 0;

void MongoDataSource::createNewIndexes(srch2is::Indexer* indexer, const ConfigManager *configManager) {

    string dbNameWithCollection = configManager->getMongoDbName() +
            "."  + configManager->getMongoCollection();
    string host = configManager->getMongoServerHost();
    string port = configManager->getMongoServerPort();
    boost::algorithm::trim(host);
    try {
        string hostAndport = host;
        if (port.size())
            hostAndport.append(":").append(port);  // std::string is mutable unlike java
        mongo::ScopedDbConnection * mongoConnector =
        		mongo::ScopedDbConnection::getScopedDbConnection(hostAndport);
        Logger::console("connected to Mongo Db Instance %s-%s", host.c_str(), port.c_str());

        unsigned collectionCount = mongoConnector->conn().count(dbNameWithCollection);
        // We fetch data from mongo db only if there are some records to be processed
        unsigned indexCnt = 0;
        if (collectionCount > 0) {
            srch2is::Record *record = new srch2is::Record(indexer->getSchema());
            // create new analyzer
            srch2is::Analyzer *analyzer = AnalyzerFactory::createAnalyzer(configManager);
            // query the mongo database for all the objects in collection. mongo::BSONObj() means get
            // all records from mongo db
            auto_ptr<mongo::DBClientCursor> cursor = mongoConnector->conn().query(dbNameWithCollection, mongo::BSONObj());
            // get data from cursor
            while (cursor->more()) {
                mongo::BSONObj bsonObj = cursor->next();
                mongo::BSONElement bsonElmt;
                bsonObj.getObjectID(bsonElmt);
                // parse BSON object returned by cursor and fill in record object
                bool result = BSONParser::parse(record, bsonObj, configManager);
                if (result) {
                    indexer->addRecord(record, analyzer, 0);
                    ++indexCnt;
                }
                record->clear();
                if (indexCnt && (indexCnt % 1000) == 0)
                    Logger::console("Indexed %d records so far ...", indexCnt);
            }
            Logger::console("Total indexed %d / %d records.", indexCnt, collectionCount);

            delete analyzer;
            delete record;

        } else {
            Logger::console("No data found in the collection %s", dbNameWithCollection.c_str());
        }
        indexer->commit();
        if (indexCnt > 0) {
            Logger::console("Saving Indexes.....");
            indexer->save();
            Logger::console("Indexes saved.");
        }
        mongoConnector->done();
    } catch( const mongo::DBException &e ) {
        Logger::console("MongoDb Exception : %s", e.what());
        exit(-1);
    } catch (const exception& ex){
        Logger::console("Unknown exception : %s", ex.what());
        exit(-1);
    }
}

void MongoDataSource::spawnUpdateListener(Srch2Server * server){

    int res = pthread_create(MongoDataSource::mongoListenerThread, NULL,
    		MongoDataSource::runUpdateListener, (void *)server);
    if (res != 0)
        Logger::console("Could not create mongo oplog reader thread: error = %d", res);

}

void* MongoDataSource::runUpdateListener(void *searchServer){
    Logger::console("MOGNOLISTENER: thread started ...");
    Srch2Server * server =(Srch2Server *)searchServer;
    const ConfigManager *configManager = server->indexDataContainerConf;
    string mongoNamespace= "local.oplog.rs";
    string filterNamespace = "";
    filterNamespace.append(configManager->getMongoDbName()).append(".").append(configManager->getMongoCollection());
    string host = configManager->getMongoServerHost();
    string port = configManager->getMongoServerPort();
    unsigned retryCount = 0;
  retry:
    try {
    string hostAndport = host;
    if (port.size()) {
        hostAndport.append(":").append(port);  // std::string is mutable unlike java
    }
    mongo::ScopedDbConnection * mongoConnector = mongo::ScopedDbConnection::getScopedDbConnection(hostAndport);
    mongo::DBClientBase& oplogConnection = mongoConnector->conn();

    mongo::BSONElement _lastValue = mongo::minKey.firstElement();
    bool printOnce = true;
    time_t opLogTime = 0;
    time_t threadSpecificCutOffTime = bulkLoadEndTime;

    // first check whether the replication is enabled and the host is primary of the
    // replica set
    if (!oplogConnection.exists(mongoNamespace)){
    	Logger::error("MOGNOLISTENER: oplog does not exist on host = %s", host.c_str());
    	Logger::error("MOGNOLISTENER: either replication is not enabled on the host instance "
    			"or the host is not a primary member of replica set");
    	Logger::error("MOGNOLISTENER: exiting ... ");
    	return NULL;
    }
    while(1) {
        // open the tail cursor on the capped collection oplog.rs
        // the cursor will wait for more data when it reaches at
        // the end of the collection. For more info please see
        // following link.
        // http://docs.mongodb.org/manual/tutorial/create-tailable-cursor/
        mongo::Query query = QUERY("_id" << mongo::GT << _lastValue).hint( BSON( "$natural" << 1 ) );
        auto_ptr<mongo::DBClientCursor> tailCursor = oplogConnection.query(mongoNamespace, query, 0, 0, 0,
                mongo::QueryOption_CursorTailable | mongo::QueryOption_AwaitData );
        while (1) {
            if (tailCursor->more()){
                mongo::BSONObj obj = tailCursor->next();
                string recNS = obj.getStringField("ns");
                if (recNS.compare(filterNamespace) == 0) {
                    mongo::BSONElement timestampElement = obj.getField("ts");
                    opLogTime = timestampElement.timestampTime().toTimeT();
                    //time_t curRecTime = obj.getField("ts");
                    if ( opLogTime > threadSpecificCutOffTime) {
                        parseOpLogObject(obj, filterNamespace, server, oplogConnection);
                    }
                }
                _lastValue = obj["_id"];
                printOnce = true;
            } else {
                // cursor is either dead or does not have more records
                // store the timestamp of the last record processed, so that
                // when the cursor fetches more data we can filter out any
                // records processed earlier. Alternative is to store current time.
            	threadSpecificCutOffTime = opLogTime;
                if (tailCursor->isDead())
                    break;
                if (printOnce){
                    Logger::console("MOGNOLISTENER: waiting for updates ...");
                    printOnce = false;
                }
                retryCount = 0; // reset retryCount
                sleep(configManager->getMongoListenerWaitTime());  // sleep...do not hog the CPU
            }
        }
    }
    }catch( const mongo::DBException &e ) {
    	Logger::console("MongoDb Exception : %s", e.what());
    } catch (const exception& ex){
    	Logger::console("Unknown exception : %s", ex.what());
    }
    sleep(configManager->getMongoListenerWaitTime());
    if (retryCount++ < configManager->getMongoListnerMaxRetryCount()) {
    	Logger::console("MONGOLISTENER: trying again ...");
    	goto retry;
    }
    // if all retries failed then exit the thread
    Logger::error("MONGOLISTENER: exiting...");
    return NULL;
}

bool BSONParser::parse(srch2is::Record * record, const mongo::BSONObj& bsonObj, const ConfigManager* configManager){

    stringstream error;
    bool result = JSONRecordParser::populateRecordFromJSON(bsonObj.jsonString(), configManager, record, error);
    if(!result)
        cout << error.str() << endl;
    return result;
}

// The parseOpLogObject function parses the oplog's JSON record to get
// the incremental updates. The logic can be categorized as following.
//
// 1. it identifies the type of update using oplog record field "op"
// op == "i" ---> insert operation
// op == "d" ---> delete operation
// op == "u" ---> update operation
//
// 2. Based on the "op" value , the function perform insert/update/delete on
// indexes.
//
// for op == 'i' :  the full record is found in oplog entry itself which is inserted into
// indexes.
// for op == 'd' : Only primary key of the record is present in oplog. We use
// this primary key to delete the record in indexes.
// for op == 'u' : Only primary key of a record is present in oplog. In order to fetch
// the updated record, we query mongo db using the primary key found in oplog.
//
void MongoDataSource::parseOpLogObject(mongo::BSONObj& bobj, string currentNS, Srch2Server * server, mongo::DBClientBase& oplogConnection){

    Logger::info("MONGO LISTENER PROCESSING : %s", bobj.jsonString().c_str());
    string operation =  bobj.getField("op").valuestrsafe();
    if (operation.size() == 0){
        Logger::warn("MONGO: oplog entry does not have op field or the value is not set");
        operation = "x"; // undefined operation
    }

    const ConfigManager *configManager = server->indexDataContainerConf;
    stringstream errorMsg;

    switch(operation[0])
    {
    case 'i':
    case 'I':
    {
        errorMsg << "INSERT : ";
        // Parse example data
        Json::Value root;
        Json::Reader reader;
        mongo::BSONObj bsonData =  bobj.getObjectField("o");
        string jsonRecord = bsonData.jsonString();
        bool parseSuccess = reader.parse(jsonRecord, root, false);

        if (parseSuccess == false) {
            Logger::error("BSON object parse error %s", jsonRecord.c_str());
        } else {
            srch2is::Record *record = new srch2is::Record(server->indexer->getSchema());
            IndexWriteUtil::_insertCommand(server->indexer,
                    configManager, root, 0, record, errorMsg);
            record->clear();
            delete record;
        }
        break;
    }
    case 'd':
    case 'D':
    {
        errorMsg << "DELETE : ";
        mongo::BSONElement _oElement = bobj.getField("o");
        if (_oElement.type() != mongo::Object){
            Logger::error("MONGO_LISTENER:DELETE: \"o\" element is not an Object type!! ..Cannot update engine");
            break;
        }
        mongo::BSONElement pk = _oElement.Obj().getField(configManager->getPrimaryKey().c_str());
        string primaryKeyStringValue;
        //mongo::BSONElement pk = bobj.getField(cmPtr->getPrimaryKey().c_str());
        if (pk.type() == mongo::jstOID && configManager->getPrimaryKey().compare("_id") == 0){
            mongo::OID oid = pk.OID();
            primaryKeyStringValue = oid.str();
        }
        else{
            primaryKeyStringValue = pk.valuestrsafe();
        }
        Logger::debug("Delete: pk = %s  val =  %s ", configManager->getPrimaryKey().c_str(), primaryKeyStringValue.c_str());
        if (primaryKeyStringValue.size()) {
            errorMsg << "{\"rid\":\"" << primaryKeyStringValue << "\",\"delete\":\"";
            //delete the record from the index
            switch(server->indexer->deleteRecord(primaryKeyStringValue, 0))
            {
            case srch2is::OP_FAIL:
            {
                errorMsg << "failed\",\"reason\":\"no record with given primary key\"}";
                break;
            }
            default: // OP_SUCCESS.
            {
                errorMsg << "success\"}";
            }
            };
        }
        else
        {
            errorMsg << "{\"rid\":\"NULL\",\"delete\":\"failed\",\"reason\":\"no record with given primary key\"}";
        }
        break;
    }
    case 'u':
    case 'U':
    {
        errorMsg << "UPDATE : ";
        mongo::BSONElement _o2Element = bobj.getField("o2");
        if (_o2Element.type() != mongo::Object){
            Logger::error("MONGO_LISTENER:UPDATE: o2 element is not an ObjectId type!! ..Cannot update engine");
            break;
        }
        mongo::BSONObj _internalMongoId = bobj.getField("o2").Obj();
        auto_ptr<mongo::DBClientCursor> cursor = oplogConnection.query(currentNS, _internalMongoId);
        mongo::BSONObj updateRecord = cursor->next();  // should return only one

        if( string(updateRecord.firstElementFieldName()).compare("$err") == 0 ) {
            Logger::error("MONGO_LISTENER:UPDATE: updated record could not be found in db!! ..Cannot update engine");
            break;
        }
        mongo::BSONElement pk = updateRecord.getField(configManager->getPrimaryKey().c_str());
        string primaryKeyStringValue ;
        if (pk.type() == mongo::jstOID && configManager->getPrimaryKey().compare("_id") == 0){
            mongo::OID oid = pk.OID();
            primaryKeyStringValue = oid.str();
        }
        else{
            primaryKeyStringValue = pk.valuestrsafe();
        }
        Logger::debug("MONGO_LISTENER:UPDATE: pk = %s  val =  %s ",
        		configManager->getPrimaryKey().c_str(), primaryKeyStringValue.c_str());
        unsigned deletedInternalRecordId;
        if (primaryKeyStringValue.size()) {
            srch2is::INDEXWRITE_RETVAL ret =
            		server->indexer->deleteRecordGetInternalId(primaryKeyStringValue, 0, deletedInternalRecordId);
            if (ret == srch2is::OP_FAIL)
            {
                errorMsg << "failed\",\"reason\":\"no record with given primary key\"}";
                break;
            }
            else
                Logger::debug("MONGO_LISTENER:UPDATE: deleted record ");

            if ( server->indexer->getNumberOfDocumentsInIndex() < configManager->getDocumentLimit() )
            {
                Json::Value root;
                Json::Reader reader;
                string jsonRecord = updateRecord.jsonString();
                bool parseSuccess = reader.parse(jsonRecord, root, false);
                if (parseSuccess == false) {
                    Logger::error("UPDATE : BSON object parse error %s", jsonRecord.c_str());
                } else {
                    srch2is::Record *record = new srch2is::Record(server->indexer->getSchema());
                    IndexWriteUtil::_insertCommand(server->indexer,
                            configManager, root, 0, record, errorMsg);
                    record->clear();
                    delete record;
                }
                Logger::debug("MONGO_LISTENER:UPDATE: inserted record ");
            }
            else
            {
                errorMsg << "failed\",\"reason\":\"insert: Document limit reached." << endl;
                /// reaching here means the insert failed, need to resume the deleted old record
                srch2::instantsearch::INDEXWRITE_RETVAL ret =
                		server->indexer->recoverRecord(primaryKeyStringValue, 0, deletedInternalRecordId);
            }
        }
        break;
    }
    default:
        break;
        Logger::warn("The mongodb operation (ops='%c') is not supported by the engine", operation[0]);
    }
    Logger::debug(errorMsg.str().c_str());
}

}
}

