/*
 * DataConnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef __DATACONNECTOR_H__
#define __DATACONNECTOR_H__

//#include <string>
//
//class ServerInterface {
//public:
//	virtual ~ServerInterface() = 0;
//	virtual int insertRecord(std::string jsonString) = 0;
//	virtual int deleteRecord(std::string primaryKey) = 0;
//	virtual int updateRecord(std::string jsonSrting) = 0;
//	// this API will provide key based lookup
//	// from engine's connector specific configuration store.
//	//  e.g  "dbname" => "mysql"  (single value)
//	//       "collections" => "collection1, collection2 " (multi value)
//	virtual std::string configLookUp(std::string key) = 0;
//
//};

#include "ServerInterface.h"

class DataConnector {
public:

	virtual ~DataConnector() = 0;
	virtual void init(ServerInterface *serverHandle) = 0;
	virtual void* runListener() = 0;

};

// the types of the class factories
typedef DataConnector* create_t();
typedef void destroy_t(DataConnector*);

#endif /* __DATACONNECTOR_H__ */
