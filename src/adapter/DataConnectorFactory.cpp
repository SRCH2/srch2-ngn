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
	void *pdlHandle = dlopen(libName.c_str(), RTLD_LAZY);
	if (!pdlHandle) {
		std::cout << "Fail to load " << libName << " due to " << dlerror()
				<< std::endl;
		return 0;
	}

	create_t* create_dataConnector = (create_t*)dlsym(pdlHandle,"create");
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		std::cout << "Cannot load symbol create: " << dlsym_error << '\n';
		return 0;
	}
	DataConnector *dc=create_dataConnector();


	return dc;
}

