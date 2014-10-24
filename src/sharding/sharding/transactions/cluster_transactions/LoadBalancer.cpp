#include "LoadBalancer.h"


#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../../../configuration/ShardingConstants.h"
#include "../../metadata_manager/Cluster_Writeview.h"
#include "../../ShardManager.h"
#include "../ShardAssignOperation.h"
#include "../ShardCopyOperation.h"
#include "../ShardMoveOperation.h"
#include "../TransactionSession.h"
#include "../../state_machine/StateMachine.h"
#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../../lock_manager/LockManager.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


// only non pending partitions
AssignCandidatePartition::AssignCandidatePartition(const ClusterPID & pid){
	this->pid = pid;
}
unsigned AssignCandidatePartition::getAttentionNeedScore() const{
	return assignCandidateReplicas.size();
}
void AssignCandidatePartition::removeUnavailableReadyReplicas(){
	vector<std::pair<ClusterShardId, NodeId> > readyReplicasCopy;
	for(unsigned i = 0 ; i < readyReplicas.size(); ++i){
		bool canAcquireSLock = ShardManager::getShardManager()->_getLockManager()->canAcquireLock(readyReplicas.at(i).first, LockLevel_S);;
		if(canAcquireSLock){
			readyReplicasCopy.push_back(readyReplicas.at(i));
		}
	}
	readyReplicas = readyReplicasCopy;
}
bool AssignCandidatePartition::hasReplicaOnNode(NodeId nodeId) const{
	for(unsigned i = 0 ; i < readyReplicas.size(); ++i){
		if(readyReplicas.at(i).second == nodeId){
			return true;
		}
	}
	return false;
}
inline bool AssignCandidatePartition::operator()(const AssignCandidatePartition& a, const AssignCandidatePartition& b)
{
	return a.getAttentionNeedScore() >= b.getAttentionNeedScore();
}


void LoadBalancer::runLoadBalancer(){
	LoadBalancer * loadBalancer = new LoadBalancer();
	Transaction::startTransaction(loadBalancer);
}

LoadBalancer::~LoadBalancer(){
	if(shardAssigner != NULL){
		delete shardAssigner;
	}
	if(shardCopyer != NULL){
		delete shardCopyer;
	}
	if (shardMover != NULL){
		delete shardMover;
	}
}

void LoadBalancer::initSession(){
	TransactionSession * session = new TransactionSession();
	// TODO : we don't give response or readview to shard copy an#include "../state_machine/State.h"d shard move
	setSession(session);
}

// tries to balance the shard placement on cluster nodes
bool LoadBalancer::run(){
	collectInfo();
	return true;
}

// asks other nodes about load information
void LoadBalancer::collectInfo(){
	reportReq = SP(LoadBalancingReport::REQUEST)(new LoadBalancingReport::REQUEST());
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	vector<NodeId> allNodes;
	writeview->getArrivedNodes(allNodes, true);
	ConcurrentNotifOperation * commandSender =
			new ConcurrentNotifOperation(reportReq, ShardingLoadBalancingReportMessageType, allNodes, this);
	this->currentOp = CollectInfo;
	ShardManager::getShardManager()->getStateMachine()->registerOperation(commandSender);
}
void LoadBalancer::end_(map<NodeOperationId, SP(ShardingNotification) > & replies){

	for(map<NodeOperationId , SP(ShardingNotification)>::iterator replyItr = replies.begin();
			replyItr != replies.end(); ++replyItr){
		nodeLoads[replyItr->first.nodeId] = boost::dynamic_pointer_cast<LoadBalancingReport>(replyItr->second)->getLoad();
	}
	balance();
}

