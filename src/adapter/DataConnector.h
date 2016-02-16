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
 * DataConnector.h
 *
 *  Created on: Jun 9, 2014
 *      Author: Chen Liu at SRCH2
 */

#ifndef __DATACONNECTOR_H__
#define __DATACONNECTOR_H__

#include <string>

/*
 *  The abstract class ServerInterface provides the interface of the engine to
 *  an external database connector. The concrete implementation is within the 
 *  engine.
 */

class ServerInterface {
public:
    virtual ~ServerInterface() {
    }
    ;
    /*
     * "insertRecord" takes a record as an input in a JSON string format.
     *
     *  Note: The API only accepts single JSON Object as a string.
     *        It does not accept JSON Array as a string.
     */
    virtual int insertRecord(const std::string& jsonString) = 0;

    /*
     * "deleteRecord" takes the primary key of a record as an input and deletes
     *  it from engine.
     */
    virtual int deleteRecord(const std::string& primaryKey) = 0;

    /*
     * "updateRecord" takes the old primary key of a record as an input and
     * updates it with the new record passed in as a JSON string.
     */
    virtual int updateRecord(const std::string& oldPk,
            const std::string& jsonString) = 0;

    /*
     * "configLookUp" provides key based lookup for the connector specific
     *  configuration. The return value can be a single value or multiple values
     *  separated by comma. 
     *  e.g.  "dbname" => "mysql"  (single value)
     *        "collections" => "collection1, collection2 " (multiple values)
     */
    virtual int configLookUp(const std::string& key, std::string & value) = 0;
};

/*
 *  A database connector for the engine should implement the interface provided
 *  in the DataConnector abstract class by extending it.
 */
class DataConnector {
public:
    virtual ~DataConnector() {
    }
    ;

    /*
     * "init" is called once when the connector is loaded by the engine.
     * All the initialization is recommended to be implemented here.
     * e.g., check the config file and connect to the database.
     *
     * The serverHandle is provided by the engine which is an instance of 
     * ServerInterface class. The serverHandle must be used to call 
     * ServerInterface class APIs. 
     */
    virtual int init(ServerInterface *serverHandle) = 0;

    /*
     * "runListener" should be implemented as a pull based listener.
     * It should periodically fetch the data changes from the database.
     */
    virtual int runListener() = 0;

    /*
     * "createNewIndexes" is called when there is no index found
     * in the folder <dataDir>. It should fetch all the data from the
     * collection and insert them into the srch2-engine.
     */
    virtual int createNewIndexes() = 0;

    /*
     * "saveLastAccessedLogRecordTime" is called when the engine is saving
     * the indexes. The data connector should also save the timestamp so that
     * the next time when the engine starts, the connector can ignore the
     * previous executed log events.
     */
    virtual void saveLastAccessedLogRecordTime() = 0;
};

/*
 * "create_t()" and "destroy_t(DataConnector*)" are called to create/delete
 *  the instance of the connector, respectively.
 *
 * extern "C" DataConnector* create() {
 *     return new YourDBConnector;
 * }
 *
 * extern "C" void destroy(DataConnector* p) {
 *     delete p;
 * }
 *
 * These two C APIs are used by the srch2-engine to create/delete the instance
 * in the shared library.
 * The engine will call "create()" to get the connector and call
 * "destroy" to delete it.
 */
typedef DataConnector* create_t();
typedef void destroy_t(DataConnector*);

#endif /* __DATACONNECTOR_H__ */
