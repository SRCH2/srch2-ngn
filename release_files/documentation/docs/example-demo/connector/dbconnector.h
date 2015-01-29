
#ifndef __DBCONNECTOR_H__
#define __DBCONNECTOR_H__

#include "DataConnector.h"
#include <string>
#include <vector>

/*
 * Assuming the target table "emp(id, name, age, salary)" is in
 * the database and the log table schema is "emp_ct(ts, op, id, name, age, salary)".
 *
 * Most databases have their log mechanism, for example, mongodb is "oplog",
 * MySQL is "binary log", MS SQL Server is "Change table" and "Oracle" is
 * "Change table/Change set".
 */
class ExampleConnector: public DataConnector {
public:
    ExampleConnector();
    virtual ~ExampleConnector();

    /*
     * Initialize the connector. Establish a connection to the Database.
     * The init() function should implement the following things :
     *
     * 1. Pass the ServerInterface handle from the engine to the connector so
     * that the connector can use function "insertRecord", "deleteRecord",
     * "updateRecord" and "configLookUp".
     *
     * 2. Check if the config file contains all the required parameters.
     *
     * 3. Connect to the database.
     *
     * 4. Get the schema information from the database.
     */
    virtual int init(ServerInterface *serverHandle);

    /*
     * Retrieve records from the table records and insert them into the SRCH2 engine.
     * Query : SELECT * FROM emp;
     */
    virtual int createNewIndexes();

    /*
     * Periodically Pull the updates from the log table, and send
     * corresponding requests to the SRCH2 engine.
     * For example: table emp(id, name, age, salary).
     * Change table name : emp_ct
     * Query : SELECT timestamp, OPERATION$, id, name, age, salary
     *         FROM emp_ct
     *         WHERE timestamp$ > ?;
     * Query is using prepared statement where "?" is the last accessed record timestamp.
     * The connector always keeps the latest timestamp so that the connector can skip the
     * processed record.
     */
    virtual int runListener();

    /*
     * Save the lastAccessedLogRecordTS to the file.
     * This function is called automatically by the engine when
     * the engine is saving the indexes.
     */
    virtual void saveLastAccessedLogRecordTime();
private:
    //Database connection handlers
    //Some parameters

    //Config file parameters
    int listenerWaitTime;
    ServerInterface *serverHandle;

    //Storing the table schema information {"id", "name", "age", "salary"}
    std::vector<std::string> fieldNames;

    /*
     * The last time the connector accessed the log table ts,
     */
    long int lastAccessedLogRecordTS;

    /*
     * Connect to the database
     */
    bool connectToDB();

    /*
     * Check if the config file has all the required parameters.
     * e.g. if it contains dataSource, table etc.
     * The config file must indicate the Data Source configuration name, host address,
     * user, primary key, etc. Otherwise, the check fails.
     */
    bool checkConfigValidity();

    //Fetch the table schema and store into tableSchema
    bool populateTableSchema(std::string & tableName);

    /*
     * Load the last accessed record time stamp from the file.
     * If the file does not exist(The first time create the indexes),
     * it will query the database and fetch the latest record ts.
     */
    void loadLastAccessedLogRecordTime();

    //Get the MAX RSID$ from Oracle database.
    //Query : "SELECT MAX(ts) FROM emp_ct";
    void populateLastAccessedLogRecordTime();

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
extern "C" DataConnector* create() {
    return new ExampleConnector;
}

extern "C" void destroy(DataConnector* p) {
    delete p;
}

#endif /* DBCONNECTOR_H_ */
