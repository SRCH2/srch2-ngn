/*
 * mongodbconnector.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */
#include "mongodbconnector.h"
#include <iostream>
#include <unistd.h>
#include "mongo/client/dbclient.h"
#include <iomanip>
#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;

time_t MongoDBConnector::bulkLoadEndTime = 0;

MongoDBConnector::MongoDBConnector() {
	serverHandle=NULL;
	oplogConnection=NULL;
}

bool MongoDBConnector::init(ServerInterface *serverHandle) {
	this->serverHandle = serverHandle;
	bulkLoadEndTime=time(NULL);
	if(!conn()){
		return false;
	}
	return true;
}

bool MongoDBConnector::conn() {
	string mongoNamespace = "local.oplog.rs";

	string host = this->serverHandle->configLookUp("host");
	string port = this->serverHandle->configLookUp("port");

	int listenerWaitTime = atoi(
			this->serverHandle->configLookUp("listenerWaitTime").c_str());
	int maxRetryOnFailure = atoi(
			this->serverHandle->configLookUp("maxRetryOnFailure").c_str());

	for (unsigned retryCount = -1; retryCount != maxRetryOnFailure;
			retryCount++) {

		try {
			string hostAndport = host;
			if (port.size()) {
				hostAndport.append(":").append(port); // std::string is mutable unlike java
			}
			mongoConnector = new mongo::ScopedDbConnection(hostAndport);
			oplogConnection = &mongoConnector->conn();

			// first check whether the replication is enabled and the host is primary of the
			// replica set
			if (!oplogConnection->exists(mongoNamespace)) {
				printf("MOGNOLISTENER: oplog does not exist on host = %s \n",
						host.c_str());
				printf(
						"MOGNOLISTENER: either replication is not enabled on the host instance "
								"or the host is not a primary member of replica set \n");
				printf("MOGNOLISTENER: exiting ... \n");
				return false;
			}

			return true;
		} catch (const mongo::DBException &e) {
			printf("MongoDb Exception : %s \n", e.what());
		} catch (const exception& ex) {
			printf("Unknown exception : %s \n", ex.what());
		}
		sleep(listenerWaitTime);
		printf("MONGOLISTENER: trying again ...");
	}
	// if all retries failed then exit the thread
	printf("MONGOLISTENER: exiting...\n");
	return false;
}

void MongoDBConnector::createNewIndexes(){
	std::cout<<"Calling Create New Indexes"<<std::endl;
	string mongoNamespace = "local.oplog.rs";
	string dbname = this->serverHandle->configLookUp("db");
	string collection = this->serverHandle->configLookUp("collection");
	string filterNamespace = dbname + "." + collection;
	try {
		unsigned collectionCount = oplogConnection->count(filterNamespace);
		// We fetch data from mongo db only if there are some records to be processed
		unsigned indexedRecordsCount = 0;
		if (collectionCount > 0) {
			// query the mongo database for all the objects in collection. mongo::BSONObj() means get
			// all records from mongo db
			auto_ptr<mongo::DBClientCursor> cursor = oplogConnection->query(
					filterNamespace, mongo::BSONObj());
			// get data from cursor
			while (cursor->more()) {
				mongo::BSONObj obj = cursor->next();
				// parse BSON object returned by cursor and fill in record object
				string recNS = obj.getStringField("ns");

				string jsonRecord = obj.jsonString();
				this->serverHandle->insertRecord(jsonRecord);

				++indexedRecordsCount;
				if (indexedRecordsCount && (indexedRecordsCount % 1000) == 0)
					printf("Indexed %d records so far ...",
							indexedRecordsCount);
			}
			printf("Total indexed %d / %d records.", indexedRecordsCount,
					collectionCount);

		} else {
			printf("No data found in the collection %s",
					filterNamespace.c_str());
		}
	} catch (const mongo::DBException &e) {
		printf("MongoDb Exception : %s", e.what());
		exit(-1);
	} catch (const exception& ex) {
		printf("Unknown exception : %s", ex.what());
		exit(-1);
	}
}

time_t MongoDBConnector::getLastExecutedLogTime(){
	std::string path=this->serverHandle->configLookUp("srch2Home")+"mongodb_data/"+"data.bin";
	if(access(path.c_str(), F_OK) == 0){
		ifstream a_file(path.c_str(),ios::in|ios::binary);
		time_t t;
		a_file >> t;
		a_file.close();
		bulkLoadEndTime=t;
	}
	return bulkLoadEndTime;
}

void MongoDBConnector::saveLastExecutedLogTime(time_t t) {
	std::string path=this->serverHandle->configLookUp("srch2Home")+"mongodb_data/";
	if (access(path.c_str(), F_OK) != 0) {
		boost::filesystem::create_directories(path);
	}
	std::string pt=path+"data.bin";
	ofstream a_file(pt.c_str(), ios::trunc | ios::binary);
	a_file << t;
	a_file.flush();
	a_file.close();
}

