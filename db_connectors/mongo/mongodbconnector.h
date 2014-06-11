#ifndef __MONGODBCONNECTOR_H__
#define __MONGODBCONNECTOR_H__

#include "DataConnector.h"

class MongoDBConnector : public DataConnector {
public:
        virtual ~MongoDBConnector(); 
        virtual void init(ServerInterface *serverHandle);
        virtual void* runListener();
private:
	ServerInterface *serverHandle;

};

#endif //__MONGODBCONNECTOR_H__
