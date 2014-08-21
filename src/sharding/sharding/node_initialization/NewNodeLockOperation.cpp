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
	writeview->beginClusterShardsIteration();
	while(writeview->getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(isLocal){
			lockTypes[id] = ResourceLockType_X;
		}else{
			lockTypes[id] = ResourceLockType_S;
		}
		lockRequests->requestBatch.push_back(new SingleResourceLockRequest(id, NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()), lockTypes[id]));
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
	writeview->getArrivedNodes(allNodesUpToCurrentNode);
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
	// check the validity of our data structures
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	map<NodeId, ShardingNodeState> nodeStates;
	map<NodeId, std::pair<ShardingNodeState, Node *> > & allNodes = writeview->nodes;
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr =
			allNodes.begin(); nodeItr != allNodes.end(); ++nodeItr){
		nodeStates[nodeItr->first] = nodeItr->second.first;
	}

	vector<NodeId> allNodesUpToCurrentNodeFixed;
	unsigned nodeIndexFixed = this->nodeIndex;
	bool mustRetry = false;
	for(unsigned i = 0 ; i < allNodesUpToCurrentNode.size() ; ++i){
		NodeId clusterNode = allNodesUpToCurrentNode.at(i);
		if(nodeStates[clusterNode] == ShardingNodeStateFailed){
			if(i < this->nodeIndex){ // we are passed that node so we don't care
				nodeIndexFixed--;
			}else if( i == this->nodeIndex){ // we are waiting for this ack, must resend the notification
				nodeIndexFixed ++; // we must retry on the next node
				mustRetry = true;
			}//else{
			//// i > this->nodeIndex : We haven't reached this node, so we are fine.
			//}
		}else{
			allNodesUpToCurrentNodeFixed.push_back(clusterNode);
		}
	}
	this->allNodesUpToCurrentNode = allNodesUpToCurrentNodeFixed;
	this->nodeIndex = nodeIndexFixed;

	// if we have reached to self:
	if(this->nodeIndex >= allNodesUpToCurrentNode.size()){
		// we reached to ourself. We can continue we initializing our repository
		// and that's it. we are done. we can assume we have the locks.
		ShardManager::getShardManager()->getLockManager()->setLockHolderRepository(lastShardLockRepository);
		// Commit the metadata, because maybe lock values on readview need to change
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
		// we are done.
		// transit to the join operation of new node.
		return NULL;
	}

	if(mustRetry){
		this->send(newNodeLockNotification, NodeOperationId(allNodesUpToCurrentNode.at(this->nodeIndex)));
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

	if(nodeIndex >= allNodesUpToCurrentNode.size()){
		// we reached to ourself. We can continue we initializing our repository
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

bool NewNodeLockOperation::doesExpect(NewNodeLockNotification::ACK * ack) const{
	if(ack == NULL){
		ASSERT(false);
		return false;
	}
	if(nodeIndex >= allNodesUpToCurrentNode.size()){
		return false;
	}
	return ack->getSrc().nodeId == allNodesUpToCurrentNode.at(nodeIndex);
}


}
}
