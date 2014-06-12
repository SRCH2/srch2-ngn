/*
 * DataConnectorFactory.h
 *
 *  Created on: Jun 10, 2014
 *      Author: liusrch2
 */

#ifndef DATACONNECTORFACTORY_H_
#define DATACONNECTORFACTORY_H_

#include <string>
#include "DataConnector.h"
struct ThreadArguments {
	std::string dbType;
	ServerInterface *server;
};

void * spawnConnector(void *arg);

class DataConnectorFactory{
public:
	static void bootStrapConnector(std::string dbType, ServerInterface *server);
private:
	static DataConnector* getDataConnector(std::string dbType);
	static const  std::string DB_CONNECTORS_PATH;
	static const  std::string DYNAMIC_LIBRARY_SUFFIX;
	static const  std::string DYNAMIC_LIBRARY_PREFIX;
};



#endif /* DATACONNECTORFACTORY_H_ */
