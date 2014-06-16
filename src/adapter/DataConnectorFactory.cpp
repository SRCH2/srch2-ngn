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
	DataConnectorFactory::bootStrapConnector((ThreadArguments *)arg);
	return NULL;
}

const std::string DataConnectorFactory::DB_CONNECTORS_PATH =
		"/home/liusrch2/srch2-ngn/db_connectors/build/";
const std::string DataConnectorFactory::DYNAMIC_LIBRARY_SUFFIX = "Connector.so";
const std::string DataConnectorFactory::DYNAMIC_LIBRARY_PREFIX = "lib";
const std::string DataConnectorFactory::PRIMARY_KEY="uniqueKey";
const std::string DataConnectorFactory::DATABASE_NAME="db";
const std::string DataConnectorFactory::DATABASE_PORT="port";
const std::string DataConnectorFactory::DATABASE_HOST="host";
const std::string DataConnectorFactory::DATABASE_COLLECTION="collection";
const std::string DataConnectorFactory::DATABASE_TYPE_NAME="dbTypeName";
const std::string DataConnectorFactory::DATABASE_LISTENER_WATI_TIME="listenerWaitTime";
const std::string DataConnectorFactory::DATABASE_MAX_RETRY_ON_FALIFURE="maxRetryOnFailure";
const std::string DataConnectorFactory::DATABASE_MAX_RETRY_COUNT="maxRetryCount";

void DataConnectorFactory::bootStrapConnector(ThreadArguments * targ) {
	DataConnector *connector = getDataConnector(targ->server->configLookUp(DataConnectorFactory::DATABASE_TYPE_NAME));

	connector->init(targ->server);
	connector->runListener();
}

DataConnector* DataConnectorFactory::getDataConnector(std::string dbname) {
	std::string libName = DB_CONNECTORS_PATH + DYNAMIC_LIBRARY_PREFIX + dbname
			+ DYNAMIC_LIBRARY_SUFFIX;
	void *pdlHandle = dlopen(libName.c_str(), RTLD_LAZY);
	if (!pdlHandle) {
		std::cout << "Fail to load " << libName << " due to " << dlerror()
				<< std::endl;
		return 0;
	}

	create_t* create_dataConnector = (create_t*) dlsym(pdlHandle, "create");
	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		std::cout << "Cannot load symbol create: " << dlsym_error << '\n';
		return 0;
	}
	DataConnector *dc = create_dataConnector();

	return dc;
}

ServerInterfaceInternal* DataConnectorFactory::getServerInterfaceInternal(
		srch2::httpwrapper::DataSourceType dbType, void * server) {
	std::map<std::string, std::string> * connectorConfig =
			populateConnectorConfig(dbType, server);

	ServerInterfaceInternal* ret = new ServerInterfaceInternal(server,
			connectorConfig);
	return ret;
}

std::map<std::string, std::string> * DataConnectorFactory::populateConnectorConfig(
		srch2::httpwrapper::DataSourceType dbType, void * server) {
	std::map<std::string, std::string> * config = new std::map<std::string,
			std::string>();

	srch2::httpwrapper::Srch2Server *srch2Server =
			(srch2::httpwrapper::Srch2Server *) server;

	(*config)[PRIMARY_KEY]=srch2Server->indexDataConfig->getPrimaryKey();

	switch (dbType) {
	case srch2::httpwrapper::DATA_SOURCE_NOT_SPECIFIED:
		break;
	case srch2::httpwrapper::DATA_SOURCE_JSON_FILE:
		break;
	case srch2::httpwrapper::DATA_SOURCE_MONGO_DB:
		(*config)[DATABASE_TYPE_NAME]="mongodb";
		(*config)[DATABASE_NAME]=srch2Server->indexDataConfig->getMongoDbName();
		(*config)[DATABASE_HOST]=srch2Server->indexDataConfig->getMongoServerHost();
		(*config)[DATABASE_PORT]=srch2Server->indexDataConfig->getMongoServerPort();
		(*config)[DATABASE_COLLECTION]=srch2Server->indexDataConfig->getMongoCollection();
		(*config)[DATABASE_LISTENER_WATI_TIME]=srch2Server->indexDataConfig->getMongoListenerWaitTime();
		(*config)[DATABASE_MAX_RETRY_ON_FALIFURE]=srch2Server->indexDataConfig->getMongoListenerMaxRetryOnFailure();
		(*config)[DATABASE_MAX_RETRY_COUNT]=srch2Server->indexDataConfig->getMongoListenerMaxRetryCount();
		break;
	default:
		break;
	}

	return config;
}
