/*
 * DataConnectorThread.cpp
 *
 *  Created on: Jun 24, 2014
 *      Author: liusrch2
 */




#include <dlfcn.h>
#include "DataConnectorThread.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <cstdlib>
#include "DataConnector.h"

void * spawnConnector(void *arg) {
	DataConnectorThread * dcf= (DataConnectorThread*)arg;
	dcf->bootStrapConnector();
	return NULL;
}

const std::string DataConnectorThread::DB_CONNECTORS_PATH =
//		"/home/liusrch2/srch2-ngn/db_connectors/mysql/mysql-replication-listener/build/lib/";
		"/home/liusrch2/srch2-ngn/db_connectors/build/";
const std::string DataConnectorThread::DYNAMIC_LIBRARY_SUFFIX = "Connector.so";
const std::string DataConnectorThread::DYNAMIC_LIBRARY_PREFIX = "lib";
const std::string DataConnectorThread::PRIMARY_KEY="uniqueKey";
const std::string DataConnectorThread::DATABASE_NAME="db";
const std::string DataConnectorThread::DATABASE_PORT="port";
const std::string DataConnectorThread::DATABASE_HOST="host";
const std::string DataConnectorThread::DATABASE_COLLECTION="collection";
const std::string DataConnectorThread::DATABASE_TYPE_NAME="dbTypeName";
const std::string DataConnectorThread::DATABASE_LISTENER_WATI_TIME="listenerWaitTime";
const std::string DataConnectorThread::DATABASE_MAX_RETRY_ON_FALIFURE="maxRetryOnFailure";
const std::string DataConnectorThread::DATABASE_MAX_RETRY_COUNT="maxRetryCount";
const std::string DataConnectorThread::SRCH2HOME="srch2Home";
const std::string DataConnectorThread::INDEXTYPE="indexType";

DataConnectorThread::DataConnectorThread(srch2::httpwrapper::DataSourceType dbType, void * server) {
	dbListenerThread = new pthread_t;
	dbArg = new ThreadArguments();

	config = new std::map<std::string, std::string>();
	populateConnectorConfig(dbType, server);

	dbArg->dbType=dbType;
	dbArg->server = new ServerInterfaceInternal(server, config);
}

DataConnectorThread::~DataConnectorThread(){
	delete dbArg->server;
	delete config;
	delete dbArg;
	delete dbListenerThread;

	destroy_t* destory_dataConnector = (destroy_t*) dlsym(pdlHandle,"destroy");
	destory_dataConnector(connector);
	dlclose(pdlHandle);
}

void DataConnectorThread::create(){
	int res = pthread_create(dbListenerThread, NULL, spawnConnector,
			(void *) this);
}


void DataConnectorThread::bootStrapConnector() {
	populateConnector(this->dbArg->server->configLookUp(DataConnectorThread::DATABASE_TYPE_NAME));

	if (connector->init(dbArg->server)) {
		if (!checkIndexExistence((void*) dbArg->server)) {
			Logger::debug("Create Indices from empty");
			connector->createNewIndexes();
		}
		connector->runListener();
	}
}

void DataConnectorThread::populateConnector(std::string dbname) {
	std::string libName = DB_CONNECTORS_PATH + DYNAMIC_LIBRARY_PREFIX
			+ "mongodb" + DYNAMIC_LIBRARY_SUFFIX;
	pdlHandle = dlopen(libName.c_str(), RTLD_LAZY);
	if (!pdlHandle) {
		Logger::error("Fail to load %c due to %c", libName.c_str(), dlerror());
		return;
	}

	create_t* create_dataConnector = (create_t*) dlsym(pdlHandle, "create");

	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		Logger::error("Cannot load symbol create: %c", dlsym_error);
		return;
	}

	connector = create_dataConnector();
}


void DataConnectorThread::populateConnectorConfig(
		srch2::httpwrapper::DataSourceType dbType, void * server) {

	srch2::httpwrapper::Srch2Server *srch2Server =
			(srch2::httpwrapper::Srch2Server *) server;

	(*config)[PRIMARY_KEY]=srch2Server->indexDataConfig->getPrimaryKey();

	switch (dbType) {
	case srch2::httpwrapper::DATA_SOURCE_NOT_SPECIFIED:
		break;
	case srch2::httpwrapper::DATA_SOURCE_JSON_FILE:
		break;
	case srch2::httpwrapper::DATA_SOURCE_MONGO_DB:{
		(*config)[DATABASE_TYPE_NAME]="mongodb";
		(*config)[DATABASE_NAME]=srch2Server->indexDataConfig->getMongoDbName();
		(*config)[DATABASE_HOST]=srch2Server->indexDataConfig->getMongoServerHost();
		(*config)[DATABASE_PORT]=srch2Server->indexDataConfig->getMongoServerPort();
		(*config)[DATABASE_COLLECTION]=srch2Server->indexDataConfig->getMongoCollection();
		(*config)[DATABASE_LISTENER_WATI_TIME]=srch2Server->indexDataConfig->getMongoListenerWaitTime();
		(*config)[DATABASE_MAX_RETRY_ON_FALIFURE]=srch2Server->indexDataConfig->getMongoListenerMaxRetryOnFailure();
		(*config)[DATABASE_MAX_RETRY_COUNT]=srch2Server->indexDataConfig->getMongoListenerMaxRetryCount();
		(*config)[SRCH2HOME]=srch2Server->indexDataConfig->getIndexPath();

		srch2::instantsearch::IndexType it  = (srch2::instantsearch::IndexType)srch2Server->indexDataConfig->getIndexType();
		std::stringstream ss;ss << it;std::string itStr;   ss >> itStr;
		(*config)[INDEXTYPE]=itStr;
	}
		break;
	default:
		break;
	}
}


bool DataConnectorThread::checkIndexExistence(void * server)
{
	ServerInterfaceInternal* serverInterface = (ServerInterfaceInternal*)server;

    string directoryName =serverInterface->configLookUp(SRCH2HOME);
	srch2::instantsearch::IndexType it  = (srch2::instantsearch::IndexType)(atoi(serverInterface->configLookUp(INDEXTYPE).c_str()));

    if(!checkDirExistence((directoryName + "/" + IndexConfig::analyzerFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::trieFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::forwardIndexFileName).c_str()))
        return false;
    if(!checkDirExistence((directoryName + "/" + IndexConfig::schemaFileName).c_str()))
        return false;
    if (it==srch2::instantsearch::DefaultIndex){
        // Check existence of the inverted index file for basic keyword search ("A1")
        if(!checkDirExistence((directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
	    return false;
    }else{
        // Check existence of the quadtree index file for geo keyword search ("M1")
        if(!checkDirExistence((directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
	    return false;
    }
    return true;
}
