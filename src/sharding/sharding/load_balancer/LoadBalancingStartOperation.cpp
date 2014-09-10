#include "LoadBalancingStartOperation.h"


#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../../configuration/ShardingConstants.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "ShardAssignOperation.h"
#include "ShardCopyOperation.h"
#include "ShardMoveOperation.h"

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
		bool canAcquireSLock = ShardManager::getShardManager()->getLockManager()->canAcquireLock(readyReplicas.at(i).first, ResourceLockType_S);;
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


OperationState * LoadBalancingStartOperation::entry(){

	Cluster_Writeview * writeview = ShardManager::getWriteview();
	// either returns a chain of operations to start a load balancing activity or returns NULL.
	// Send a ReportRequest and collect load information of all nodes.
	LoadBalancingReport::REQUEST * reportReq = new LoadBalancingReport::REQUEST();
	vector<NodeId> allNodes;
	writeview->getArrivedNodes(allNodes, true);
	if(allNodes.size() == 1){
		ASSERT(allNodes.at(0) == ShardManager::getCurrentNodeId());
		return LoadBalancingStartOperation::finalizeLoadBalancing();
	}
	for(vector<NodeId>::iterator nodeItr = allNodes.begin(); nodeItr != allNodes.end(); ++nodeItr){
		if(*nodeItr == ShardManager::getCurrentNodeId()){
			// get the load value of this node and add it to the map.
			nodeLoads[ShardManager::getCurrentNodeId()] = writeview->getLocalNodeTotalLoad();
			nodeReportArrived[*nodeItr] = true;
		}else{
			this->send(reportReq, NodeOperationId(*nodeItr));
			nodeReportArrived[*nodeItr] = false;
			nodeLoads[*nodeItr] = 0;
		}
	}
	delete reportReq;
	return this;
}

OperationState * LoadBalancingStartOperation::handle(LoadBalancingReport * n){
	// REPORT
	double newLoad = n->getLoad();
	nodeReportArrived[n->getSrc().nodeId] = true;
	nodeLoads[n->getSrc().nodeId] = newLoad;

	update();
	return balance();
}
OperationState * LoadBalancingStartOperation::handle(NodeFailureNotification * n){

    update();

	return balance();
}

OperationState * LoadBalancingStartOperation::handle(Notification * n){
	if(n == NULL){
		ASSERT(false);
		return LoadBalancingStartOperation::finalizeLoadBalancing();
	}
	switch(n->messageType()){
	case ShardingNodeFailureNotificationMessageType:
		return handle((NodeFailureNotification *)n);
	case ShardingLoadBalancingReportMessageType:
		return handle((LoadBalancingReport *)n);
	default:
		// ignore;
		return this;
	}
}

OperationState * LoadBalancingStartOperation::finalizeLoadBalancing(){
	ShardManager::getShardManager()->resetLoadBalancing();
	return NULL;
}

OperationState * LoadBalancingStartOperation::balance(){
	if(nodeReportArrived.size() < 2){
		// only us ?
		return LoadBalancingStartOperation::finalizeLoadBalancing();
	}
	// do we have all load reports ?
	if(haveAllReportsArrived()){
		Cluster_Writeview * writeview = ShardManager::getWriteview();
		if(! isLightLoadedNode(ShardManager::getCurrentNodeId())){
			OperationState * assignCopy = tryShardAssginmentAndShardCopy(true);// assignment only
			if(assignCopy != NULL){
				return assignCopy;
			}
			return LoadBalancingStartOperation::finalizeLoadBalancing();
		}

		OperationState * assignCopy = tryShardAssginmentAndShardCopy();
		if(assignCopy != NULL){
			return assignCopy;
		}

		// no load balancing can be done by shard copy. There's no available unassigned shards.
		// find a heavy loaded node that has a shard that doesn't have a replica on us
		// and move that shard to us.

		OperationState * shardMove = tryShardMove();
		if(shardMove == NULL){
			return LoadBalancingStartOperation::finalizeLoadBalancing();
		}else{
			return shardMove;
		}
	}
	return this;
}

void LoadBalancingStartOperation::update(){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::iterator nodeItr = writeview->nodes.begin();
			nodeItr != writeview->nodes.end(); ++nodeItr){
		if(nodeItr->second.first == ShardingNodeStateFailed){
			if(nodeReportArrived.find(nodeItr->first) != nodeReportArrived.end()){
				nodeReportArrived.erase(nodeItr->first);
				nodeLoads.erase(nodeItr->first);
			}
		}

		if(nodeItr->second.first == ShardingNodeStateArrived){
			if(nodeReportArrived.find(nodeItr->first) == nodeReportArrived.end()){
				if(nodeItr->first == ShardManager::getCurrentNodeId()){
					ASSERT(false);
				}else{
					LoadBalancingReport::REQUEST * reportReq = new LoadBalancingReport::REQUEST();
					this->send(reportReq, NodeOperationId(nodeItr->first));
					nodeReportArrived[nodeItr->first] = false;
					nodeLoads[nodeItr->first] = 0;
				}
			}
		}

	}
}

