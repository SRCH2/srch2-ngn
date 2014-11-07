

#include "ClusterShutdownOperation.h"
#include "../../metadata_manager/Cluster_Writeview.h"
#include "server/HTTPJsonResponse.h"
#include "../../state_machine/StateMachine.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

bool ShutdownCommand::run(){


    switch (req->type) {
    case EVHTTP_REQ_PUT: {
    	getSession()->response->addMessage("Shutting down the cluster...");
        break;
    }
    default: {
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
    	getSession()->response->finalizeInvalid();
        return false;
    }
    }

	save();
	return true;
}

void ShutdownCommand::save(){

	saveOperation = new ShardCommand(this, -1, ShardCommandCode_SaveData_SaveMetadata );
	saveOperation->produce();
}

void ShutdownCommand::consume(map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) {
	this->clusterShutdown();
}

void ShutdownCommand::clusterShutdown(){
	if(saveOperation != NULL){
		ASSERT(false);
		return ;
	}

	// shut down the cluster.
	// 1. send shut down message to every body.
	// a) prepare list of nodes that we must send shutdown to them
	vector<NodeId> arrivedNodes;
	SP(ClusterNodes_Writeview) nodesWriteview = this->getNodesWriteview_read();
	nodesWriteview->getArrivedNodes(arrivedNodes, false);
	// b) send shut down message to everybody
	shutdownNotif = SP(ShutdownNotification)(new ShutdownNotification());

	ConcurrentNotifOperation * commandSender = new ConcurrentNotifOperation(shutdownNotif, NULLType, arrivedNodes, NULL, false);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(commandSender);

	this->setFinished();
	getSession()->response->printHTTP(req);
	ShardManager::getShardManager()->_shutdown();
	return; // it never reaches this point because before that the engine dies.
}

}
}
