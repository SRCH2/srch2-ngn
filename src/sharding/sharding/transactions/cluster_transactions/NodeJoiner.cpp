
#include "NodeJoiner.h"
#include "../../metadata_manager/MetadataInitializer.h"
#include "../AtomicMetadataCommit.h"
#include "../AtomicLock.h"
#include "../AtomicRelease.h"

#include "core/util/Logger.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {



void NodeJoiner::run(){
	NodeJoiner * joiner = new NodeJoiner();
	ShardManager::getShardManager()->getStateMachine()->registerTransaction(this);
	joiner->join();
}


NodeJoiner::~NodeJoiner(){
	if(locker != NULL){
		delete locker;
	}
	if( readMetadataNotif != NULL){
		delete readMetadataNotif;
	}
	if(commitNotification != NULL){
		delete commitNotification;
	}
	if(releaser != NULL){
		delete releaser;
	}
	if(metadataChange != NULL){
		delete metadataChange;
	}
	if(committer != NULL){
		delete committer;
	}
}

NodeJoiner::NodeJoiner(){
	this->finalizedFlag = false;
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), this->getTID());
	this->randomNodeToReadFrom = 0;
	this->locker = NULL;
	this->readMetadataNotif = new MetadataReport::REQUEST();;
	this->commitNotification = NULL;
	this->releaser = NULL;
	this->metadataChange = NULL;
	this->committer = NULL;
	this->currentOperation = "";
}

ShardingTransactionType NodeJoiner::getTransactionType(){
	return ShardingTransactionType_NodeJoin;
}
void NodeJoiner::join(){
	lock();
}


void NodeJoiner::lock(){ // locks the metadata to be safe to read it

	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);
	//lock should be acquired on all nodes
	locker = new AtomicLock(selfOperationId, this, olderNodes); // X-locks metadata by default
	releaser = new AtomicRelease(selfOperationId, this);
	Logger::debug("Acquiring lock on metadata ...");
	this->currentOperation = "lock";
	locker->produce();
}

// coming back from lock
void NodeJoiner::consume(bool granted){
	if(this->currentOperation.compare("lock")){ // lock
		if(! granted){
			ASSERT(false);
			Logger::error("New node could not join the cluster.");
			finalize(false);
		}else{
			readMetadata();
		}
	}else if(this->currentOperation.compare("readmetadata")){ // read metadata
		ASSERT(false);
	}else if(this->currentOperation.compare("commit")){ // commit
		if(! granted){
			Logger::debug("New node booting a fresh cluster because commit operation failed.");
			ASSERT(false);
			finalize(false);
			return;
		}else{
			release();
		}
	}else if(this->currentOperation.compare("release")){ // release
		if(! granted){
			Logger::debug("New node booting a fresh cluster because release operation failed.");
			ASSERT(false);
			finalize(false);
			return;
		}else{
			finalize();
		}
	}else{
		Logger::debug("New node booting a fresh cluster because of unknown reason.");
		ASSERT(false); //
		finalize(false);
	}
}

void NodeJoiner::readMetadata(){ // read the metadata of the cluster
	Logger::debug("Reading metadata writeview ...");
	// send read_metadata notification to the smallest node id
	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);

	if(olderNodes.size() == 0){
		Logger::info("New node booting up a fresh cluster ...");
		finalize(false);
		return;
	}

	srand(time(NULL));
	this->randomNodeToReadFrom = olderNodes.at(rand() % olderNodes.size());

	ConcurrentNotifOperation * reader = new ConcurrentNotifOperation(readMetadataNotif,
			ShardingNewNodeReadMetadataReplyMessageType, randomNodeToReadFrom, this);
	this->currentOperation = "readmetadata";
	// committer is deallocated in state-machine so we don't have to have the
	// pointer. Before deleting committer, state-machine calls it's getMainTransactionId()
	// which calls lastCallback from its consumer
	ShardManager::getShardManager()->getStateMachine()->registerOperation(reader);
}

