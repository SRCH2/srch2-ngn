/*
 * DataConnectorInternal.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

//#include "Srch2Server.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include "DataConnectorInternal.h"

pthread_t * DataConnectorInternal::listenerThread = new pthread_t;

DataConnectorInternal::DataConnectorInternal() {
	this->server=NULL;
}

void DataConnectorInternal::insertRecord() {
	std::cout << "DataConnectorInternal::insertRecord()" << std::endl;
}

void DataConnectorInternal::deleteRecord() {
	std::cout << "DataConnectorInternal::deleteRecord()" << std::endl;
}

void DataConnectorInternal::updateRecord() {
	std::cout << "DataConnectorInternal::updateRecord()" << std::endl;
}

void DataConnectorInternal::spawnUpdateListener(
		void* (*pRunListener)(void*)) {
	int res = pthread_create(DataConnectorInternal::listenerThread, NULL,
			DataConnector::pRunListener, (void *) server);

	if (res != 0)
		std::cout << "DataConnectorInternal::spawnUpdateListener()" << std::endl;
}
