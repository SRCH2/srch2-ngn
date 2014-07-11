/*
 * DataConnectorThread.cpp
 *
 *  Created on: Jun 24, 2014
 *      Author: liusrch2
 */

#include <dlfcn.h>
#include "DataConnectorThread.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <cstdlib>
#include "DataConnector.h"

//Called by the pthread_create, create the database connector
void * spawnConnector(void *arg) {
    ConnectorThreadArguments * connThreadArg = (ConnectorThreadArguments *) arg;
    DataConnectorThread::bootStrapConnector(connThreadArg);

    delete connThreadArg->server;
    delete connThreadArg;

    return NULL;
}

//The main function run by the thread, get connector and start listener.
void DataConnectorThread::bootStrapConnector(ConnectorThreadArguments * connThreadArg) {
    void * pdlHandle = NULL;

    //Get the pointer of the shared library
    std::string libName =  connThreadArg->sharedLibraryFullPath;

    pdlHandle = dlopen(libName.c_str(), RTLD_LAZY); //Open the shared library.

    if (!pdlHandle) {
        Logger::error("Fail to load shared library %s due to %s",
                libName.c_str(), dlerror());
        exit(1);   //Exit if can not open the shared library
    }

    /*
     * Suppress specific warnings on gcc/g++.
     * See: http://www.mr-edd.co.uk/blog/supressing_gcc_warnings
     * The warnings g++ spat out is to do with an invalid cast from
     * a pointer-to-object to a pointer-to-function before gcc 4.
     */
#ifdef __GNUC__
    __extension__
#endif
    //Get the function "create" in the shared library.
    create_t* create_dataConnector = (create_t*) dlsym(pdlHandle, "create");

    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        Logger::error(
                "Cannot load symbol \"create\" in shared library due to: %s",
                dlsym_error);
        exit(1);   //Exit if can not open the shared library
    }

    /*
     * Suppress specific warnings on gcc/g++.
     * See: http://www.mr-edd.co.uk/blog/supressing_gcc_warnings
     * The warnings g++ spat out is to do with an invalid cast from
     * a pointer-to-object to a pointer-to-function before gcc 4.
     */
#ifdef __GNUC__
    __extension__
#endif
    destroy_t* destroy_dataConnector = (destroy_t*) dlsym(pdlHandle, "destroy");

    dlsym_error = dlerror();
    if (dlsym_error) {
        Logger::error(
                "Cannot load symbol \"destroy\" in shared library due to: %s",
                dlsym_error);
        exit(1);   //Exit if can not open the shared library
    }

    //Call the "create" function in the shared library.
    DataConnector * connector = create_dataConnector();

    if (connector->init(connThreadArg->server)) {
        if (!connThreadArg->createNewIndexFlag) {
            Logger::debug("Create Indices from empty");
            connector->createNewIndexes();
        }
        connector->runListener();
    }

    //After the listener;
    destroy_dataConnector(connector);
    dlclose(pdlHandle);
}

//Create thread if interface built successfully.
void DataConnectorThread::getDataConnectorThread(void * server) {
    pthread_t tid;
    ConnectorThreadArguments * dbArg = new ConnectorThreadArguments();
    ServerInterfaceInternal * internal = new ServerInterfaceInternal(server);

    //If the source is not database, do not create the thread.
    if (!internal->isDatabase()) {
        delete dbArg;
        delete internal;
    } else {
        dbArg->server = internal;
        dbArg->createNewIndexFlag = checkIndexExistence(server);

        srch2::httpwrapper::Srch2Server* srch2Server =
                (srch2::httpwrapper::Srch2Server*) server;
        dbArg->sharedLibraryFullPath =
                srch2Server->indexDataConfig->getDatabaseSharedLibraryPath()
                        + "/"
                        + srch2Server->indexDataConfig->getDatabaseSharedLibraryName()
                        + ".so";

        int res = pthread_create(&tid, NULL, spawnConnector, (void *) dbArg);
    }
}


bool DataConnectorThread::checkIndexExistence(void * server) {
    srch2::httpwrapper::Srch2Server * srch2Server =
            (srch2::httpwrapper::Srch2Server*) server;

    const string &directoryName = srch2Server->indexDataConfig->getIndexPath();
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::analyzerFileName).c_str()))
        return false;
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::trieFileName).c_str()))
        return false;
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::forwardIndexFileName).c_str()))
        return false;
    if (!checkDirExistence(
            (directoryName + "/" + IndexConfig::schemaFileName).c_str()))
        return false;
    if (srch2Server->indexDataConfig->getIndexType()
            == srch2::instantsearch::DefaultIndex) {
        // Check existence of the inverted index file for basic keyword search ("A1")
        if (!checkDirExistence(
                (directoryName + "/" + IndexConfig::invertedIndexFileName).c_str()))
            return false;
    } else {
        // Check existence of the quadtree index file for geo keyword search ("M1")
        if (!checkDirExistence(
                (directoryName + "/" + IndexConfig::quadTreeFileName).c_str()))
            return false;
    }
    return true;
}
