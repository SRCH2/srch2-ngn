/*
 * DataConnectorThread.h
 *
 *  Created on: Jun 24, 2014
 *      Author: liusrch2
 */

#ifndef __DATACONNECTORTHREAD_H__
#define __DATACONNECTORTHREAD_H__

#include <string>
#include "DataConnector.h"
#include "ServerInterfaceInternal.h"

//Arguments passed to the thread
struct ConnectorThreadArguments {
    ServerInterface* server;
    bool ifCreateNewIndex;
};

//Called by the pthread_create, create the database connector
void * spawnConnector(void *arg);

class DataConnectorThread {
public:
    //Create thread if interface built successfully.
    static void getDataConnectorThread(void * server);
    //The main function run by the thread, get connector and start listener.
    static void bootStrapConnector(ConnectorThreadArguments * targ);
private:
    static bool checkIndexExistence(void * server);
    //Get the pointer and handle to the specific connector in shared library.
    static DataConnector * getDataConnector(void * pdlHandle,
            const std::string& sharedLibraryPath);
    static const std::string DATABASE_SHARED_LIBRARY_PATH;
};

#endif /* __DATACONNECTORTHREAD_H__ */
