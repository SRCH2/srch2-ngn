
#include "AtomicSaveOperation.h"

#include "../metadata_manager/ResourceMetadataManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

AtomicSaveOperation::AtomicSaveOperation(const unsigned operationId):OperationState(operationId){
	this->dataSavedFlag = false;
}
AtomicSaveOperation::~AtomicSaveOperation(){

}

OperationState * AtomicSaveOperation::entry(){
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
	SaveDataNotification * notif = new SaveDataNotification();
	for(map<NodeId, bool>::const_iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		if(nodesStatusItr->first == writeview->currentNodeId){ // save the data of this node
			pthread_t localSaveThread;
		    if (pthread_create(&localSaveThread, NULL, localSaveData , notif) != 0){
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

OperationState * AtomicSaveOperation::handle(SaveDataNotification::ACK * notification){
	NodeId srcNodeId = notification->getSrc().nodeId;
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

	dataSavedFlag = true;

	return saveMetadata();
}

OperationState * AtomicSaveOperation::saveMetadata(){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	// 1. first, set the status of all nodes to false again to prepare for data save acks
	for(map<NodeId, bool>::iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		nodesStatusItr->second = false;
	}

	// 2. second, send save_metadata command to all nodes
	SaveMetadataNotification * notif = new SaveMetadataNotification();
	for(map<NodeId, bool>::const_iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		if(nodesStatusItr->first == writeview->currentNodeId){ // save the data of this node
			pthread_t localSaveThread;
		    if (pthread_create(&localSaveThread, NULL, localSaveMetadata , notif) != 0){
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

OperationState * AtomicSaveOperation::finalize(){
	return NULL;
}

OperationState * AtomicSaveOperation::handle(SaveMetadataNotification::ACK * notification){

	NodeId srcNodeId = notification->getSrc().nodeId;
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


OperationState * AtomicSaveOperation::handle(NodeFailureNotification * nodeFailure){
	// check if this failed node is among the participants
	if(! (nodesStatus.find(nodeFailure->getFailedNodeID()) == nodesStatus.end())){
		return this;
	}

	// erase this node from participants
	nodesStatus.erase(nodeFailure->getFailedNodeID());

	if(! haveAllNodeseReplied()){
		return this; // doesn't affect save
	}

	// we should see whether we must finish the process or continue to saveMetadata
	if(! dataSavedFlag){
		// we should proceed to saveMetadata
		dataSavedFlag = true;
		return saveMetadata();
	}

	return finalize();
}

OperationState * AtomicSaveOperation::handle(Notification * notification){
	if(notification == NULL){
		ASSERT(false);
		return NULL;
	}

	switch (notification->messageType()) {
		case ShardingNodeFailureNotificationMessageType:
			return handle((NodeFailureNotification *)notification);
		case ShardingSaveDataACKMessageType:
			return handle((SaveDataNotification::ACK *)notification);
		case ShardingSaveMetadataACKMessageType:
			return handle((SaveMetadataNotification::ACK *)notification);
		default:
			ASSERT(false);
			return this;
	}
}

string AtomicSaveOperation::getOperationName() const {
	return "atomic_save";
}
string AtomicSaveOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Participants : %" ;
	for(map<NodeId, bool>::const_iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		ss << "Node " << nodesStatusItr->first << " : " << nodesStatusItr->second << "%";
	}
	return ss.str();
}


bool AtomicSaveOperation::haveAllNodeseReplied() const{
	for(map<NodeId, bool>::const_iterator nodesStatusItr = nodesStatus.begin(); nodesStatusItr != nodesStatus.end(); ++nodesStatusItr){
		if(nodesStatusItr->second == false){
			return false;
		}
	}
	return true;
}

void * AtomicSaveOperation::localSaveData(void * arg){
	SaveDataNotification * saveNotif = (SaveDataNotification *)arg;
	boost::unique_lock<boost::mutex> lock(ShardManager::getShardManager()->shardManagerGlobalMutex);
	ShardManager::getShardManager()->resolve(saveNotif);
	return NULL;
}


void * AtomicSaveOperation::localSaveMetadata(void * arg){
	SaveMetadataNotification * saveNotif = (SaveMetadataNotification *)arg;
	boost::unique_lock<boost::mutex> lock(ShardManager::getShardManager()->shardManagerGlobalMutex);
	ShardManager::getShardManager()->resolve(saveNotif);
	return NULL;
}


}

}
