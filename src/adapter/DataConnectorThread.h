/*
 * DataConnectorThread.h
 *
 *  Created on: Jun 24, 2014
 *      Author: liusrch2
 */

#ifndef DATACONNECTORTHREAD_H_
#define DATACONNECTORTHREAD_H_

#include <string>
#include "DataConnector.h"
#include "ServerInterfaceInternal.h"

//Arguments passed to the thread
struct ThreadArguments {
	srch2::httpwrapper::DataSourceType dbType;
	ServerInterface* server;
};

void * spawnConnector(void *arg);	//Called by the pthread_create, create the database connector

class DataConnectorThread {
public:
	static void getDataConnectorThread(		//Create thread if interface built successfully.
			srch2::httpwrapper::DataSourceType dbType, void * server);
	static void bootStrapConnector(srch2::httpwrapper::DataSourceType dbType,	//The main function run by the thread, get connector and start listener.
			ServerInterface* server);
private:
	static bool checkIndexExistence(void * server);
	static DataConnector * getDataConnector(void * pdlHandle,	//Get the pointer and handle to the specific connector in shared library.
			std::string dbname);
};

#endif /* DATACONNECTORTHREAD_H_ */
