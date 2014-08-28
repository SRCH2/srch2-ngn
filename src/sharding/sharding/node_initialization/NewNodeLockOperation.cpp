#include "NewNodeLockOperation.h"

#include "../ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


/*
 * NOTE : metadata nodes map must NOT contain anything from NA notifications when we come to this
 *        constructor. It can be achieved if we just don't do any commit until we finish node intialization.
 */
NewNodeLockOperation::NewNodeLockOperation(const unsigned & operationId):OperationState(operationId){


	lockRequests = new ResourceLockRequest();

	Cluster_Writeview * writeview = ShardManager::getWriteview();
	// prepare the lock requests to be used in entry()
	map<ClusterShardId, ResourceLockType> lockTypes;
	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(isLocal){
			lockTypes[id] = ResourceLockType_X;
		}else{
			lockTypes[id] = ResourceLockType_S;
		}
		lockRequests->requestBatch.push_back(new SingleResourceLockRequest(id,
				NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()), lockTypes[id]));
	}

	this->lastShardLockRepository = NULL; // this will set when we get ACKs
	this->nodeIndex = 0;
	this->newNodeLockNotification = NULL;
}

NewNodeLockOperation::~NewNodeLockOperation(){
	delete lockRequests;
	if(lastShardLockRepository != NULL){
		delete lastShardLockRepository;
	}
	if(newNodeLockNotification != NULL){
		delete newNodeLockNotification;
	}
}

// sends the node list and lock request to the node with minimum id
OperationState * NewNodeLockOperation::entry(){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	// prepare the list of nodes that are discovered before us.
	vector<const Node *> allNodes;
	writeview->getAllNodes(allNodes);
	for(unsigned i = 0 ; i < allNodes.size(); ++i){
		if(allNodes.at(i)->getId() >= ShardManager::getCurrentNodeId()){
			continue;
		}
		allNodesUpToCurrentNode.push_back(allNodes.at(i)->getId());
	}
	std::sort(allNodesUpToCurrentNode.begin(), allNodesUpToCurrentNode.end());

	ASSERT(allNodesUpToCurrentNode.size() > 0 &&
			allNodesUpToCurrentNode.at(allNodesUpToCurrentNode.size()-1) < ShardManager::getCurrentNodeId());

	// prepare the notification and send it to the first node
	this->newNodeLockNotification = new NewNodeLockNotification(allNodesUpToCurrentNode, lockRequests);
	this->nodeIndex = 0;
	this->send(newNodeLockNotification, NodeOperationId(allNodesUpToCurrentNode.at(this->nodeIndex)));
	return this;
}

OperationState * NewNodeLockOperation::handle(NodeFailureNotification * nodeFailure){
	ASSERT(nodeFailure != NULL);
	// check the validity of our data structures

	if(nodeFailure->getFailedNodeID() > ShardManager::getCurrentNodeId()){
		return this;
	}

	unsigned failedNodeIndex = 0 ;
	for(failedNodeIndex = 0 ; failedNodeIndex < allNodesUpToCurrentNode.size(); ++failedNodeIndex){
		if(allNodesUpToCurrentNode.at(failedNodeIndex) == nodeFailure->getFailedNodeID()){
			break;
		}
	}
	ASSERT(failedNodeIndex < allNodesUpToCurrentNode.size());
	if(failedNodeIndex >= allNodesUpToCurrentNode.size()){
		return this;
	}
	// failedNodeIndex gives the location of the failed node.
	// we are passed the failed node, just remove it from vector and
	// decrement nodeIndex
	if(failedNodeIndex < nodeIndex){
		allNodesUpToCurrentNode.erase(allNodesUpToCurrentNode.begin() + failedNodeIndex);
		nodeIndex --;
		ASSERT(nodeIndex < allNodesUpToCurrentNode.size());
	}else if(failedNodeIndex == nodeIndex){
		// We need to resend the request because now nodeIndex is pointing to a new
		// place in the list of nodes.
		allNodesUpToCurrentNode.erase(allNodesUpToCurrentNode.begin() + failedNodeIndex);
		if(nodeIndex < allNodesUpToCurrentNode.size()){
			this->send(newNodeLockNotification, NodeOperationId(allNodesUpToCurrentNode.at(this->nodeIndex)));
		}
	}else{
		ASSERT(failedNodeIndex > nodeIndex);
		// just erase it from the vector
		allNodesUpToCurrentNode.erase(allNodesUpToCurrentNode.begin() + failedNodeIndex);
		ASSERT(nodeIndex < allNodesUpToCurrentNode.size());
	}

	// if we have reached to self:
	if(this->nodeIndex >= allNodesUpToCurrentNode.size()){
		// we reached to ourself. We can continue we initializing our repository
		// and that's it. we are done. we can assume we have the locks.
		if(lastShardLockRepository != NULL){ // it can be NULL because maybe even before getting the reply
			                                 // from the first node, it crashed.
			ShardManager::getShardManager()->getLockManager()->setLockHolderRepository(lastShardLockRepository);
			lastShardLockRepository = NULL;
		}
		// Commit the metadata, because maybe lock values on readview need to change
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
		// we are done.
		// transit to the join operation of new node.
		return NULL;
	}
	return this;
}

OperationState * NewNodeLockOperation::handle(NewNodeLockNotification::ACK * ack){
	if(! doesExpect(ack) ){
		return this;
	}
	this->nodeIndex ++;

	if(lastShardLockRepository != NULL){
		delete lastShardLockRepository;
	}
	lastShardLockRepository = ack->getShardLockRepository();
	ASSERT(lastShardLockRepository != NULL);

	if(nodeIndex >= allNodesUpToCurrentNode.size()){
		// we reached to ourself. We can continue by initializing our repository
		// and that's it. we are done. we can assume we have the locks.
		ShardManager::getShardManager()->getLockManager()->setLockHolderRepository(lastShardLockRepository);
		lastShardLockRepository = NULL; // so that we don't deallocate it in destructor.
		// Commit the metadata, because maybe lock values on readview need to change
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
		// we are done.
		// transit to the join operation of new node.
		return NULL;
	}

	this->send(newNodeLockNotification, NodeOperationId(allNodesUpToCurrentNode.at(this->nodeIndex)));
	return this;

}

string NewNodeLockOperation::getOperationName() const {
	return "new_node_lock";
};
string NewNodeLockOperation::getOperationStatus() const {

	stringstream ss;
	ss << "All nodes up to this node : % ++ " ;
	for(unsigned i  = 0 ; i < allNodesUpToCurrentNode.size(); ++i){
		if(i != 0){
			ss << " - ";
		}

		ss << i << ":" << allNodesUpToCurrentNode.at(i) ;
	}
	ss << " ++%";
	ss << "Node Index : " << nodeIndex << "%";

	ss << "Lock request : ";
	if(lockRequests == NULL){
		ss << "NULL%";
	}else{
		ss << "%";
		ss << lockRequests->toString();
	}
	return ss.str();
};

bool NewNodeLockOperation::doesExpect(NewNodeLockNotification::ACK * ack) const{
	if(ack == NULL){
		ASSERT(false);
		return false;
	}
	if(nodeIndex >= allNodesUpToCurrentNode.size()){
		return false;
	}
	return (ack->getSrc().nodeId == allNodesUpToCurrentNode.at(nodeIndex));
}


}
}
