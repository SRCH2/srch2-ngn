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
#include "mongo/bson/bsonobj.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/client/dbclientinterface.h"
#include "util/Logger.h"
using namespace std;

MongoDBConnector::MongoDBConnector(){
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
	std::string dbname = "mongodb";
	std::string username = "dummy";
	std::string password = "dummy";

	//connector->init(dbname, username, password);
	while (!stop) {
		std::cout << "listening " << std::endl;
		//get record and parse into json format
		this->serverHandle->insertRecord("");
		this->serverHandle->deleteRecord("");
		this->serverHandle->updateRecord("");
		sleep(1);
	}
	return NULL;
}

MongoDBConnector::~MongoDBConnector() {

}

bool MongoDBConnector::getOplogConnector() {
	string mongoNamespace = "local.oplog.rs";

	string dbname = this->serverHandle->configLookUp("db");
	string collection = this->serverHandle->configLookUp("collection");
	string filterNamespace = dbname + "." + collection;
	string host = this->serverHandle->configLookUp("host");
	string port = this->serverHandle->configLookUp("port");

	try {
		string hostAndport = host;
		if (port.size()) {
			hostAndport.append(":").append(port); // std::string is mutable unlike java
		}
		mongo::ScopedDbConnection * mongoConnector =
				mongo::ScopedDbConnection::getScopedDbConnection(hostAndport);
		mongo::DBClientBase& oplogConnection = mongoConnector->conn();
	} catch (const mongo::DBException &e) {
		Logger::console("MongoDb Exception : %s", e.what());
	} catch (const exception& ex) {
		Logger::console("Unknown exception : %s", ex.what());
	}
}

// the class factories
extern "C" DataConnector* create() {
    return new MongoDBConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}
