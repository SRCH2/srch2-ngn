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
	ServerInterfaceInternal(void *server);
	virtual ~ServerInterfaceInternal();
	virtual int insertRecord(std::string jsonString);
	virtual int deleteRecord(std::string primaryKey);
	virtual int updateRecord(std::string pk,std::string jsonSrting);
	// this API will provide key based lookup
	// from engine's connector specific configuration store.
	//  e.g  "dbname" => "mysql"  (single value)
	//       "collections" => "collection1, collection2 " (multi value)
	virtual std::string configLookUp(std::string key);

	static const std::string DB_CONNECTORS_PATH;
	static const std::string DYNAMIC_LIBRARY_SUFFIX;
	static const std::string DYNAMIC_LIBRARY_PREFIX;
	static const std::string PRIMARY_KEY;
	static const std::string DATABASE_NAME;
	static const std::string DATABASE_PORT;
	static const std::string DATABASE_HOST;
	static const std::string DATABASE_COLLECTION;
	static const std::string DATABASE_TYPE_NAME;
	static const std::string DATABASE_LISTENER_WATI_TIME;
	static const std::string DATABASE_MAX_RETRY_ON_FALIFURE;
	static const std::string DATABASE_MAX_RETRY_COUNT;
	static const std::string SRCH2HOME;
	static const std::string INDEXTYPE;
private:
	void populateConnectorConfig();
	srch2::httpwrapper::Srch2Server *server;
	std::map<std::string, std::string> * connectorConfig;


};
#endif /* __SERVERINTERFACEINTERNAL__ */
