

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
        this->getTransaction()->getSession()->response->finalizeInvalid();
        return ;
    }
    }

    if(shouldSave){
		save();
    }else{
    	shouldShutdown = true;
    }
	return ;
}

void ShutdownCommand::save(){

	saveOperation = new ShardCommand(this, -1, ShardCommandCode_SaveData_SaveMetadata );
	saveOperation->produce();
}

void ShutdownCommand::consume(bool status, map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) {
	if(! status && ! force){
    	this->getTransaction()->getSession()->response->addMessage(
    			"Could not successfully save the indices. Will not shutdown. Either use force=true in the request or try again.");
        this->getTransaction()->getSession()->response->finalizeOK();
        shouldShutdown = false;
	}
	shouldShutdown = true;
	return;
}

void ShutdownCommand::finalizeWork(Transaction::Params * arg){
	if(! shouldShutdown){
		this->getTransaction()->getSession()->response->printHTTP(req);
		return;
	}

	// shut down the cluster.
	// 1. send shut down message to every body.
	// a) prepare list of nodes that we must send shutdown to them
	vector<NodeId> arrivedNodes;
	SP(const ClusterNodes_Writeview) nodesWriteview = this->getNodesWriteview_read();
	nodesWriteview->getArrivedNodes(arrivedNodes, false);
	// b) send shut down message to everybody
	shutdownNotif = SP(ShutdownNotification)(new ShutdownNotification());

	ConcurrentNotifOperation * commandSender = new ConcurrentNotifOperation(shutdownNotif, NULLType, arrivedNodes, NULL, false);
	ShardManager::getStateMachine()->registerOperation(commandSender);

	this->getTransaction()->getSession()->response->printHTTP(req);
	this->_shutdown();
	return; // it never reaches this point because before that the engine dies.
}

void ShutdownCommand::_shutdown(){
	Logger::console("Shutting down the instance upon HTTP request ...");
	raise(SIGTERM);
}

}
}