// decides on shard copy or shard movements
void LoadBalancer::balance(){
	if(nodeLoads.size() == 0){
		finalize();
	}
	if(! canAcceptMoreShards(ShardManager::getCurrentNodeId())){
		finalize();
	}

	if(nodeLoads.size() == 1){ // only us or not light
		tryShardAssginmentAndShardCopy(true);// assignment only
		if(this->shardAssigner != NULL){
			currentOp = Assign;
			shardAssigner->produce();
			return;
		}
		// only us and no unassigned shard can be assigned to us
		finalize();
		return;
	}

	tryShardAssginmentAndShardCopy();
	if(shardCopyer != NULL){
		currentOp = Copy;
		shardCopyer->produce();
		return;
	}

	// no load balancing can be done by shard copy. There's no available unassigned shards.
	// find a heavy loaded node that has a shard that doesn't have a replica on us
	// and move that shard to us.

	tryShardMove();
	if(shardMover != NULL){
		currentOp = Move;
		shardMover->produce();
		return;
	}
	finalize();
	return;
}




// receives the results of ShardAssign, ShardMove and ShardCopy
void LoadBalancer::consume(bool done){

	if(currentOp == Assign){
		Logger::debug("Load balancing : Shard assignment was %s", done?"successful." : "failed.");
	}else if(currentOp == Copy){
		Logger::debug("Load balancing : Shard copy was %s", done?"successful." : "failed.");
	}else if (currentOp == Move){
		Logger::debug("Load balancing : Shard move was %s", done?"successful." : "failed.");
	}
	finalize();
}

ShardingTransactionType LoadBalancer::getTransactionType(){
	return ShardingTransactionType_Loadbalancing;
}

void LoadBalancer::finalize(){

	this->setFinished();

	ShardManager::getShardManager()->resetLoadBalancing();
	return;
}

LoadBalancer::LoadBalancer(){
	this->shardAssigner = NULL;
	this->shardCopyer = NULL;
	this->shardMover = NULL;
	this->currentOp = PreStart;
}


void LoadBalancer::tryShardMove(){
	vector<std::pair<NodeId, ClusterShardId> > candidateSrcShards;
	prepareMoveCandidates(candidateSrcShards);
	if(candidateSrcShards.size() == 0){
		return;
	}
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	srand(ShardManager::getCurrentNodeId());
	unsigned moveOptionIndex = rand() % candidateSrcShards.size();

	shardMover = moveShard(candidateSrcShards.at(moveOptionIndex).first, candidateSrcShards.at(moveOptionIndex).second);

}

void LoadBalancer::tryShardAssginmentAndShardCopy(bool assignOnly){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	/*
	 * 1. UNASSIGNED shards
	 *  First, get a list of pairs of <ClusterShardId,<list of nodes>>
	 *  that is valid for our calculations.
	 *  valid means :
	 *          I)   The shard is UNASSIGNED
	 *  		II)  the partition of the shard is not pending.
	 *  		III) acquiring X lock on local node on this shard
	 *  		     is possible (don't acquire, just check.)
	 *  		IV)  The second value of pair is the list of all nodes
	 *  		     that have a READY replica of that ClusterShardId
	 *  		     *** if that vector is empty, this patition is UNASSIGNED
	 */
	vector<AssignCandidatePartition *> assignCandidates;
	prepareAssignmentCandidates(assignCandidates);


	// do we have critical partitions ?
	vector<ClusterPID> totallyUnassignedPartitions;
	for(unsigned i = 0 ; i < assignCandidates.size(); ++i){
		if(assignCandidates.at(i)->readyReplicas.size() == 0){
			if(find(totallyUnassignedPartitions.begin(),
					totallyUnassignedPartitions.end(), assignCandidates.at(i)->pid) == totallyUnassignedPartitions.end()){
				totallyUnassignedPartitions.push_back(assignCandidates.at(i)->pid);
			}
		}
	}
	if(totallyUnassignedPartitions.size() > 0){
		srand(ShardManager::getCurrentNodeId());
		unsigned assignOptionIndex = rand() % totallyUnassignedPartitions.size();
		deleteAssignmentCandidates(assignCandidates);
		shardAssigner = assignShard(totallyUnassignedPartitions.at(assignOptionIndex)); // we first assign the primary shard
		return;
	}
	if(assignOnly){
		return;
	}

	// all partitions at least have one alive replica
	// if some replica resides on current node: remove that candidate from this list at this point.
	vector<AssignCandidatePartition *> assignCandidatesNotOnLocalNode;
	for(unsigned i = 0 ; i < assignCandidates.size() ; ++i){
		if(! assignCandidates.at(i)->hasReplicaOnNode(ShardManager::getCurrentNodeId())){
		    // those replicas that we have a local lock them and
		    // we already know we cannot use them...
			assignCandidates.at(i)->removeUnavailableReadyReplicas();
			if(assignCandidates.at(i)->readyReplicas.size() > 0){
				assignCandidatesNotOnLocalNode.push_back(assignCandidates.at(i));
			}
		}
	}

	if(assignCandidatesNotOnLocalNode.size() > 0){
		// now, choose a partition randomly.
		srand(ShardManager::getCurrentNodeId());
		unsigned partitionAssignCandidIndex = rand() % assignCandidatesNotOnLocalNode.size();
		AssignCandidatePartition * candidatePartition = assignCandidatesNotOnLocalNode.at(partitionAssignCandidIndex);
		// and now choose a shard from this partition, and a node
		// to get copy from.
		ClusterShardId shardToAssign = candidatePartition->assignCandidateReplicas.at(0);
		// and choose a src node to get copy from
		unsigned readyReplicaIndex = rand() % candidatePartition->readyReplicas.size();
		ClusterShardId shardToReplicate = candidatePartition->readyReplicas.at(readyReplicaIndex).first;
		NodeId srcNodeId = candidatePartition->readyReplicas.at(readyReplicaIndex).second;
		deleteAssignmentCandidates(assignCandidates);
		shardCopyer = replicateShard(shardToAssign, srcNodeId, shardToReplicate);
		return;
	}
	deleteAssignmentCandidates(assignCandidates);
	return;
}


