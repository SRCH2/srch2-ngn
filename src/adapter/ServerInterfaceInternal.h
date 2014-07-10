/*
 * ServerInterfaceInternal.h
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

    virtual int insertRecord(const std::string& jsonString);
    virtual int deleteRecord(const std::string& primaryKey);
    virtual int updateRecord(const std::string& pk, const std::string& jsonSrting);
    virtual void saveChanges();

    /*
     * "configLookUp" will provide key based lookup from engine's connector
     *  specific configuration store. e.g  "dbname" => "mysql"  (single value)
     *  "collections" => "collection1, collection2 " (multi value)
     */
    virtual bool configLookUp(const std::string& key,std::string & value);

    //return false if the source is not database
    bool isDatabase();
private:
    srch2::httpwrapper::Srch2Server *server;
};
#endif /* __SERVERINTERFACEINTERNAL__ */
