/*
 * DataConnector.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#include "DataConnector.h"

DataConnector::DataConnector(void * server) {
	this->dataConnectorInternal = new DataConnectorInternal(server);
}

void DataConnector::insertRecord() {
	this->dataConnectorInternal->insertRecord();
}

void DataConnector::deleteRecord() {
	this->dataConnectorInternal->deleteRecord();
}

void DataConnector::updateRecord() {
	this->dataConnectorInternal->updateRecord();
}

void DataConnector::spawnUpdateListener(){
	//PRunListener pRunListener=&DataConnector::runListener;
	void* (DataConnector::*pRunListener)(void*)=&DataConnector::runListener;

	this->dataConnectorInternal->spawnUpdateListener(pRunListener);
}
