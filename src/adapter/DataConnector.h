#ifndef __DATACONNECTOR_H__
#define __DATACONNECTOR_H__

#include <string>

/*
 *  The abstract class ServerInterface provides the interface of the engine to
 *  an external data connector. Its implementation is within the 
 *  engine.
 */

class ServerInterface {
public:
    virtual ~ServerInterface() {
    }
    ;
    /*
     * This function inserts a record (in JSON format)
     * of this source to the SRCH2 indexes.
     *
     * Parameters:
     *  jsonString : A JSON format string.
     *
     * Return value:
     *    0: success;
     *   Otherwise: failed.
     *
     * Notice that this function accepts a single JSON string, and
     * does not accept a string of a JSON array.
     */
    virtual int insertRecord(const std::string& jsonString) = 0;

    /*
     * This function deletes a record with a specified primary key from the
     * SRCH2 indexes for this source.
     *
     *  Parameters:
     *    primaryKey: A string as the primary key of a record to
     *         be deleted.
     *
     * Return value:
     *    0: success;
     *    Otherwise : failed.
     */
    virtual int deleteRecord(const std::string& primaryKey) = 0;

    /*
     * This function takes the old primary key of a record as an input and
     * updates it in the engine with a new record passed as a JSON format
     * string.
     *
     * Parameters:
     *   oldPk: The old primary key of the updated record.
     *   jsonString:  The new record as a JSON string.
     *
     * Return value:
     *   0: success;
     *   Otherwise: failed.
     */
    virtual int updateRecord(const std::string& oldPk,
            const std::string& jsonString) = 0;

    /*
     * This function supports a key-based lookup for a parameter for the
     * connector, as specified in the dbKeyValues section
     * in the configuration file.
     *
     * Parameters:
     *   key: A key defined in the config file;
     *   value: The corresponding value.

     * Return value:
     *    0 : success
     *    -1 : value not found, and the value will be empty.
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
     * The serverInterface is provided by the engine which is an instance of
     * ServerInterface class. The serverInterface must be used to call
     * ServerInterface class APIs. 
     */
    virtual int init(ServerInterface *serverInterface) = 0;

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
 * These two C APIs are used by the SRCH2 engine to create/delete the
 * instance in the shared library. The engine will call "create()" to
 * get the connector and call "destroy" to delete it.
 */
typedef DataConnector* create_t();
typedef void destroy_t(DataConnector*);

#endif /* __DATACONNECTOR_H__ */