string LoadBalancingStartOperation::getOperationName() const {
	return "load_balancing_check";
}
string LoadBalancingStartOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Node reports: ";
	if(nodeReportArrived.size() == 0){
		ss << "empty. %";
	}else{
		ss << "%";
	}
	for(map<NodeId, bool>::const_iterator nodeItr = nodeReportArrived.begin();
			nodeItr != nodeReportArrived.end(); ++ nodeItr){
		ss << "Node : " << nodeItr->first;
		if(nodeItr->second){
			ss << " Arrvied.%";
		}else{
			ss << " Not arrived.%";
		}
	}
	ss << "Node loads: ";
	if(nodeLoads.size() == 0){
		ss << "empty. %";
	}else{
		ss << "%";
	}
	for(map<NodeId, double>::const_iterator nodeItr = nodeLoads.begin();
			nodeItr != nodeLoads.end(); ++ nodeItr){
		ss << "Node : " << nodeItr->first << ", load : " << nodeItr->second << "%";
	}
	return ss.str();
}

bool LoadBalancingStartOperation::haveAllReportsArrived(){
	for(map<NodeId, bool>::iterator nodeItr = nodeReportArrived.begin(); nodeItr != nodeReportArrived.end(); ++nodeItr){
		if(! nodeItr->second){
			return false;
		}
	}
	return true;
}

OperationState * LoadBalancingStartOperation::tryShardMove(){
	vector<std::pair<NodeId, ClusterShardId> > candidateSrcShards;
	prepareMoveCandidates(candidateSrcShards);
	if(candidateSrcShards.size() == 0){
		return NULL;
	}
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	srand(ShardManager::getCurrentNodeId());
	unsigned moveOptionIndex = rand() % candidateSrcShards.size();

	return moveShard(candidateSrcShards.at(moveOptionIndex).first, candidateSrcShards.at(moveOptionIndex).second);

}

OperationState * LoadBalancingStartOperation::tryShardAssginmentAndShardCopy(bool assignOnly){
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
		return assignShard(totallyUnassignedPartitions.at(assignOptionIndex)); // we first assign the primary shard
	}
	if(assignOnly){
		return NULL;
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
		return replicateShard(shardToAssign, srcNodeId, shardToReplicate);
	}
	deleteAssignmentCandidates(assignCandidates);
	return NULL;
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
void LoadBalancingStartOperation::prepareAssignmentCandidates(vector<AssignCandidatePartition *> & assignCandidates){
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

		bool canAcquireXLock = ShardManager::getShardManager()->getLockManager()->canAcquireLock(id, ResourceLockType_X);
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

void LoadBalancingStartOperation::deleteAssignmentCandidates(vector<AssignCandidatePartition *> & assignCandidates){
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
void LoadBalancingStartOperation::prepareMoveCandidates(vector<std::pair<NodeId, ClusterShardId> > & candidateSrcShards){
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

		bool canAcquireULock = ShardManager::getShardManager()->getLockManager()->canAcquireLock(id, ResourceLockType_U);
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

bool LoadBalancingStartOperation::isLightLoadedNode(NodeId nodeId){
	Cluster_Writeview * writeview = ShardManager::getWriteview();
	double loadAvg = 0;
	for(map<NodeId, double>::iterator nodeLoadItr = nodeLoads.begin(); nodeLoadItr != nodeLoads.end(); ++nodeLoadItr){
		loadAvg += nodeLoadItr->second;
	}
	loadAvg = loadAvg / nodeLoads.size();

	if(nodeLoads.find(nodeId) == nodeLoads.end()){
		return false;
	}
	return (nodeLoads[nodeId] < loadAvg) ;
}

// The first shard of a partition
OperationState * LoadBalancingStartOperation::assignShard(const ClusterShardId & unassignedShard){
	return new ShardAssignOperation(this->getOperationId(), unassignedShard);
}
OperationState * LoadBalancingStartOperation::replicateShard(const ClusterShardId & unassignedShard, NodeId srcNodeId, const ClusterShardId & shardToReplicate){
	return new ShardCopyOperation(this->getOperationId(), unassignedShard, srcNodeId, shardToReplicate);
}

OperationState * LoadBalancingStartOperation::moveShard(const NodeId & srcNodeId, const ClusterShardId & moveShardId){
	return new ShardMoveOperation(this->getOperationId(), srcNodeId, moveShardId);
}

}
}
