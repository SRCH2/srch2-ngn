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
    /*
     * "insertRecord" takes the Record in JSON string format
     *  and passes it into the srch2-engine.
     *
     *  Note that only JSON Object string is accepted.
     *  Does not accept JSON Array string.
     */
    virtual int insertRecord(const std::string& jsonString) = 0;

    /*
     * "deleteRecord" takes the primary key in string format and
     *  delete the record in the srch2-engine.
     */
    virtual int deleteRecord(const std::string& primaryKey) = 0;

    /*
     * "updateRecord" takes the old primary key of the Record and the new
     * Record in JSON string format.
     */
    virtual int updateRecord(const std::string& oldPk,
            const std::string& jsonString) = 0;

    /*
     * "saveChanges" saves the index into the disk.
     */
    virtual void saveChanges() = 0;

    /*
     * "configLookUp" will provide key based lookup from engine's connector
     *  specific configuration store. e.g.  "dbname" => "mysql"  (single value)
     *  "collections" => "collection1, collection2 " (multi value)
     */
    virtual bool configLookUp(const std::string& key, std::string & value) = 0;
};

class DataConnector {
public:
    virtual ~DataConnector() {
    }
    ;

    /*
     * "init" is called when the connector starts running.
     * All the initialization is recommended to be implemented here. e.g.
     * "check the config file", "connect to the database".
     *
     * The serverHandle is provided by the srch2-engine, providing the
     * functions in the class "ServerInterface".
     */
    virtual bool init(ServerInterface *serverHandle) = 0;

    /*
     * "runListener" should be implemented as a pull based listener.
     * It should periodically fetch the data changes form the database.
     */
    virtual void* runListener() = 0;

    /*
     * "createNewIndexes" is called when there is no index found
     * in the folder <dataDir>. It should fetch all the data from the
     * collection and insert them into the srch2-engine.
     */
    virtual void createNewIndexes() = 0;
};

/*
 * "create_t()" and "destroy_t(DataConnector*)" is called to create/delete
 * the instance of the connector. A simple example of implementing these
 * two function is here.
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