/*
 *          I)   The shard is UNASSIGNED
 *  		II)  the partition of the shard is not pending.
 *  		III) acquiring X lock on local node on this shard
 *  		     is possible (don't acquire, just check.)
 *  		IV)  The second value of pair is the list of all nodes
 *  		     that have a READY replica of that ClusterShardId
 *  		     *** if that vector is empty, this partition is UNASSIGNED
 */
void LoadBalancer::prepareAssignmentCandidates(vector<AssignCandidatePartition *> & assignCandidates){
	Cluster_Writeview * writeview = ShardManager::getWriteview();


	map<ClusterPID, AssignCandidatePartition *> partitionCandidates;

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
		if(state != SHARDSTATE_UNASSIGNED){
			continue;
		}

		if(writeview->isPartitionPending(id.getPartitionId())){
			continue;
		}

		bool canAcquireXLock = ShardManager::getShardManager()->_getLockManager()->canAcquireLock(id, LockLevel_X);
		if(! canAcquireXLock){
			continue;
		}
		if(partitionCandidates.find(id.getPartitionId()) == partitionCandidates.end()){
			partitionCandidates[id.getPartitionId()] = new AssignCandidatePartition(id.getPartitionId());
		}
		partitionCandidates[id.getPartitionId()]->assignCandidateReplicas.push_back(id);
	}

	// now add ready replicas
    ClusterShardIterator cShardItr2(writeview);
    cShardItr2.beginClusterShardsIteration();
	while(cShardItr2.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(state != SHARDSTATE_READY){
			continue;
		}

		// we save all ready replicas here but later we only consider those that are X locked.
		if(partitionCandidates.find(id.getPartitionId()) != partitionCandidates.end()){
			partitionCandidates[id.getPartitionId()]->readyReplicas.push_back(std::make_pair(id, nodeId));
		}
	}


	for(map<ClusterPID, AssignCandidatePartition *>::iterator pidItr = partitionCandidates.begin();
			pidItr != partitionCandidates.end(); ++pidItr){
		assignCandidates.push_back(pidItr->second);
	}
}

void LoadBalancer::deleteAssignmentCandidates(vector<AssignCandidatePartition *> & assignCandidates){
	for(unsigned i = 0 ; i < assignCandidates.size(); ++i){
		delete assignCandidates.at(i);
	}
}

/*
 * I)   The shard is READY
 * II)  There is no replica of the shard on current node
 * III) There is no X or U lock on local repository.
 * IV)  This shard is not the only READY shard of its partition
 * V)   The srcNodeId must have a load greater than or equal to 2 + local load
 */
