#include "LockBatch.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

LockBatch::LockBatch(){
	this->blocking = false;
	this->release = false;
	this->incremental = false;
	this->granted = false;
	this->lastGrantedItemIndex = -1;
	this->batchType = LockRequestType_ShardIdList;
	this->versionId = 0;
}
LockBatch::~LockBatch(){
}

bool LockBatch::isReadviewPending() const{
	return (versionId > 0);
}

bool LockBatch::update(const NodeId & failedNode){
	if(! ack){
		if(ack->getDest().nodeId == failedNode){
			return false;
		}
	}
	for(vector<NodeOperationId>::iterator nItr = opIds.begin(); nItr != opIds.end(); ){
		if(nItr->nodeId == failedNode){
			nItr = opIds.erase(nItr);
		}else{
			++nItr;
		}
	}
	if(opIds.empty()){
		return false;
	}
	for(vector<NodeId>::iterator nItr = olderNodes.begin(); nItr != olderNodes.end();){
		if(failedNode == *nItr){
			nItr = olderNodes.erase(nItr);
		}else{
			++nItr;
		}
	}
	return true;
}

LockBatch * LockBatch::generateLockBatch(SP(LockingNotification) notif){
	LockBatch * lockBatch = new LockBatch();
	lockBatch->blocking = notif->isBlocking();
	lockBatch->release = notif->isReleaseRequest();
	lockBatch->batchType = notif->getType();
	lockBatch->ack = ShardingNotification::create<LockingNotification::ACK>();
	lockBatch->ack->setDest(notif->getSrc());
	lockBatch->ack->setSrc(notif->getDest());
	switch (notif->getType()) {
		case LockRequestType_Copy:
		{
			ClusterShardId srcShardId, destShardId;
			NodeOperationId copyAgent;
			notif->getLockRequestInfo(srcShardId, destShardId, copyAgent);
			lockBatch->opIds.push_back(copyAgent);
			if(srcShardId <= destShardId){
				lockBatch->tokens.push_back(std::make_pair(srcShardId, LockLevel_S));
				lockBatch->tokens.push_back(std::make_pair(destShardId, LockLevel_X));
			}else{
				lockBatch->tokens.push_back(std::make_pair(destShardId, LockLevel_X));
				lockBatch->tokens.push_back(std::make_pair(srcShardId, LockLevel_S));
			}
			return lockBatch;
		}
		case LockRequestType_Move:
		{
			ClusterShardId shardId;
			NodeOperationId srcMoveAgent, destMoveAgent;
			notif->getLockRequestInfo(shardId, srcMoveAgent, destMoveAgent);
			lockBatch->opIds.push_back(srcMoveAgent);
			lockBatch->opIds.push_back(destMoveAgent);
			lockBatch->tokens.push_back(std::make_pair(shardId, LockLevel_X));
			return lockBatch;
		}
		case LockRequestType_Metadata:
		{
			NodeOperationId newNodeOpId;
			vector<NodeId> listOfOlderNodes;
			LockLevel lockLevel;
			notif->getLockRequestInfo(newNodeOpId, listOfOlderNodes, lockLevel);
			lockBatch->opIds.push_back(newNodeOpId);
			lockBatch->metadataLockLevel = lockLevel;
			lockBatch->olderNodes = listOfOlderNodes;
			return lockBatch;
		}
		case LockRequestType_PrimaryKey:
		{
			vector<string> primaryKeys;
			NodeOperationId writerAgent;
			ClusterPID pid;
			notif->getLockRequestInfo(primaryKeys, writerAgent, pid);
			lockBatch->pid = pid;
			lockBatch->opIds.push_back(writerAgent);
			for(unsigned pkIdx = 0; pkIdx < primaryKeys.size(); ++pkIdx){
				lockBatch->pkTokens.push_back(std::make_pair(primaryKeys.at(pkIdx), LockLevel_X));
			}
			return lockBatch;
		}
		case LockRequestType_GeneralPurpose:
		{
			ClusterShardId shardId;
			NodeOperationId agent;
			LockLevel lockLevel;
			notif->getLockRequestInfo(shardId, agent, lockLevel);
			lockBatch->opIds.push_back(agent);
			lockBatch->tokens.push_back(std::make_pair(shardId, lockLevel));
			return lockBatch;
		}
		case LockRequestType_ShardIdList:
		{
			vector<ClusterShardId> shardIdList;
			NodeOperationId shardIdListLockHolder;
			LockLevel shardIdListLockLevel;
			notif->getLockRequestInfo(shardIdList, shardIdListLockHolder, shardIdListLockLevel);
			lockBatch->opIds.push_back(shardIdListLockHolder);
			for(unsigned i = 0 ; i < shardIdList.size() ; ++i){
				lockBatch->tokens.push_back(std::make_pair(shardIdList.at(i), shardIdListLockLevel));
			}
			return lockBatch;
		}
	}
}

LockBatch * LockBatch::generateLockBatch(const ClusterShardId & shardId, const LockLevel & lockLevel){
	LockBatch * lockBatch = new LockBatch();
	lockBatch->blocking = false;
	lockBatch->release = false;
	lockBatch->batchType = LockRequestType_GeneralPurpose;
	lockBatch->tokens.push_back(std::make_pair(shardId, lockLevel));
	return lockBatch;
}

}
}
