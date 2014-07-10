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
    virtual ~ServerInterface() {
    }
    ;
    virtual int insertRecord(const std::string& jsonString) = 0;
    virtual int deleteRecord(const std::string& primaryKey) = 0;
    virtual int updateRecord(const std::string& pk, const std::string& jsonString) = 0;
    virtual void saveChanges() = 0;	//Save changes to the disk

    /*
     * "configLookUp" will provide key based lookup from engine's connector
     *  specific configuration store. e.g  "dbname" => "mysql"  (single value)
     *  "collections" => "collection1, collection2 " (multi value)
     */
    virtual bool configLookUp(const std::string& key,std::string & value) = 0;
};

class DataConnector {
public:
    virtual ~DataConnector() {
    }
    ;
    virtual bool init(ServerInterface *serverHandle) = 0;
    virtual void* runListener() = 0;	//Periodically fetch the data form log
    virtual void createNewIndexes() = 0;	//Create New Indexes from the table.
};

/*
 * Used by the shared library mechanism to get the connector by the engine.
 * The engine will call create() to get the specific connector and call
 * destroy to delete it.
 */
typedef DataConnector* create_t();
typedef void destroy_t(DataConnector*);

#endif /* __DATACONNECTOR_H__ */
