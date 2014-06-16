#ifndef __MONGODBCONNECTOR_H__
#define __MONGODBCONNECTOR_H__


#include <cstdlib>
#include <iostream>
#include <string>
#include "../../src/adapter/DataConnector.h"
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/client/dbclientinterface.h"

class MongoDBConnector: public DataConnector {
public:
	MongoDBConnector();
	virtual ~MongoDBConnector();
	virtual void init(ServerInterface *serverHandle);
	virtual void* runListener();
	void parseOpLogObject(mongo::BSONObj& bobj, std::string currentNS,
			mongo::DBClientBase& oplogConnection);
private:
	ServerInterface *serverHandle;

}
;



#endif //__MONGODBCONNECTOR_H__
