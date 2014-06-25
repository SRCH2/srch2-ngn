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
	void create();
	void bootStrapConnector();

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
	static const std::string SRCH2HOME;
	static const std::string INDEXTYPE;

	DataConnectorThread(srch2::httpwrapper::DataSourceType dbType, void * server);
	~DataConnectorThread();
private:
	bool checkIndexExistence(void * server);
	void populateConnectorConfig(srch2::httpwrapper::DataSourceType dbType,
			void * server);
	void populateConnector(std::string dbname);

	pthread_t * dbListenerThread;
	ThreadArguments * dbArg;
	std::map<std::string, std::string> * config;
	DataConnector *connector;
	void *pdlHandle;
};



#endif /* DATACONNECTORTHREAD_H_ */
