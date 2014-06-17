/*
 * mongodbconnector.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */
#include "mongodbconnector.h"
#include <iostream>
#include <unistd.h>
//#include "mongo/client/dbclient.h"
//#include "mongo/bson/bsonobj.h"
//#include "mongo/client/dbclientcursor.h"
//#include "mongo/client/dbclientinterface.h"
using namespace std;


MongoDBConnector::MongoDBConnector() {
	cout << "constructor " << endl;
	std::cout << "constructor " << std::endl;
}

void MongoDBConnector::init(ServerInterface *serverHandle) {
	this->serverHandle = serverHandle;
	serverHandle->insertRecord("");

	std::cout << "init " << std::endl;
}

// illustrative code..
void* MongoDBConnector::runListener() {
	// connect to db
	bool stop = false;

	printf("MOGNOLISTENER: thread started ... \n");
	string mongoNamespace = "local.oplog.rs";

	string dbname = this->serverHandle->configLookUp("db");
	string collection = this->serverHandle->configLookUp("collection");
	string filterNamespace = dbname + "." + collection;
	string host = this->serverHandle->configLookUp("host");
	string port = this->serverHandle->configLookUp("port");
	unsigned retryCount = 0;

	retry: try {
		string hostAndport = host;
		if (port.size()) {
			hostAndport.append(":").append(port); // std::string is mutable unlike java
		}
		mongo::ScopedDbConnection * mongoConnector;
//		mongo::ScopedDbConnection * mongoConnector =
//				mongo::ScopedDbConnection::getScopedDbConnection(hostAndport);
		mongo::DBClientBase& oplogConnection = mongoConnector->conn();

		mongo::BSONElement _lastValue = mongo::minKey.firstElement();
		bool printOnce = true;
		time_t opLogTime = 0;
		time_t threadSpecificCutOffTime = 0;

		// first check whether the replication is enabled and the host is primary of the
		// replica set
		if (!oplogConnection.exists(mongoNamespace)) {
			printf("MOGNOLISTENER: oplog does not exist on host = %s \n",
					host.c_str());
			printf(
					"MOGNOLISTENER: either replication is not enabled on the host instance "
							"or the host is not a primary member of replica set \n");
			printf("MOGNOLISTENER: exiting ... \n");
			return NULL;
		}
		while (1) {
			// open the tail cursor on the capped collection oplog.rs
			// the cursor will wait for more data when it reaches at
			// the end of the collection. For more info please see
			// following link.
			// http://docs.mongodb.org/manual/tutorial/create-tailable-cursor/
			mongo::Query query = QUERY("_id" << mongo::GT << _lastValue).hint( BSON( "$natural" << 1 ) );
			auto_ptr<mongo::DBClientCursor> tailCursor = oplogConnection.query(mongoNamespace, query, 0, 0, 0,
					mongo::QueryOption_CursorTailable | mongo::QueryOption_AwaitData );
			while (1) {
				if (tailCursor->more()) {
					mongo::BSONObj obj = tailCursor->next();
					string recNS = obj.getStringField("ns");
					if (recNS.compare(filterNamespace) == 0) {
						mongo::BSONElement timestampElement = obj.getField("ts");
						opLogTime = timestampElement.timestampTime().toTimeT();
						//time_t curRecTime = obj.getField("ts");
						if ( opLogTime > threadSpecificCutOffTime) {
							parseOpLogObject(obj, filterNamespace, oplogConnection);
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
					if (printOnce) {
						printf("MOGNOLISTENER: waiting for updates ... \n");
						printOnce = false;
					}
					retryCount = 0; // reset retryCount

					sleep(atoi(this->serverHandle->configLookUp("listenerWaitTime").c_str()));// sleep...do not hog the CPU
				}
			}
		}
	} catch( const mongo::DBException &e ) {
		printf("MongoDb Exception : %s \n", e.what());
	} catch (const exception& ex) {
		printf("Unknown exception : %s \n", ex.what());
	}
	sleep(atoi(this->serverHandle->configLookUp("listenerWaitTime").c_str()));
	if (retryCount++
			< atoi(
					this->serverHandle->configLookUp("maxRetryOnFailure").c_str())) {
		printf("MONGOLISTENER: trying again ...");
		goto retry;
	}
	// if all retries failed then exit the thread
	printf("MONGOLISTENER: exiting...\n");

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

		string jsonRecord = updateRecord.jsonString();
		this->serverHandle->updateRecord(jsonRecord);

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

}

// the class factories
extern "C" DataConnector* create() {
	return new MongoDBConnector;
}

extern "C" void destroy(DataConnector* p) {
	delete p;
}
