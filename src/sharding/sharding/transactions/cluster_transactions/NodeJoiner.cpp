
#include "NodeJoiner.h"
#include "../../metadata_manager/MetadataInitializer.h"
#include "../AtomicMetadataCommit.h"
#include "../AtomicLock.h"
#include "../AtomicRelease.h"
#include "../../state_machine/StateMachine.h"
#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"

#include "core/util/Logger.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {



void NodeJoiner::join(){
	SP(NodeJoiner) joiner = SP(NodeJoiner)(new NodeJoiner());
	Transaction::startTransaction(joiner);
}


NodeJoiner::~NodeJoiner(){
	finalize();
	if(locker != NULL){
		delete locker;
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
	__FUNC_LINE__ // prints the name of file and function (only in debug mode)
}

NodeJoiner::NodeJoiner(){
	this->selfOperationId = NodeOperationId(ShardManager::getCurrentNodeId(), this->getTID());
	__FUNC_LINE__
	Logger::sharding(Logger::Detail, "NodeJoiner| Join operation ID : %s", this->selfOperationId.toString().c_str());
	this->randomNodeToReadFrom = 0;
	this->locker = NULL;
	this->readMetadataNotif = SP(MetadataReport::REQUEST)(new MetadataReport::REQUEST());
	this->releaser = NULL;
	this->metadataChange = NULL;
	this->committer = NULL;
	this->currentOperation = PreStart;
}


SP(Transaction) NodeJoiner::getTransaction() {
	return sharedPointer;
}

ShardingTransactionType NodeJoiner::getTransactionType(){
	return ShardingTransactionType_NodeJoin;
}
void NodeJoiner::run(){
	__FUNC_LINE__
	Logger::sharding(Logger::Step, "NodeJoiner| Starting to join this node ...");
	lock();
	return;
}


void NodeJoiner::lock(){ // locks the metadata to be safe to read it
	__FUNC_LINE__
    Logger::sharding(Logger::Step, "NodeJoiner| Starting to lock the metadata ...");
	vector<NodeId> olderNodes;
	getOlderNodesList(olderNodes);
	//lock should be acquired on all nodes
	locker = new AtomicLock(selfOperationId, this, olderNodes); // X-locks metadata by default
	this->currentOperation = Lock;
	locker->produce();
}

// coming back from lock
void NodeJoiner::consume(bool granted){
	__FUNC_LINE__
    switch (this->currentOperation) {
        case Lock:
            if(! granted){
                ASSERT(false);
                Logger::error("New node could not join the cluster.");
                this->setFinalizeArgument(false, true);
            }else{
                readMetadata();
            }
            break;
        case ReadMetadata:
            ASSERT(false);
            break;
        case Commit:
            if(! granted){
                Logger::sharding(Logger::Step, "NodeJoiner| New node booting a fresh cluster because commit operation failed.");
                ASSERT(false);
                this->setFinalizeArgument(false, true);
            }else{
                release();
            }
            break;
        case Release:
            if(! granted){
                Logger::sharding(Logger::Step, "NodeJoiner| New node booting a fresh cluster because release operation failed.");
                ASSERT(false);
                this->setFinalizeArgument(false, true);
            }else{
                this->setFinalizeArgument(true, true);
            }
            break;
        default:
            Logger::sharding(Logger::Step, "NodeJoiner| New node booting a fresh cluster because of unknown reason.");
            ASSERT(false); //
            this->setFinalizeArgument(false, true);
            break;
    }
}

void NodeJoiner::readMetadata(){ // read the metadata of the cluster
	__FUNC_LINE__
	Logger::sharding(Logger::Step,"NodeJoiner| Reading metadata writeview ...");
	// send read_metadata notification to the smallest node id
	vector<NodeId> olderNodes;
	this->getOlderNodesList(olderNodes);

	if(olderNodes.size() == 0){
		Logger::info("New node booting up a fresh cluster ...");
		Logger::sharding(Logger::Step, "NodeJoiner| no other nodes are left.");
		this->setFinalizeArgument(false, true);
		return;
	}

	srand(time(NULL));
	this->randomNodeToReadFrom = olderNodes.at(rand() % olderNodes.size());

	Logger::sharding(Logger::Detail, "NodeJoiner| Reading metadata from node %d", this->randomNodeToReadFrom);
	ConcurrentNotifOperation * reader = new ConcurrentNotifOperation(readMetadataNotif,
			ShardingNewNodeReadMetadataReplyMessageType, randomNodeToReadFrom, this);
	this->currentOperation = ReadMetadata;
	// committer is deallocated in state-machine so we don't have to have the
	// pointer. Before deleting committer, state-machine calls it's getMainTransactionId()
	// which calls lastCallback from its consumer
	ShardManager::getShardManager()->getStateMachine()->registerOperation(reader);
}

bool NodeJoiner::shouldAbort(const NodeId & failedNode){
	if(randomNodeToReadFrom == failedNode){
		Logger::sharding(Logger::Detail, "NodeJoiner| retrying metadata read.");
		readMetadata();
		return true;
	}
	return false;
}
// if returns true, operation must stop and return null to state_machine
void NodeJoiner::end_(map<NodeOperationId, SP(ShardingNotification) > & replies){
	if(replies.size() > 1){
		ASSERT(false);
		this->setFinalizeArgument(false, false);
		return;
	}
	if(replies.size() < 1){
		Logger::sharding(Logger::Warning, "NodeJoiner|  Node Joiner : reading metadata failed.");
		readMetadata();
		return;
	}
	ASSERT(replies.begin()->first == randomNodeToReadFrom);
	Logger::sharding(Logger::Step, "NodeJoiner|  Node Joiner : metadata is read from node %d", randomNodeToReadFrom);
	SP(MetadataReport) metadataReport = boost::dynamic_pointer_cast<MetadataReport>(replies.begin()->second);
	Cluster_Writeview * clusterWriteview = metadataReport->getWriteview();
	if(clusterWriteview == NULL){
		ASSERT(false);
		//use metadata initializer to make sure no partition remains unassigned.
		this->setFinalizeArgument(false, true);
		return;
	}
	Cluster_Writeview * currentWriteview = this->getWriteview();
	// attach data pointers of current writeview to the new writeview coming from
	// the minID node.
	currentWriteview->fixClusterMetadataOfAnotherNode(clusterWriteview);
	// new writeview is ready, replace current writeview with the new one
	SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
	ShardManager::getShardManager()->getMetadataManager()->setWriteview(clusterWriteview, false);
	this->setWriteview(clusterWriteview);
	// update the readview
	ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata(false);
	nodesWriteview.reset();
	Logger::sharding(Logger::Detail, "NodeJoiner| Metadata initialized from the cluster.");
	// ready to commit.
	commit();
}

void NodeJoiner::commit(){
	__FUNC_LINE__
	Logger::sharding(Logger::Step, "NodeJoiner| Committing the new node change to the cluster ...");
	// prepare the commit operation
	const Cluster_Writeview * writeview = this->getWriteview();
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
	this->committer = new AtomicMetadataCommit(nodeAddChange,  olderNodes, this, true);
	this->currentOperation = Commit;
	this->committer->produce();
}

void NodeJoiner::release(){ // releases the lock on metadata
	__FUNC_LINE__
	releaser = new AtomicRelease(selfOperationId, this);
	if(! this->releaser->updateParticipants()){
		this->setFinalizeArgument(false, true);
		return;
	}
	this->currentOperation = Release;
	Logger::sharding(Logger::Step, "NodeJoiner| Releasing lock ...");
	this->releaser->produce();
}

void NodeJoiner::finalizeWork(Transaction::Params * arg){
	bool result = false;
	if(arg != NULL){
		result = arg->shouldLock; // if it's passed to finalize, this boolean is interpreted as finalizeResult
	}
	if(! result){
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		ShardManager::getShardManager()->initFirstNode(false);
		Logger::error("New node booting up as a single node to form a cluster.");
		Logger::sharding(Logger::Error, "NodeJoiner| New node booting up as a single node to form a cluster.");
	}else{
		// release is also done.
		// just setJoined and done.
		ShardManager::getShardManager()->setJoined();
		Logger::info("Joined to the cluster.");
		Logger::sharding(Logger::Step, "NodeJoiner| Joined. Done.");
	}
}

void NodeJoiner::getOlderNodesList(vector<NodeId> & olderNodes){
	SP(const ClusterNodes_Writeview) nodeWriteview = this->getNodesWriteview_read();
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodeWriteview->getNodes_read().begin();
			nodeItr != nodeWriteview->getNodes_read().end(); ++nodeItr){
		if(nodeItr->first >= nodeWriteview->currentNodeId){
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
	Logger::sharding(Logger::Detail, "NodeJoiner| List of older nodes: %s", ss.str().c_str());

	ASSERT(olderNodes.size() > 0 &&
			olderNodes.at(olderNodes.size()-1) < ShardManager::getCurrentNodeId());
}

}
}
