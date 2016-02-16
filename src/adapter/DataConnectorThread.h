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
 * DataConnectorThread.h
 *
 *  Created on: Jun 24, 2014
 *      Author: Chen Liu at SRCH2
 */

#ifndef __DATACONNECTORTHREAD_H__
#define __DATACONNECTORTHREAD_H__

#include <string>
#include <map>
#include "DataConnector.h"
#include "ServerInterfaceInternal.h"

//Arguments passed to the thread
struct ConnectorThreadArguments {
    ServerInterface* serverInterface;
    bool createNewIndexFlag;
    std::string sharedLibraryFullPath;
    std::string coreName;
};

//Called by the pthread_create, create the database connector
void * spawnConnector(void *arg);

class DataConnectorThread {
public:
    //Create thread if interface built successfully.
    static void getDataConnectorThread(void * server);
    //The main function run by the thread, get connector and start listener.
    static void bootStrapConnector(ConnectorThreadArguments * connThreadArg);
    static void saveConnectorTimestamps();
private:
    static bool checkIndexExistence(void * server);
    static void getDataConnector(std::string & path, void * pdlHandle,
            DataConnector *& connector);
    static void closeDataConnector(void * pdlHandle,
            DataConnector *& connector);
    static std::map<std::string, DataConnector *> connectors;
};

#endif /* __DATACONNECTORTHREAD_H__ */
