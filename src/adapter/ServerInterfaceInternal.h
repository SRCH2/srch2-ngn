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
using namespace std;
class Srch2Server;

class ServerInterfaceInternal : public ServerInterface {

        ServerInterfaceInternal(Srch2Server *server, std::map<string, string> * connectorConfig);
	int insertRecord(string jsonString);
	int deleteRecord(string primaryKey);
	int updateRecord(string jsonSrting);
        // this API will provide key based lookup
        // from engine's connector specific configuration store. 
        //  e.g  "dbname" => "mysql"  (single value)
        //       "collections" => "collection1, collection2 " (multi value) 
        string configLookUp(string key);
private:
	Srch2Server *server;
	std::map<string, string> * connectorConfig;        
};

#endif /* __SERVERINTERFACEINTERNAL__ */
