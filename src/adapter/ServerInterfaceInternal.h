/*
 * DataConnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef __SERVERINTERFACEINTERNAL__
#define __SERVERINTERFACEINTERNAL__ 
#include "DataConnector.h"
#include <string>

namespace adapter{
class Srch2Server;

class ServerInterfaceInternal : public ServerInterface {

public:
        ServerInterfaceInternal(void *server, std::map<std::string, std::string> * connectorConfig);
	int insertRecord(std::string jsonString);
	int deleteRecord(std::string primaryKey);
	int updateRecord(std::string jsonSrting);
        // this API will provide key based lookup
        // from engine's connector specific configuration store. 
        //  e.g  "dbname" => "mysql"  (single value)
        //       "collections" => "collection1, collection2 " (multi value) 
	std::string configLookUp(std::string key);
private:
	Srch2Server *server;
	std::map<std::string, std::string> * connectorConfig;
};
}
#endif /* __SERVERINTERFACEINTERNAL__ */
