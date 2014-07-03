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
	void * pdlHandle = NULL;
	DataConnector *connector = getDataConnector(pdlHandle,	//Get the pointer of the specific library
			server->configLookUp(ServerInterfaceInternal::DATABASE_SHARED_LIBRARY_PATH));

	if (connector == NULL) {
		Logger::error(
				"Can not open the shared library. Either the shared library is not found or the engine is built in static mode, exit.");
		exit(1);	//Exit if can not open the shared library
	}


	if (connector->init(server)) {
		if (!checkIndexExistence((void*) server)) {
			Logger::debug("Create Indices from empty");
			connector->createNewIndexes();
		}
		connector->runListener();
	}

	//After the listener;

//Supressing specific warnings on gcc/g++. http://www.mr-edd.co.uk/blog/supressing_gcc_warnings
//The warnings g++ spat out is to do with an invalid cast from a pointer-to-object to a pointer-to-function before gcc 4.
#ifdef __GNUC__
__extension__
#endif
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
		std::string sharedLibraryPath) {
#ifndef BUILD_STATIC
	std::string libName = sharedLibraryPath;

	pdlHandle = dlopen(libName.c_str(), RTLD_LAZY);	//Open the shared library.

	if (!pdlHandle) {
		Logger::error("Fail to load shared library %c due to %c", libName.c_str(), dlerror());
		return NULL;
	}

//Supressing specific warnings on gcc/g++. http://www.mr-edd.co.uk/blog/supressing_gcc_warnings
//The warnings g++ spat out is to do with an invalid cast from a pointer-to-object to a pointer-to-function before gcc 4.
#ifdef __GNUC__
__extension__
#endif
	create_t* create_dataConnector = (create_t*) dlsym(pdlHandle, "create");	//Get the function "create" in the shared library.

	const char* dlsym_error = dlerror();
	if (dlsym_error) {
		Logger::error("Cannot load symbol \"create\" in shared library due to: %c", dlsym_error);
		return NULL;
	}

	DataConnector * connector = create_dataConnector();	//Call the "create" func in the shared library.

	return connector;
#endif
	return NULL;
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
