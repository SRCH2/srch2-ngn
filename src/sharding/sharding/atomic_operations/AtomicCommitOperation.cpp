#include "AtomicCommitOperation.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../metadata_manager/Shard.h"
#include "../metadata_manager/Node.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


AtomicCommitOperation::AtomicCommitOperation(const unsigned & operationId,
		const vector<NodeId> & exceptions, MetadataChange * metadataChange):OperationState(operationId){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();
	vector<NodeId> allNodes;
	writeview->getArrivedNodes(allNodes, true);
	for(unsigned i = 0;  i < allNodes.size(); ++i){
		if(std::find(exceptions.begin(), exceptions.end(), allNodes.at(i)) == exceptions.end()){
			this->participants.push_back(allNodes.at(i));
		}
	}
}

AtomicCommitOperation::AtomicCommitOperation(const unsigned & operationId,
		const NodeId & exception, MetadataChange * metadataChange):OperationState(operationId){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();
	vector<NodeId> allNodes;
	writeview->getArrivedNodes(allNodes);
	for(unsigned i = 0;  i < allNodes.size(); ++i){
		if(exception != allNodes.at(i)){
			this->participants.push_back(allNodes.at(i));
		}
	}
}

AtomicCommitOperation::AtomicCommitOperation(const unsigned & operationId,
		MetadataChange * metadataChange, const vector<NodeId> & participants):OperationState(operationId){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
	this->participants = participants;
}

AtomicCommitOperation::~AtomicCommitOperation(){
	if(metadataChange != NULL){
		delete metadataChange;
	}
}

OperationState * AtomicCommitOperation::entry(){

	if(participants.size() == 0){
		// no participant exists
		return NULL;
	}
	if(metadataChange == NULL){
		ASSERT(false);
		return NULL;
	}

	Cluster_Writeview * writeview = ShardManager::getWriteview();

	// 1. send commit notification to all other nodes
	unsigned localIndex = participants.size();
	CommitNotification * commitNotification = new CommitNotification( metadataChange );
	for(unsigned i = 0 ; i < participants.size(); ++i){
		if(participants.at(i) == ShardManager::getCurrentNodeId()){
			localIndex = i;
			ShardManager::getShardManager()->getMetadataManager()->applyAndCommit(metadataChange);
		}else{
			this->send(commitNotification, NodeOperationId(participants.at(i)));
		}
	}
	if(localIndex < participants.size()){ // if we had local node
		// we had a local commit, so we must remove local index from list of participants
		participants.erase(participants.begin() + localIndex);
	}
	delete commitNotification;
	if(participants.size() > 0){
		return this;
	}
	return NULL;
}

OperationState * AtomicCommitOperation::handle(NodeFailureNotification * nodeFailure){
	if(nodeFailure == NULL){
		ASSERT(false);
		return this;
	}

	unsigned failedNodeIndex = 0;
	for(failedNodeIndex = 0; failedNodeIndex < participants.size(); ++failedNodeIndex){
		if( participants.at(failedNodeIndex) == nodeFailure->getFailedNodeID() ){
			break;
		}
	}

	if(failedNodeIndex >= participants.size()){
		return this; // failed node is not in the list, either returned the response before or not in list from the beginning
	}

	participants.erase(participants.begin() + failedNodeIndex);

	if(participants.size() > 0){
		return this;
	}
	return NULL;
}

// returns false when it's done.
OperationState * AtomicCommitOperation::handle(CommitNotification::ACK * commitAck){

	if(! doesExpect(commitAck)){
		ASSERT(false);
		return this;
	}

	NodeOperationId srcOpId = commitAck->getSrc();
	// remove nodeid from participants list
	unsigned nodeIndex = participants.size();
	for(unsigned i = 0 ; i < participants.size(); ++i){
		if(participants.at(i) == srcOpId.nodeId){
			nodeIndex = i;
			break;
		}
	}
	if(nodeIndex >= participants.size()){
		ASSERT(false);
		return this;
	}
	participants.erase(participants.begin() + nodeIndex);
	if(participants.size() > 0){
		return this;
	}
	return NULL;
}
bool AtomicCommitOperation::doesExpect(CommitNotification::ACK * inputNotification) const{
	if(inputNotification == NULL){
		ASSERT(false);
		return false;
	}
	// ack src node id must be participants
	for(unsigned i = 0 ; i < participants.size(); ++i){
		if(participants.at(i) == inputNotification->getSrc().nodeId){
			return true;
		}
	}
	return false;
}

string AtomicCommitOperation::getOperationName() const {
	return "commit_operation";
};
string AtomicCommitOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Metadata change : " ;
	if(metadataChange == NULL){
		ss << "NULL%";
	}else{
		ss << metadataChange->toString();
	}
	ss << "Participants : " ;
	for(unsigned i  = 0 ; i < participants.size(); ++i){
		if(i != 0){
			ss << " - ";
		}

		ss << i << ":" << participants.at(i) ;
	}
	ss << "%";
	return ss.str();
};


}
}
