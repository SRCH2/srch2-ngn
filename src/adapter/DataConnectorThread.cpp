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

//Called by the pthread_create, create the database connector
void * spawnConnector(void *arg) {
	ThreadArguments * targ = (ThreadArguments *) arg;
	DataConnectorThread::bootStrapConnector(targ->dbType, targ->server);

	delete targ->server;
	delete targ;

	return NULL;
}

//The main function run by the thread, get connector and start listener.
void DataConnectorThread::bootStrapConnector(
		srch2::httpwrapper::DataSourceType dbType, ServerInterface* server) {
	void * pdlHandle;
	DataConnector *connector = getDataConnector(pdlHandle,	//Get the pointer of the specific library
			server->configLookUp(ServerInterfaceInternal::DATABASE_TYPE_NAME),server->configLookUp(ServerInterfaceInternal::SRCH2HOME));

	if (connector == NULL)
		return;

	if (connector->init(server)) {
		if (!checkIndexExistence((void*) server)) {
			Logger::debug("Create Indices from empty");
			connector->createNewIndexes();
		}
		connector->runListener();
	}

	//After the listener;
	destroy_t* destory_dataConnector = (destroy_t*) dlsym(pdlHandle, "destroy");
	destory_dataConnector(connector);
	dlclose(pdlHandle);

}

//Create thread if interface built successfully.
void DataConnectorThread::getDataConnectorThread(
		srch2::httpwrapper::DataSourceType dbType, void * server) {
	pthread_t tid;
	ThreadArguments * dbArg = new ThreadArguments();
	ServerInterfaceInternal * internal = new ServerInterfaceInternal(server);

	if (!internal->isBuildSuccess()) {
		delete dbArg;
		delete internal;
	} else {
		dbArg->dbType = dbType;
		dbArg->server = internal;

		int res = pthread_create(&tid, NULL, spawnConnector, (void *) dbArg);
	}
}

//Get the pointer and handle to the specific connector in shared library.
DataConnector * DataConnectorThread::getDataConnector(void * pdlHandle,
		std::string dbname, std::string srch2homePath) {
		std::string libName = srch2homePath + "/"
			+ ServerInterfaceInternal::DB_CONNECTORS_PATH + "/"
			+ ServerInterfaceInternal::DYNAMIC_LIBRARY_PREFIX + dbname
			+ ServerInterfaceInternal::DYNAMIC_LIBRARY_SUFFIX;
	pdlHandle = dlopen(libName.c_str(), RTLD_LAZY);	//Open the shared library.
	if (!pdlHandle) {
		Logger::error("Fail to load %c due to %c", libName.c_str(), dlerror());
		return NULL;
	}

	create_t* create_dataConnector = (create_t*) dlsym(pdlHandle, "create");	//Get the function "create" in the shared library.

	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		Logger::error("Cannot load symbol create: %c", dlsym_error);
		return NULL;
	}

	DataConnector * connector = create_dataConnector();	//Call the "create" func in the shared library.

	return connector;
}

bool DataConnectorThread::checkIndexExistence(void * server) {
	ServerInterfaceInternal* serverInterface = (ServerInterfaceInternal*) server;

	string directoryName = serverInterface->configLookUp(
			ServerInterfaceInternal::INDEXPATH);
	srch2::instantsearch::IndexType it =
			(srch2::instantsearch::IndexType) (atoi(
					serverInterface->configLookUp(
							ServerInterfaceInternal::INDEXTYPE).c_str()));

	if (!checkDirExistence(
			(directoryName + "/" + IndexConfig::analyzerFileName).c_str()))
		return false;
	if (!checkDirExistence(
			(directoryName + "/" + IndexConfig::trieFileName).c_str()))
		return false;
	if (!checkDirExistence(
			(directoryName + "/" + IndexConfig::forwardIndexFileName).c_str()))
		return false;
	if (!checkDirExistence(
			(directoryName + "/" + IndexConfig::schemaFileName).c_str()))
		return false;
	if (it == srch2::instantsearch::DefaultIndex) {
		// Check existence of the inverted index file for basic keyword search ("A1")
		if (!checkDirExistence(
				(directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
			return false;
	} else {
		// Check existence of the quadtree index file for geo keyword search ("M1")
		if (!checkDirExistence(
				(directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
			return false;
	}
	return true;
}
