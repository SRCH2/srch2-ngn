/*
 * DataConnectorInternal.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef DATACONNECTORINTERNAL_H_
#define DATACONNECTORINTERNAL_H_

#include "DataConnector.h"
class DataConnector;

typedef void* (*PRunListener)(void*);

class DataConnectorInternal {
public:
	DataConnectorInternal();
	DataConnectorInternal(void* server);

	~DataConnectorInternal();

	void spawnUpdateListener(void* (DataConnector::*pRunListener)(void*));

	void insertRecord();
	void deleteRecord();
	void updateRecord();
private:
	void* server; //include: CoreInfo_t *config = server->indexDataConfig;
				  //srch2is::Record *record = new srch2is::Record(server->indexer->getSchema());
				  //server->indexer
	static pthread_t * listenerThread;
};

#endif /* DATACONNECTORINTERNAL_H_ */
