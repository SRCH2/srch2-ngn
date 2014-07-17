/*
 * mongodbconnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

#ifndef __MONGODBCONNECTOR_H__
#define __MONGODBCONNECTOR_H__

#include <cstdlib>
#include <iostream>
#include <string>
#include "DataConnector.h"
#include "mongo/client/dbclient.h"

class MongoDBConnector: public DataConnector {
public:
    MongoDBConnector();
    ~MongoDBConnector();

    //Init the connector, call connect
    virtual int init(ServerInterface *serverHandle);

    //Listen to the oplog and do modification to the engine
    virtual int runListener();

    //Load the table records and insert into the engine
    virtual int createNewIndexes();
private:
    ServerInterface *serverHandle;
    mongo::DBClientBase* oplogConnection;
    mongo::ScopedDbConnection * mongoConnector;

    //Connect to the mongodb
    bool connectToDB();

    //Load the last time last oplog record accessed
    bool getLastAccessedLogRecordTime(time_t& t);

    //Save the time last oplog record accessed
    void setLastAccessedLogRecordTime(const time_t t);

    //Parse the record into json format and do the corresponding operation
    void parseOpLogObject(mongo::BSONObj& bobj, std::string filterNamespace,
            mongo::DBClientBase& oplogConnection);

    //Check config validity. e.g. if contains port, dbname, etc.
    bool checkConfigValidity();
}
;

#endif //__MONGODBCONNECTOR_H__