void LoadBalancer::prepareMoveCandidates(vector<std::pair<NodeId, ClusterShardId> > & candidateSrcShards){
	Cluster_Writeview * writeview = ShardManager::getWriteview();


	ClusterShardId id;
	NodeShardId nodeShardId;
	ShardState state;
	bool isLocal;
	NodeId nodeId;
	LocalPhysicalShard physicalShard;
	double load;

	vector< std::pair<NodeId, ClusterShardId> > candidates;
	map<ClusterPID, unsigned> partitionNumberOfReadyReplicas;
	set<ClusterPID> partitionHasReplicaOnCurrentNode;

    ClusterShardIterator cShardItr(writeview);
    cShardItr.beginClusterShardsIteration();
	while(cShardItr.getNextClusterShard(id, load, state, isLocal, nodeId)){
		if(state != SHARDSTATE_READY){
			continue;
		}
		if(partitionNumberOfReadyReplicas.find(id.getPartitionId()) == partitionNumberOfReadyReplicas.end()){
			partitionNumberOfReadyReplicas[id.getPartitionId()] = 1;
		}else{
			partitionNumberOfReadyReplicas[id.getPartitionId()] ++;
		}

		if(nodeId == ShardManager::getCurrentNodeId()){
			partitionHasReplicaOnCurrentNode.insert(id.getPartitionId());
			continue;
		}

		bool canAcquireULock = ShardManager::getShardManager()->_getLockManager()->canAcquireLock(id, LockLevel_X);
		if(! canAcquireULock){
			continue;
		}
		if(nodeLoads[nodeId] < 2 + nodeLoads[ShardManager::getCurrentNodeId()]){
			continue;
		}
		candidates.push_back(std::make_pair(nodeId, id));
	}

	for(unsigned i = 0 ; i < candidates.size(); ++i){
		if(partitionNumberOfReadyReplicas[candidates.at(i).second.getPartitionId()] >= 2){
			if(partitionHasReplicaOnCurrentNode.count(candidates.at(i).second.getPartitionId()) == 0){
				candidateSrcShards.push_back(candidates.at(i));
			}
		}
	}
}

bool LoadBalancer::canAcceptMoreShards(NodeId nodeId){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	double loadAvg = 0;
	for(map<NodeId, double>::iterator nodeLoadItr = nodeLoads.begin(); nodeLoadItr != nodeLoads.end(); ++nodeLoadItr){
		loadAvg += nodeLoadItr->second;
	}
	loadAvg = loadAvg / nodeLoads.size();

	if(nodeLoads.find(nodeId) == nodeLoads.end()){
		return false;
	}
	return (nodeLoads[nodeId] <= loadAvg) ;
}

// The first shard of a partition
ShardAssignOperation * LoadBalancer::assignShard(const ClusterShardId & unassignedShard){
    Logger::debug("Load balancing | Going to assign shard %s to node %d.", unassignedShard.toString().c_str() , ShardManager::getCurrentNodeId());
//	return new ShardAssignOperation(this->getOperationId(), unassignedShard);
    return new ShardAssignOperation(unassignedShard, this);
}
ShardCopyOperation * LoadBalancer::replicateShard(const ClusterShardId & unassignedShard, NodeId srcNodeId, const ClusterShardId & shardToReplicate){
    Logger::debug("Load balancing | Going to copy shard %s from node %d to use for unassigned shard %s which will reside on node %d",
            shardToReplicate.toString().c_str(), srcNodeId, unassignedShard.toString().c_str() , ShardManager::getCurrentNodeId());
//    return new ShardCopyOperation(this->getOperationId(), unassignedShard, srcNodeId, shardToReplicate);
    return new ShardCopyOperation(unassignedShard, srcNodeId, shardToReplicate, this);
}

ShardMoveOperation * LoadBalancer::moveShard(const NodeId & srcNodeId, const ClusterShardId & moveShardId){
    Logger::debug("Load balancing | Going to move shard %s from node %d to node %d",
            moveShardId.toString().c_str(), srcNodeId, ShardManager::getCurrentNodeId());
	return new ShardMoveOperation(srcNodeId, moveShardId, this);
}

}
}
