/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * mongodbconnector.h
 *
 *  Created on: Jun 9, 2014
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
    virtual int init(ServerInterface *serverInterface);

    //Listen to the oplog and do modification to the engine
    virtual int runListener();

    //Load the table records and insert into the engine
    virtual int createNewIndexes();

    //Save the lastAccessedLogRecordTime to the disk
    virtual void saveLastAccessedLogRecordTime();
private:
    ServerInterface *serverInterface;
    mongo::DBClientBase* oplogConnection;
    mongo::ScopedDbConnection * mongoConnector;
    time_t lastAccessedLogRecordTime;

    //Connect to the mongodb
    bool connectToDB();

    //Load the last time last oplog record accessed
    bool loadLastAccessedLogRecordTime();

    //Parse the record into json format and do the corresponding operation
    void parseOpLogObject(mongo::BSONObj& bobj, std::string filterNamespace,
            mongo::DBClientBase& oplogConnection);

    //Check config validity. e.g. if contains port, dbname, etc.
    bool checkConfigValidity();



}
;

#endif //__MONGODBCONNECTOR_H__
