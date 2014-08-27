#include "CommitOperation.h"

#include "core/util/SerializationHelper.h"
#include "src/core/util/Assert.h"
#include "metadata_manager/Shard.h"
#include "metadata_manager/Node.h"
#include "./metadata_manager/Cluster_Writeview.h"
#include "./ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


CommitOperation::CommitOperation(const unsigned & operationId,
		const vector<NodeId> & exceptions, MetadataChange * metadataChange):OperationState(operationId){
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

CommitOperation::CommitOperation(const unsigned & operationId,
		const NodeId & exception, MetadataChange * metadataChange):OperationState(operationId){
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

OperationState * CommitOperation::entry(){

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

OperationState * CommitOperation::handle(NodeFailureNotification * nodeFailure){
	// erase any failed nodes from the participants list.
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	map<NodeId, ShardingNodeState> nodeStates;
	map<NodeId, std::pair<ShardingNodeState, Node *> > & allNodes = writeview->nodes;
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr =
			allNodes.begin(); nodeItr != allNodes.end(); ++nodeItr){
		nodeStates[nodeItr->first] = nodeItr->second.first;
	}
	vector<NodeId> participantsFixed;
	for(unsigned i = 0 ; i < participants.size(); ++i){
		if(nodeStates[participants.at(i)] == ShardingNodeStateArrived){
			participantsFixed.push_back(participants.at(i));
		}
	}
	participants = participantsFixed;
	if(participants.size() > 0){
		return this;
	}
	return NULL;
}

// returns false when it's done.
OperationState * CommitOperation::handle(CommitNotification::ACK * inputNotification){
	if(! doesExpect(inputNotification)){
		ASSERT(false);
		return this;
	}
	NodeOperationId srcOpId = inputNotification->getSrc();
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
bool CommitOperation::doesExpect(CommitNotification::ACK * inputNotification) const{
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

string CommitOperation::getOperationName() const {
	return "commit_operation";
};
string CommitOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Metadata change : " ;
	if(metadataChange == NULL){
		ss << "NULL%";
	}else{
		ss << metadataChange->toString();;
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
