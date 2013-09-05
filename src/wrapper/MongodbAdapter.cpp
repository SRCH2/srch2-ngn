/*
 * MongodbAdapter.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: sbisht
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/client/dbclientinterface.h"
#include "util/Logger.h"
#include "json/json.h"
#include "MongodbAdapter.h"
#include "AnalyzerFactory.h"
#include "JSONRecordParser.h"
#include "boost/algorithm/string.hpp"
#include <instantsearch/Constants.h>
#include "Srch2Server.h"
#include "IndexWriteUtil.h"

using namespace std;

namespace srch2 {
namespace httpwrapper {

pthread_t * MongoDataSource::mongoListnerThread = new pthread_t;
time_t MongoDataSource::maxRecTime = 0;
mongo::DBClientConnection * MongoDataSource::pooledConnection = NULL;

void MongoDataSource::createNewIndexes(srch2is::Indexer* indexer, const ConfigManager *configManager) {

    string dbNameWithCollection = configManager->getMongoDbName() +
    							  "."  + configManager->getMongoCollection();
    string host = configManager->getMongoServerHost();
    string port = configManager->getMongoServerPort();
    boost::algorithm::trim(host);
    try {
        mongo::DBClientConnection c;
        string hostAndport = host;
        if (port.size())
        	hostAndport.append(":").append(port);  // std::string is mutable unlike java
        c.connect(hostAndport);
        Logger::console("connected to Mongo Db Instance %s-%s", host.c_str(), port.c_str());
        unsigned collectionCount = c.count(dbNameWithCollection);
        if (collectionCount > 0) {
        	srch2is::Record *record = new srch2is::Record(indexer->getSchema());
        	srch2is::Analyzer *analyzer = AnalyzerFactory::createAnalyzer(configManager);
            auto_ptr<mongo::DBClientCursor> cursor = c.query(dbNameWithCollection, mongo::BSONObj());
            unsigned indexCnt = 0;
            while (cursor->more()) {
                mongo::BSONObj bsonObj = cursor->next();
                mongo::BSONElement bsonElmt;
                bsonObj.getObjectID(bsonElmt);
                unsigned long long ts = bsonElmt.timestampInc();

                bool result = BSONParser::parse(record, bsonObj, configManager);
                if (result)
                	indexer->addRecord(record, analyzer, 0);
                record->clear();

                ++indexCnt;
//                mongo::BSONElement lastValue = bsonObj["_id"];
//                time_t curRecTime = lastValue.OID().asTimeT();
//                if (curRecTime > maxRecTime)
//                	maxRecTime = curRecTime;

                if (indexCnt && (indexCnt % 1000) == 0)
                	Logger::console("Indexed %d records so far ...", indexCnt);
            }
            Logger::console("Total indexed %d / %d records.", indexCnt, collectionCount);

            delete analyzer;
            delete record;

        } else {
        	Logger::console("No data found in the collection %s", dbNameWithCollection.c_str());
        }
        maxRecTime = time(NULL);
        indexer->commit();
        Logger::console("Saving Index.....");
        indexer->save();
        Logger::console("Index saved.");

    } catch( const mongo::DBException &e ) {
    	Logger::console("MongoDb Exception : %s", e.what());
    	exit(-1);
    } catch (const exception& ex){
    	Logger::console("Unknown exception : %s", ex.what());
    	exit(-1);
    }
}

void MongoDataSource::spawnUpdateListener(Srch2Server * server){

	int res = pthread_create(MongoDataSource::mongoListnerThread, NULL, MongoDataSource::runUpdateListener, (void *)server);
	if (res != 0)
		Logger::console("Could not create mongo oplog reader thread: error = %d", res);
	else
		Logger::info("Mongo oplog reader thread started ...");
}

void* MongoDataSource::runUpdateListener(void *vPtr){

	Srch2Server * server =(Srch2Server *)vPtr;
	const ConfigManager *cmPtr = server->indexDataContainerConf;
	string ns = "local.oplog.rs";
	string filterns = "";
	filterns.append(cmPtr->getMongoDbName()).append(".").append(cmPtr->getMongoCollection());
	string host = cmPtr->getMongoServerHost();
	string port = cmPtr->getMongoServerPort();

	// create a pooled connect for update / delete operations...
	pooledConnection = new mongo::DBClientConnection(true);

	mongo::DBClientConnection c(true);
	string hostAndport = host;
	if (port.size())
		hostAndport.append(":").append(port);  // std::string is mutable unlike java
	c.connect(hostAndport);

	pooledConnection->connect(hostAndport);

	mongo::BSONElement _lastValue = mongo::minKey.firstElement();
	bool printOnce = true;
	time_t opLogTime = 0;
	while(1)
	{
		mongo::Query query = QUERY("_id" << mongo::GT << _lastValue).hint( BSON( "$natural" << 1 ) );
		auto_ptr<mongo::DBClientCursor> tailCursor = c.query(ns, query, 0, 0, 0,
	                	mongo::QueryOption_CursorTailable | mongo::QueryOption_AwaitData );
		while (1){
			if (tailCursor->more()){
				mongo::BSONObj obj = tailCursor->next();
				string recNS = obj.getStringField("ns");
				if (recNS.compare(filterns) == 0) {
					//mongo::BSONElement _oElemt  = obj.getField("o");
					//mongo::BSONElement _intMongoId = _oElemt.Obj()["_id"];
					//if (!_intMongoId.isNull() && _intMongoId.ok()) {
					//	time_t curRecTime = _intMongoId.OID().asTimeT();
						//cout << "%%%"<< _oElemt.jsonString(mongo::JS) << endl;
					mongo::BSONElement _tsElemt = obj.getField("ts");
					opLogTime = _tsElemt.timestampTime().toTimeT();
					//time_t curRecTime = obj.getField("ts");
					if ( opLogTime > maxRecTime)
						parseOpLogObject(obj, filterns, server);
					//}
				}
				//bool valdity = obj.isValid();
				_lastValue = obj["_id"];
				printOnce = true;
			} else {
				maxRecTime = opLogTime;  // need better solution ...
				if (tailCursor->isDead())
					break;
				if (printOnce){
					Logger::console("MOGNOLISTENER: waiting for updates ...");
					printOnce = false;
				}
				sleep(1);  // sleep...do not hog the CPU
			}
		}
	}
	delete pooledConnection;
	return NULL;  // if we ever return
}

bool BSONParser::parse(srch2is::Record * record, const mongo::BSONObj& bsonObj, const ConfigManager* configManager){

	stringstream error;
	bool result = JSONRecordParser::populateRecordFromJSON(bsonObj.jsonString(), configManager, record, error);
	if(!result)
		cout << error.str() << endl;
	return result;
}

void MongoDataSource::parseOpLogObject(mongo::BSONObj& bobj, string currentNS, Srch2Server * server){

	Logger::info("MONGO LISTENER PROCESSING : %s", bobj.jsonString().c_str());
	string ops =  bobj.getField("op").valuestrsafe();
	if (ops.size() == 0){
		Logger::warn("MONGO: oplog entry does not have op field or the value is not set");
		ops = "x";
	}

	const ConfigManager *cmPtr = server->indexDataContainerConf; // I hate this name
	stringstream errorMsg;

	switch(ops[0])
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
            		cmPtr, root, 0, record, errorMsg);
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
		mongo::BSONElement pk = _oElement.Obj().getField(cmPtr->getPrimaryKey().c_str());
		string primaryKeyStringValue;
		//mongo::BSONElement pk = bobj.getField(cmPtr->getPrimaryKey().c_str());
		if (pk.type() == mongo::jstOID && cmPtr->getPrimaryKey().compare("_id") == 0){
			mongo::OID oid = pk.OID();
			primaryKeyStringValue = oid.str();
		}
		else{
			primaryKeyStringValue = pk.valuestrsafe();
		}
		Logger::debug("Delete: pk = %s  val =  %s ", cmPtr->getPrimaryKey().c_str(), primaryKeyStringValue.c_str());
		if (primaryKeyStringValue.size())
		{
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
		auto_ptr<mongo::DBClientCursor> cursor = pooledConnection->query(currentNS, _internalMongoId);
		mongo::BSONObj updateRecord = cursor->next();  // should return only one
		if( string(updateRecord.firstElementFieldName()).compare("$err") == 0 ) {
			Logger::error("MONGO_LISTENER:UPDATE: updated record could not be found in db!! ..Cannot update engine");
			break;
		}
		mongo::BSONElement pk = updateRecord.getField(cmPtr->getPrimaryKey().c_str());
		string primaryKeyStringValue ;
		if (pk.type() == mongo::jstOID && cmPtr->getPrimaryKey().compare("_id") == 0){
			mongo::OID oid = pk.OID();
			primaryKeyStringValue = oid.str();
		}
		else{
			primaryKeyStringValue = pk.valuestrsafe();
		}
		Logger::debug("MONGO_LISTENER:UPDATE: pk = %s  val =  %s ", cmPtr->getPrimaryKey().c_str(), primaryKeyStringValue.c_str());
		unsigned deletedInternalRecordId;
		if (primaryKeyStringValue.size()) {
			srch2is::INDEXWRITE_RETVAL ret = server->indexer->deleteRecordGetInternalId(primaryKeyStringValue, 0, deletedInternalRecordId);
			if (ret == srch2is::OP_FAIL)
			{
				errorMsg << "failed\",\"reason\":\"no record with given primary key\"}";
				break;
			}
			else
				Logger::debug("MONGO_LISTENER:UPDATE: deleted record ");

			if ( server->indexer->getNumberOfDocumentsInIndex() < cmPtr->getDocumentLimit() )
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
							cmPtr, root, 0, record, errorMsg);
					record->clear();
					delete record;
				}
				Logger::debug("MONGO_LISTENER:UPDATE: inserted record ");
			}
			else
			{
				errorMsg << "failed\",\"reason\":\"insert: Document limit reached. Email support@srch2.com for account upgrade.\",";
				/// reaching here means the insert failed, need to resume the deleted old record
				srch2::instantsearch::INDEXWRITE_RETVAL ret = server->indexer->recoverRecord(primaryKeyStringValue, 0, deletedInternalRecordId);
			}
		}
		break;
	}
	default:
		break;
		//ignore
	}
	Logger::debug(errorMsg.str().c_str());
}

}
}