bool NodeJoiner::shouldAbort(const NodeId & failedNode){
	if(randomNodeToReadFrom == failedNode){
		readMetadata();
		return true;
	}
	return false;
}
// if returns true, operation must stop and return null to state_machine
void NodeJoiner::end_(map<NodeOperationId, ShardingNotification * > & replies){
	if(replies.size() != 1){
		ASSERT(false);
		finalize(false);
		return;
	}
	ASSERT(replies.begin()->first == randomNodeToReadFrom);
	MetadataReport * metadataReport = (MetadataReport * )replies.begin()->second;
	Cluster_Writeview * clusterWriteview = metadataReport->getWriteview();
	if(clusterWriteview == NULL){
		ASSERT(false);
		//use metadata initializer to make sure no partition remains unassigned.
		finalize(false);
		return;
	}
	Cluster_Writeview * currentWriteview = ShardManager::getWriteview();
	// attach data pointers of current writeview to the new writeview coming from
	// the minID node.
	currentWriteview->fixClusterMetadataOfAnotherNode(clusterWriteview);

	// new writeview is ready, replace current writeview with the new one
	ShardManager::getShardManager()->getMetadataManager()->setWriteview(clusterWriteview);
	ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
	Logger::debug("Metadata initialized from the cluster.");
	// ready to commit.
	commit();
}

void NodeJoiner::commit(){
	Logger::debug("Committing the new node change to the cluster ...");
	// prepare the commit operation
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	vector<ClusterShardId> localClusterShards;
	vector<NodeShardId> nodeShardIds;
	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;
	ClusterShardIterator cShardItr(writeview);
	cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextLocalClusterShard(id, load, physicalShard)){
		localClusterShards.push_back(id);
	}

	NodeShardIterator nShardItr(writeview);
	nShardItr.beginNodeShardsIteration();
	while(nShardItr.getNextLocalNodeShard(nodeShardId, load, physicalShard)){
		nodeShardIds.push_back(nodeShardId);
	}

	NodeAddChange * nodeAddChange =
			new NodeAddChange(ShardManager::getCurrentNodeId(),localClusterShards, nodeShardIds);
	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);
	this->committer = new AtomicMetadataCommit(nodeAddChange,  olderNodes, this);
	this->currentOperation = "commit";
	this->committer->produce();
}

void NodeJoiner::release(){ // releases the lock on metadata
	ASSERT(! this->releaseModeFlag);
	this->currentOperation = "release";
	this->releaser->produce();
}

void NodeJoiner::finalize(bool result){
	if(! result){
		ShardManager::getShardManager()->initFirstNode();
		Logger::error("New node booting up as a single node.");
	}else{
	// release is also done.
	// just setJoined and done.
	this->finalizedFlag = true;
	ShardManager::getShardManager()->setJoined();
	Logger::info("Joined to the cluster.");
	}
	// so state machine will deallocate this transaction
	ConsumerInterface::setTransIdToDelete(this->getTID());
}

void NodeJoiner::getOlderNodesList(vector<NodeId> & olderNodes){
	Cluster_Writeview * currentWriteview = ShardManager::getWriteview();
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = currentWriteview->nodes.begin();
			nodeItr != currentWriteview->nodes.end(); ++nodeItr){
		if(nodeItr->first >= currentWriteview->currentNodeId){
			continue;
		}
		if(nodeItr->second.first == ShardingNodeStateFailed){
			continue;
		}
		ASSERT(nodeItr->second.second != NULL); // we must have seen this node
		olderNodes.push_back(nodeItr->first);
	}
	std::sort(olderNodes.begin(), olderNodes.end());

	stringstream ss;
	for(unsigned i = 0 ; i < olderNodes.size(); ++i){
		if(i != 0){
			ss << ",";
		}
		ss << olderNodes.at(i);
	}
	Logger::debug("Sending list of nodes : %s with the lock request.", ss.str().c_str());

	ASSERT(olderNodes.size() > 0 &&
			olderNodes.at(allNodesUpToCurrentNode.size()-1) < ShardManager::getCurrentNodeId());
}

}
}
