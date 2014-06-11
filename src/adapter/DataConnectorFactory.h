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

class DataConnectorFactory{
public:
	DataConnector* getDataConnector(std::string dbType);
private:
	const  std::string DB_CONNECTORS_PATH="db_connectors/";
	const  std::string DYNAMIC_LIBRARY_SUFFIX="Connector.so";
	const  std::string DYNAMIC_LIBRARY_PREFIX="lib";
};



#endif /* DATACONNECTORFACTORY_H_ */
