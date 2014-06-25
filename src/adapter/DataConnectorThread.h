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

struct ThreadArguments {
	srch2::httpwrapper::DataSourceType dbType;
	ServerInterface* server;
};

void * spawnConnector(void *arg);

class DataConnectorThread {
public:
	static void getDataConnectorThread(srch2::httpwrapper::DataSourceType dbType, void * server);
	static void bootStrapConnector(srch2::httpwrapper::DataSourceType dbType,	ServerInterface* server);
private:
	static bool checkIndexExistence(void * server);
	static DataConnector * getDataConnector(void * pdlHandle,std::string dbname);
};



#endif /* DATACONNECTORTHREAD_H_ */
