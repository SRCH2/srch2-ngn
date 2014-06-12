/*
 * DataConnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef __DATACONNECTOR_H__
#define __DATACONNECTOR_H__

#include <string>
class ServerInterface {
public:
	int insertRecord(std::string jsonString);
	int deleteRecord(std::string primaryKey);
	int updateRecord(std::string jsonSrting);
        // this API will provide key based lookup
        // from engine's connector specific configuration store. 
        //  e.g  "dbname" => "mysql"  (single value)
        //       "collections" => "collection1, collection2 " (multi value) 
	std::string configLookUp(std::string key);
};

class DataConnector {
public:

	virtual ~DataConnector() = 0;
	virtual void init(ServerInterface *serverHandle) = 0;
	virtual void* runListener() = 0;

};

#endif /* __DATACONNECTOR_H__ */
