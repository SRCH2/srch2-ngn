#ifndef __MONGODBCONNECTOR_H__
#define __MONGODBCONNECTOR_H__

#include "../../src/adapter/DataConnector.h"
#include "mongo/client/dbclient.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/client/dbclientinterface.h"
#include "util/Logger.h"

class MongoDBConnector : public DataConnector {
public:
		MongoDBConnector();
        virtual ~MongoDBConnector();
        virtual void init(ServerInterface *serverHandle);
        virtual void* runListener();

        bool getOplogConnector();
private:
	ServerInterface *serverHandle;

};



#endif //__MONGODBCONNECTOR_H__
