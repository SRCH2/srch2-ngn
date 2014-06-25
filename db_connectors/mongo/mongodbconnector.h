#ifndef __MONGODBCONNECTOR_H__
#define __MONGODBCONNECTOR_H__


#include <cstdlib>
#include <iostream>
#include <string>
#include "DataConnector.h"
#include "mongo/client/dbclient.h"

class MongoDBConnector: public DataConnector {
public:
	MongoDBConnector();
	virtual ~MongoDBConnector();
	virtual bool init(ServerInterface *serverHandle);	//Init the connector, call connect
	virtual void* runListener();	//Listener the oplog and do modification to the engine
	virtual void createNewIndexes();	//Load the table records and insert into the engine
private:
	ServerInterface *serverHandle;
	mongo::DBClientBase* oplogConnection;
	mongo::ScopedDbConnection * mongoConnector;

	bool connectToDB();	//Connect to the mongodb
	time_t getLastExecutedLogTime();	//Load the last time last oplog record executed
	void saveLastExecutedLogTime(time_t t);	//Save the time last oplog record executed
	void parseOpLogObject(mongo::BSONObj& bobj, std::string currentNS,
			mongo::DBClientBase& oplogConnection);	//Parse the record into json format and do the corresponding operation
}
;



#endif //__MONGODBCONNECTOR_H__
