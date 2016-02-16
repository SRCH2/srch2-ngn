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


#include "ShutdownCommand.h"
#include "../../metadata_manager/Cluster_Writeview.h"
#include "server/HTTPJsonResponse.h"
#include "../../state_machine/StateMachine.h"
#include <pthread.h>
#include <signal.h>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

void ShutdownCommand::run(){


    switch (req->type) {
    case EVHTTP_REQ_PUT: {
    	Logger::console("Shutting down the cluster...");
        evkeyvalq headers;
        evhttp_parse_query(req->uri, &headers);
        const char * flagSet = evhttp_find_header(&headers, URLParser::shutdownForceParamName);
        if(flagSet){
        	if(((string)"1").compare(flagSet) == 0){
        		force = true;
        	}
        }
        flagSet = evhttp_find_header(&headers, URLParser::shutdownSaveParamName);
        if(flagSet){
        	if(((string)"false").compare(flagSet) == 0){
        		shouldSave = false;
        	}
        }
		evhttp_clear_headers(&headers);
        break;
    }
    default: {
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        this->getSession()->response->finalizeInvalid();
        return ;
    }
    }

    if(shouldSave){
		save();
    }
	return ;
}

void ShutdownCommand::save(){

	Cluster_Writeview * writeview = this->getWriteview();
	for(map<unsigned, CoreInfo_t *>::iterator coreItr = writeview->cores.begin();
			coreItr != writeview->cores.end(); ++coreItr){
		unsigned coreId = coreItr->first;
		ShardCommand *saveOperation = new ShardCommand(this, coreId, ShardCommandCode_SaveData_SaveMetadata );
		saveOperations.push_back(saveOperation);
		saveOperation->produce();
	}
}

void ShutdownCommand::consume(bool status, map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) {

	if(status && shouldShutdown){
		shouldShutdown = true;
	}
	if(! status && ! force){
		this->getSession()->response->addMessage(
				"Could not successfully save the indices. Will not shutdown. Either use force=true in the request or try again.");
		this->getSession()->response->finalizeOK();
		shouldShutdown = false;
		return;
	}
	return;
}

void ShutdownCommand::finalizeWork(Transaction::Params * arg){
	if(! shouldShutdown){
		this->getSession()->response->printHTTP(req);
		return;
	}

	// shut down the cluster.
	// 1. send shut down message to every body.
	// a) prepare list of nodes that we must send shutdown to them
	vector<NodeId> nonFailedNodes;
	SP(const ClusterNodes_Writeview) nodesWriteview = this->getNodesWriteview_read();
	nodesWriteview->getNonFailedNodes(nonFailedNodes, false);

	if(! nonFailedNodes.empty()){
		// b) send shut down message to everybody
		shutdownNotif = SP(ShutdownNotification)(new ShutdownNotification());

		ConcurrentNotifOperation * commandSender = new ConcurrentNotifOperation(shutdownNotif, NULLType, nonFailedNodes, NULL, false);
		ShardManager::getStateMachine()->registerOperation(commandSender);
	}

	this->getSession()->response->addMessage("Cluster will shutdowns peacefully.");
	this->getSession()->response->finalizeOK();
	this->getSession()->response->printHTTP(req);
	this->_shutdown();
	return; // it never reaches this point because before that the engine dies.
}

void ShutdownCommand::_shutdown(){
    pthread_t localThread;
    if (pthread_create(&localThread, NULL, _shutdownAnotherThread , NULL) != 0){
        // Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for handling local message");
        Logger::sharding(Logger::Error, "SHM| Cannot create thread for sending the SIGTERM signal.");
        return;
    }
    pthread_detach(localThread);
}

void * ShutdownCommand::_shutdownAnotherThread(void * args){
	Logger::console("Shutting down the instance upon HTTP request ...");
	raise(SIGTERM);
	return NULL;
}

}
}
