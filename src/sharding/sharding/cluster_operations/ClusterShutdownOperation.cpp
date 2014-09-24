

#include "ClusterShutdownOperation.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../../processor/ProcessorUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

OperationState * ClusterShutdownOperation::entry(){
	return save();
}

OperationState * ClusterShutdownOperation::save(){
	if(saveOperation != NULL){
		ASSERT(false);
		return NULL;
	}

	saveOperation = OperationState::startOperation(new ClusterSaveOperation(this->getOperationId()));
	if(saveOperation == NULL){
		return shutdown();
	}
	return this;
}

OperationState * ClusterShutdownOperation::handleSaveOperation(Notification * notification){
	OperationState::stateTransit(saveOperation, notification);
	if(saveOperation == NULL){
		return shutdown();
	}
	return this;
}

OperationState * ClusterShutdownOperation::shutdown(){
	if(saveOperation != NULL){
		ASSERT(false);
		return NULL;
	}

	// shut down the cluster.
	// 1. send shut down message to every body.
	// a) prepare list of nodes that we must send shutdown to them
	vector<NodeId> arrivedNodes;
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	writeview->getArrivedNodes(arrivedNodes, true);

	// b) send shut down message to everybody
	ShutdownNotification * shutdownNotif = new ShutdownNotification();
	for(unsigned n = 0 ; n < arrivedNodes.size(); ++n){
		if(arrivedNodes.at(n) == ShardManager::getCurrentNodeId()){
			continue; // this node must shut down when all notifications are sent.
		}
		send(shutdownNotif, NodeOperationId(arrivedNodes.at(n)));
	}
	delete shutdownNotif;
	ShardManager::getShardManager()->_shutdown();
	return NULL; // it never reaches this point because before that the engine dies.
}


OperationState * ClusterShutdownOperation::handle(Notification * notification){
	if(notification == NULL){
		ASSERT(false);
		return NULL;
	}

	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingLockACKMessageType:
		case ShardingMergeACKMessageType:
		case ShardingSaveDataACKMessageType:
		case ShardingSaveMetadataACKMessageType:
			return handleSaveOperation(notification);
		default:
			ASSERT(false);
			return this;
	}
}



OperationState * ClusterShutdownOperation::handle(NodeFailureNotification * nodeFailureNotif){
	if(saveOperation == NULL){
		return this;
	}
	OperationState::stateTransit(saveOperation, nodeFailureNotif);
	if(saveOperation == NULL){
		return shutdown();
	}
	return this;
}

}
}