// illustrative code..
void* MongoDBConnector::runListener() {
	std::cout<<"Calling Run Listener"<<std::endl;
	bool printOnce = true;
	time_t opLogTime = 0;
	time_t threadSpecificCutOffTime = getLastExecutedLogTime();

	string mongoNamespace = "local.oplog.rs";
	string dbname = this->serverHandle->configLookUp("db");
	string collection = this->serverHandle->configLookUp("collection");
	string filterNamespace = dbname + "." + collection;

	retry: try {
		mongo::BSONElement _lastValue = mongo::BSONObj().firstElement();

		mongo::Query query = mongo::Query().hint(BSON("$natural" << 1));
		while (1) {
			// open the tail cursor on the capped collection oplog.rs
			// the cursor will wait for more data when it reaches at
			// the end of the collection. For more info please see
			// following link.
			// http://docs.mongodb.org/manual/tutorial/create-tailable-cursor/
			auto_ptr<mongo::DBClientCursor> tailCursor = oplogConnection->query(
					mongoNamespace, query, 0, 0, 0,
					mongo::QueryOption_CursorTailable
					| mongo::QueryOption_AwaitData);
			while (1) {
				if (tailCursor->more()) {
					mongo::BSONObj obj = tailCursor->next();
					string recNS = obj.getStringField("ns");
					if (recNS.compare(filterNamespace) == 0) {
						mongo::BSONElement timestampElement = obj.getField(
								"ts");
						opLogTime = timestampElement.timestampTime().toTimeT();
						if (opLogTime > threadSpecificCutOffTime) {
							parseOpLogObject(obj, filterNamespace,
									*oplogConnection);
						}
					}
					_lastValue = obj["_id"];
					printOnce = true;
				} else {
					// cursor is either dead or does not have more records
					// store the timestamp of the last record processed, so that
					// when the cursor fetches more data we can filter out any
					// records processed earlier. Alternative is to store current time.
					if(threadSpecificCutOffTime!=opLogTime){
						saveLastExecutedLogTime(opLogTime);
					}
					threadSpecificCutOffTime = opLogTime;
					if (tailCursor->isDead())
						break;
					if (printOnce) {
						printf("MOGNOLISTENER: waiting for updates ... \n");
						printOnce = false;
					}

					sleep(1); // sleep...do not hog the CPU
				}
			}
			query = QUERY("_id" << mongo::GT << _lastValue).hint( BSON( "$natural" << 1 ) );
		}
	} catch( const mongo::DBException &e ) {
		printf("MongoDb Exception : %s \n", e.what());
	} catch (const exception& ex) {
		printf("Unknown exception : %s \n", ex.what());
	}
	sleep(atoi(this->serverHandle->configLookUp("listenerWaitTime").c_str()));
	if (conn()) {
		goto retry;
	}

	return NULL;
}

void MongoDBConnector::parseOpLogObject(mongo::BSONObj& bobj, string currentNS,
		mongo::DBClientBase& oplogConnection) {
	printf("MONGO LISTENER PROCESSING : %s \n", bobj.jsonString().c_str());
	string operation = bobj.getField("op").valuestrsafe();
	if (operation.size() == 0) {
		printf(
				"MONGO: oplog entry does not have op field or the value is not set \n");
		operation = "x"; // undefined operation
	}

	switch (operation[0]) {
	case 'i':
	case 'I': {
		// Parse example data
		mongo::BSONObj bsonData = bobj.getObjectField("o");
		string jsonRecord = bsonData.jsonString();
		this->serverHandle->insertRecord(jsonRecord);
		break;
	}
	case 'd':
	case 'D': {
		mongo::BSONElement _oElement = bobj.getField("o");
		if (_oElement.type() != mongo::Object) {
			printf(
					"MONGO_LISTENER:DELETE: \"o\" element is not an Object type!! ..Cannot update engine \n");
			break;
		}
		mongo::BSONElement pk = _oElement.Obj().getField(
				this->serverHandle->configLookUp("uniqueKey").c_str());
		string primaryKeyStringValue;
		if (pk.type() == mongo::jstOID
				&& this->serverHandle->configLookUp("uniqueKey").compare("_id")
						== 0) {
			mongo::OID oid = pk.OID();
			primaryKeyStringValue = oid.str();
		} else {
			primaryKeyStringValue = pk.valuestrsafe();
		}

		printf("Delete: pk = %s  val =  %s \n",
				this->serverHandle->configLookUp("uniqueKey").c_str(),
				primaryKeyStringValue.c_str());
		this->serverHandle->deleteRecord(primaryKeyStringValue);

		break;
	}
	case 'u':
	case 'U': {
		mongo::BSONElement _o2Element = bobj.getField("o2");
		if (_o2Element.type() != mongo::Object) {
			printf(
					"MONGO_LISTENER:UPDATE: o2 element is not an ObjectId type!! ..Cannot update engine\n");
			break;
		}
		mongo::BSONObj _internalMongoId = bobj.getField("o2").Obj();
		auto_ptr<mongo::DBClientCursor> cursor = oplogConnection.query(
				currentNS, _internalMongoId);
		mongo::BSONObj updateRecord = cursor->next();  // should return only one

		if (string(updateRecord.firstElementFieldName()).compare("$err") == 0) {
			printf(
					"MONGO_LISTENER:UPDATE: updated record could not be found in db!! ..Cannot update engine \n");
			break;
		}

		mongo::BSONElement pk = _o2Element.Obj().getField(
				this->serverHandle->configLookUp("uniqueKey").c_str());
		string primaryKeyStringValue;
		if (pk.type() == mongo::jstOID
				&& this->serverHandle->configLookUp("uniqueKey").compare("_id")
						== 0) {
			mongo::OID oid = pk.OID();
			primaryKeyStringValue = oid.str();
		} else {
			primaryKeyStringValue = pk.valuestrsafe();
		}

		string jsonRecord = updateRecord.jsonString();
		this->serverHandle->updateRecord(primaryKeyStringValue,jsonRecord);

		break;
	}
	default:
		break;
		printf(
				"The mongodb operation (ops='%c') is not supported by the engine \n",
				operation[0]);
	}
}

MongoDBConnector::~MongoDBConnector() {
	mongoConnector->done();
}

// the class factories
extern "C" DataConnector* create() {
	return new MongoDBConnector;
}

extern "C" void destroy(DataConnector* p) {
	delete p;
}
