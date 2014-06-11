/*
 * mongodbconnector.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */
#include "../../src/adapter/DataConnector.h"
#include "mongodbconnector.h"
//replace this code with actual code
#include <iostream>

void MongoDBConnector::init(std::string dbname, std::string username,
		std::string password) {
	std::cout << "init dbname: " << dbname << " username: " << username
			<< " password: " << password;
}

// illustrative code..
void* MongoDBConnector::runListener(void * engineCallBack) {
	// connect to db
	bool stop = false;
	DataConnector* connector = (DataConnector*) engineCallBack;
	std::string dbname = "mongodb";
	std::string username = "dummy";
	std::string password = "dummy";
	//connector->init(dbname, username, password);
	while (!stop) {
		//get record and parse into json format
		connector->insertRecord();
		connector->deleteRecord();
		connector->updateRecord();
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
