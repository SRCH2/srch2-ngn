/*
 * DataConnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef __DATACONNECTOR_H__
#define __DATACONNECTOR_H__

#include <string>
using namespace std;
class ServerInterface {
public:
	int insertRecord(string jsonString);
	int deleteRecord(string primaryKey);
	int updateRecord(string jsonSrting);
        // this API will provide key based lookup
        // from engine's connector specific configuration store. 
        //  e.g  "dbname" => "mysql"  (single value)
        //       "collections" => "collection1, collection2 " (multi value) 
        string configLookUp(string key);
};

class DataConnector {
public:

	virtual ~DataConnector() = 0;
	virtual void init(ServerInterface *serverHandle) = 0;
	virtual void* runListener() = 0;

};

#endif /* __DATACONNECTOR_H__ */
