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
#include "ServerInterfaceInternal.h"

struct ThreadArguments {
	srch2::httpwrapper::DataSourceType dbType;
	ServerInterface* server;
};

void * spawnConnector(void *arg);

class DataConnectorFactory {
public:
	static void bootStrapConnector(ThreadArguments* args);
	static ServerInterfaceInternal* getServerInterfaceInternal(
			srch2::httpwrapper::DataSourceType dbType, void * server);

	static const std::string DB_CONNECTORS_PATH;
	static const std::string DYNAMIC_LIBRARY_SUFFIX;
	static const std::string DYNAMIC_LIBRARY_PREFIX;
	static const std::string PRIMARY_KEY;
	static const std::string DATABASE_NAME;
	static const std::string DATABASE_PORT;
	static const std::string DATABASE_HOST;
	static const std::string DATABASE_COLLECTION;
	static const std::string DATABASE_TYPE_NAME;
	static const std::string DATABASE_LISTENER_WATI_TIME;
	static const std::string DATABASE_MAX_RETRY_ON_FALIFURE;
	static const std::string DATABASE_MAX_RETRY_COUNT;
private:
	static DataConnector* getDataConnector(std::string dbname);

	static std::map<std::string, std::string> * populateConnectorConfig(
			srch2::httpwrapper::DataSourceType dbType, void * server);

};

#endif /* DATACONNECTORFACTORY_H_ */
