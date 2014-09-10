#include "AtomicMergeOperation.h"

#include "../metadata_manager/Cluster_Writeview.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

AtomicMergeOperation::AtomicMergeOperation(const unsigned operationId):OperationState(operationId){

}
AtomicMergeOperation::~AtomicMergeOperation(){

}

// send merge notification to all nodes in the cluster
// they will do the merge and reply MERGE::ACK
// when everybody replied we are done.
OperationState * AtomicMergeOperation::entry(){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	// 1. the nodesStatus structure
	vector<NodeId> arrivedNodes;
	writeview->getArrivedNodes(arrivedNodes, true);

	if(arrivedNodes.size() == 0){
		ASSERT(false);
		return NULL;
	}

	for(unsigned n = 0 ; n < arrivedNodes.size(); n++){
		nodesStatus[arrivedNodes.at(n)] = false;
	}

	// 2. ask all nodes to save their indices
	MergeNotification * notif = new MergeNotification();
	for(map<NodeId, bool>::const_iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		if(nodesStatusItr->first == writeview->currentNodeId){ // save the data of this node
			pthread_t localMergeThread;
		    if (pthread_create(&localMergeThread, NULL, localMerge , notif) != 0){
		        // Logger::console("Cannot create thread for handling local message");
		        perror("Cannot create thread for handling local message");
		        return NULL;
		    }
		}else{
			this->send(notif, NodeOperationId(nodesStatusItr->first));
		}
	}
	delete notif;
	return this;
}
OperationState * AtomicMergeOperation::handle(MergeNotification::ACK * ack){
	NodeId srcNodeId = ack->getSrc().nodeId;
	if(nodesStatus.find(srcNodeId) == nodesStatus.end()){
		ASSERT(false);
		return this;
	}
	if(nodesStatus[srcNodeId] == true){
		ASSERT(false); /// two replies ?
		return this;
	}

	nodesStatus[srcNodeId] = true;
	if(! haveAllNodeseReplied() ){
		return this;
	}

	return finalize();
}
OperationState * AtomicMergeOperation::handle(NodeFailureNotification * nodeFailure){
	// check if this failed node is among the participants
	if(! (nodesStatus.find(nodeFailure->getFailedNodeID()) == nodesStatus.end())){
		return this;
	}

	// erase this node from participants
	nodesStatus.erase(nodeFailure->getFailedNodeID());

	if(! haveAllNodeseReplied()){
		return this; // doesn't affect save
	}

	return finalize();
}
OperationState * AtomicMergeOperation::handle(Notification * notification){
	if(notification == NULL){
		ASSERT(false);
		return NULL;
	}

	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingMergeACKMessageType:
			return handle((MergeNotification::ACK *)notification);
		default:
			ASSERT(false);
			return this;
	}
}

string AtomicMergeOperation::getOperationName() const {
	return "atomic_merge_operation";
}
string AtomicMergeOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Participants : %" ;
	for(map<NodeId, bool>::const_iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		ss << "Node " << nodesStatusItr->first << " : " << nodesStatusItr->second << "%";
	}
	return ss.str();
}

OperationState * AtomicMergeOperation::finalize(){
	return NULL;
}

bool AtomicMergeOperation::haveAllNodeseReplied() const{
	for(map<NodeId, bool>::const_iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		if(nodesStatusItr->second == false){
			return false;
		}
	}
	return true;
}

void * AtomicMergeOperation::localMerge(void * arg){
	MergeNotification * mergeNotif = (MergeNotification *) arg;
	boost::unique_lock<boost::mutex> lock(ShardManager::getShardManager()->shardManagerGlobalMutex);
	ShardManager::getShardManager()->resolve(mergeNotif);
	return NULL;
}

}
}
