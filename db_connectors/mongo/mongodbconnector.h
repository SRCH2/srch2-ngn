#ifndef __MONGODBCONNECTOR_H__
#define __MONGODBCONNECTOR_H__


#include <cstdlib>
#include <iostream>
#include <string>
#include "../../src/adapter/DataConnector.h"
#include "mongo/client/dbclient.h"

class MongoDBConnector: public DataConnector {
public:
	MongoDBConnector();
	virtual ~MongoDBConnector();
	virtual bool init(ServerInterface *serverHandle);
	virtual void* runListener();
	void parseOpLogObject(mongo::BSONObj& bobj, std::string currentNS,
			mongo::DBClientBase& oplogConnection);
	static time_t bulkLoadEndTime;
	virtual void createNewIndexes();
private:
	ServerInterface *serverHandle;
	mongo::DBClientBase* oplogConnection;
	mongo::ScopedDbConnection * mongoConnector;
	bool conn();
	time_t getLastExecutedLogTime();
	void saveLastExecutedLogTime(time_t t);
}
;



#endif //__MONGODBCONNECTOR_H__
