#ifndef __MONGODBCONNECTOR_H__
#define __MONGODBCONNECTOR_H__


#include <cstdlib>
#include <iostream>
#include <string>
#include "../../src/adapter/DataConnector.h"
#include "mongo/client/dbclient.h"
//#include "mongo/bson/bsonobj.h"
//#include "mongo/client/dbclientcursor.h"
//#include "mongo/client/dbclientinterface.h"
//#include "../../tmp/mongo-cxx-driver/build/linux2/use-system-boost/chen/include/mongo/client/dbclient.h"

//#include "mongo/bson/bsonobj.h"
//#include "../../tmp/mongo-cxx-driver/build/linux2/use-system-boost/chen/include/mongo/client/dbclientcursor.h"
//#include "../../tmp/mongo-cxx-driver/build/linux2/use-system-boost/chen/include/mongo/client/dbclientinterface.h"

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
}
;



#endif //__MONGODBCONNECTOR_H__
