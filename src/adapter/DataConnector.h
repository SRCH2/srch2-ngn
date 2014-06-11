/*
 * DataConnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef DATACONNECTOR_H_
#define DATACONNECTOR_H_

#include <string>
#include "DataConnectorInternal.h"
class DataConnectorInternal;


class DataConnector {
public:
	//DataConnector();
	DataConnector(void * server);

	virtual ~DataConnector() = 0;

	virtual void init(std::string dbname, std::string username, std::string password) = 0;
	virtual void* runListener(void * engineCallBack) = 0;

	void spawnUpdateListener();

	void insertRecord();
	void deleteRecord();
	void updateRecord();
private:
	DataConnectorInternal *dataConnectorInternal;
};

#endif /* DATACONNECTOR_H_ */
