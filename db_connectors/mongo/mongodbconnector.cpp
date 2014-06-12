/*
 * mongodbconnector.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */
#include "mongodbconnector.h"
#include <iostream>
using namespace std;

MongoDBConnector::MongoDBConnector(){
	std::cout << "constructor " << std::endl;
}

void MongoDBConnector::init(ServerInterface *serverHandle) {
	this->serverHandle = serverHandle;
	std::cout << "init " << std::endl;
}

// illustrative code..
void* MongoDBConnector::runListener() {
	// connect to db
	bool stop = false;
	std::string dbname = "mongodb";
	std::string username = "dummy";
	std::string password = "dummy";
	std::cout << "listening " << std::endl;
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

// the class factories
extern "C" DataConnector* create() {
    return new MongoDBConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}
