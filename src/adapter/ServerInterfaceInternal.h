/*
 * DataConnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef __SERVERINTERFACEINTERNAL__
#define __SERVERINTERFACEINTERNAL__ 
#include "DataConnector.h"
#include "Srch2Server.h"
#include <string>

class ServerInterfaceInternal: public ServerInterface {

public:
	ServerInterfaceInternal(void *server,
			std::map<std::string, std::string> * connectorConfig);
	virtual ~ServerInterfaceInternal();
	virtual int insertRecord(std::string jsonString);
	virtual int deleteRecord(std::string primaryKey);
	virtual int updateRecord(std::string jsonSrting);
	// this API will provide key based lookup
	// from engine's connector specific configuration store.
	//  e.g  "dbname" => "mysql"  (single value)
	//       "collections" => "collection1, collection2 " (multi value)
	virtual std::string configLookUp(std::string key);
private:
	srch2::httpwrapper::Srch2Server *server;
	std::map<std::string, std::string> * connectorConfig;


};
#endif /* __SERVERINTERFACEINTERNAL__ */
