/*
 * mongodbconnector.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */
#include "mongodbconnector.h"
#include <iostream>
using namespace std;

void MongoDBConnector::init(ServerInterface *serverHandle) {
        this->serverHandle = serverHandle;
	std::cout << "init " << std::endl;
}

// illustrative code..
void* MongoDBConnector::runListener(){
	// connect to db
	bool stop = false;
	std::string dbname = "mongodb";
	std::string username = "dummy";
	std::string password = "dummy";
	//connector->init(dbname, username, password);
	while (!stop) {
		//get record and parse into json format
		this->serverHandle->insertRecord("");
		this->serverHandle->deleteRecord("");
		this->serverHandle->updateRecord("");
	}
	return NULL;
}

MongoDBConnector::~MongoDBConnector() {

}

extern "C" {
DataConnector *getNewInstance() {
	return new MongoDBConnector;
}
}
