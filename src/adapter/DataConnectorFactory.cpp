/*
 * DataConnectorFactory.cpp
 *
 *  Created on: Jun 10, 2014
 *      Author: liusrch2
 */

#include <dlfcn.h>
#include "DataConnectorFactory.h"
#include <iostream>
#include <string>


void * spawnConnector(void *arg) {
      ThreadArguments * targ = (ThreadArguments*) arg;
      std::cout<< "Test:: dbType " <<targ->dbType <<" server: "<<std::endl;;
      DataConnectorFactory::bootStrapConnector(targ->dbType, targ->server);
}

const  std::string DataConnectorFactory::DB_CONNECTORS_PATH="/home/liusrch2/srch2-ngn/db_connectors/Build/";
const  std::string DataConnectorFactory::DYNAMIC_LIBRARY_SUFFIX="Connector.so";
const  std::string DataConnectorFactory::DYNAMIC_LIBRARY_PREFIX="lib";

void DataConnectorFactory::bootStrapConnector(std::string dbType, ServerInterface *server) {
	DataConnector *connector = getDataConnector(dbType);
	connector->init(server);
	connector->runListener();
}

DataConnector* DataConnectorFactory::getDataConnector(
		std::string dbType) {
	std::string libName = DB_CONNECTORS_PATH + DYNAMIC_LIBRARY_PREFIX + dbType
			+ DYNAMIC_LIBRARY_SUFFIX;
	void *pdlHandle = dlopen(libName.c_str(), RTLD_NOW);
	if (!pdlHandle) {
		std::cout << "Fail to load " << libName << " due to " << dlerror()
				<< std::endl;
		return 0;
	}
	DataConnector* getNewInstance = (DataConnector*) dlsym(pdlHandle,
			"getNewInstance");
	if (!getNewInstance) {
		std::cout << "There is no \"getNewInstance\" function in " << libName
				<< std::endl;
		return 0;
	}
	return getNewInstance;
}

