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
