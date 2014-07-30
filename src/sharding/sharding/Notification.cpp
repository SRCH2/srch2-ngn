#include "Notification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardingNotification::ShardingNotification(NodeId srcNodeId,
		unsigned srcOperationId,
		NodeId destNodeId,
		unsigned destOperationId){
	this->srcOperationId.nodeId = srcNodeId;
	this->destOperationId.nodeId = destNodeId;
	this->srcOperationId.operationId = srcOperationId;
	this->destOperationId.operationId = destOperationId;
}

NodeOperationId ShardingNotification::getSrcOperationId() const {
	return srcOperationId;
}
NodeOperationId ShardingNotification::getDestOperationId() const {
	return destOperationId;
}


void * ShardingNotification::serialize(void * buffer) const{
	buffer = srcOperationId.serialize(buffer);
	buffer = destOperationId.serialize(buffer);
	return buffer;
}
unsigned ShardingNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += srcOperationId.getNumberOfBytes();
	numberOfBytes += destOperationId.getNumberOfBytes();
	return numberOfBytes;
}
void * ShardingNotification::deserialize(void * buffer) const{
	buffer = srcOperationId.deserialize(buffer);
	buffer = destOperationId.deserialize(buffer);
	return buffer;
}

BroadcastNotification::BroadcastNotification(NodeId srcNodeId,
		unsigned srcOperationId,
		NodeId destNodeId,
		unsigned destOperationId): ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){
}


void * BroadcastNotification::serialize(void * buffer) const{
	buffer = ShardingNotification::serialize(buffer);
	return buffer;
}
unsigned BroadcastNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += ShardingNotification::getNumberOfBytes();
	return numberOfBytes;
}
void * BroadcastNotification::deserialize(void * buffer) const{
	buffer = ShardingNotification::deserialize(buffer);
	return buffer;
}

NotificationType ProposalNotification::OK::getType() const{
	return NotificationType_Sharding_Proposal_OK;
}
//Returns the type of message which uses this kind of object as transport
ShardingMessageType ProposalNotification::OK::messageType() const{
	return ShardingProposalOkMessageType;
}

NotificationType ProposalNotification::NO::getType() const{
	return NotificationType_Sharding_Proposal_NO;
}
//Returns the type of message which uses this kind of object as transport
ShardingMessageType ProposalNotification::NO::messageType() const{
	return ShardingProposalNoMessageType;
}



ProposalNotification::ProposalNotification(NodeId srcNodeId,
		unsigned srcOperationId,
		NodeId destNodeId,
		unsigned destOperationId,
		MetadataChange * metadataChange): ShardingNotification(srcNodeId,
				srcOperationId, destNodeId, destOperationId){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
}

NotificationType ProposalNotification::getType() const{
	ASSERT(metadataChange != NULL);
	switch (metadataChange->getType()) {
		case MetadataChangeType_ShardAssignChange:
			return NotificationType_Sharding_Commit_ShardAssignChange;
		case MetadataChangeType_ShardCopyChange:
			return NotificationType_Sharding_Commit_ShardCopyChange;
		case MetadataChangeType_ShardMoveChange:
			return NotificationType_Sharding_Commit_ShardMoveChange;
		case MetadataChangeType_ShardLoadChange:
			return NotificationType_Sharding_Commit_ShardLoadChange;
		default:
			ASSERT(false);
			break;
	}
	return NotificationType_Default;
}

MetadataChange * ProposalNotification::getMetadataChange() const{
	return this->metadataChange;
}

void * ProposalNotification::serialize(void * buffer) const{
	buffer = ShardingNotification::serialize(buffer);
	if(metadataChange == NULL){
		ASSERT(false);
		return buffer;
	}
	buffer = metadataChange->serialize(buffer);
	return buffer;
}
unsigned ProposalNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += ShardingNotification::getNumberOfBytes();
	if(metadataChange == NULL){
		ASSERT(false);
		return numberOfBytes;
	}
	numberOfBytes += metadataChange->getNumberOfBytes();
	return numberOfBytes;
}
void * ProposalNotification::deserialize(void * buffer) const{
	buffer = ShardingNotification::deserialize(buffer);
	if(metadataChange == NULL){
		ASSERT(false);
		return buffer;
	}
	buffer = metadataChange->deserialize(buffer);
	return buffer;
}
//Returns the type of message which uses this kind of object as transport
ShardingMessageType ProposalNotification::messageType() const{
	return ShardingProposalMessageType;
}

NotificationType LockingNotification::GRANTED::getType() const{
	return NotificationType_Sharding_Lock_GRANTED;
}
ShardingMessageType LockingNotification::GRANTED::messageType() const{
	return ShardingLockGrantedMessageType;
}

NotificationType LockingNotification::REJECTED::getType() const{
	return NotificationType_Sharding_Lock_REJECTED;
}
ShardingMessageType LockingNotification::REJECTED::messageType() const{
	return ShardingLockRejectedMessageType;
}


NotificationType LockingNotification::RELEASED::getType() const{
	return NotificationType_Sharding_Lock_RELEASED;
}

ShardingMessageType LockingNotification::RELEASED::messageType() const{
	return ShardingLockReleasedMessageType;
}


LockingNotification::RV_RELEASED::RV_RELEASED(unsigned metadataVersionId){
	this->metadataVersionId = metadataVersionId;
}
NotificationType LockingNotification::RV_RELEASED::getType() const{
	return NotificationType_Sharding_Lock_RV_RELEASED;
}
ShardingMessageType LockingNotification::RV_RELEASED::messageType() const{
	return ShardingLockRvReleasesMessageType;
}

unsigned LockingNotification::RV_RELEASED::getMetadataVersionId() const{
	return metadataVersionId;
}

LockingNotification::LockingNotification(NodeId srcNodeId,
		unsigned srcOperationId,
		NodeId destNodeId,
		unsigned destOperationId,
		LockChange * lockRequest):
		BroadcastNotification(srcNodeId,srcOperationId, destNodeId, destOperationId){
	ASSERT(lockRequest != NULL);
	this->lockRequest = lockRequest;
}

NotificationType LockingNotification::getType() const{
	ASSERT(this->lockRequest != NULL);
	if(this->lockRequest == NULL){
		return NotificationType_Default;
	}
	if(this->lockRequest->isAcquireOrRelease()){ // acquire
		if(this->lockRequest->isSharedOrExclusive()){ // S lock
			return NotificationType_Sharding_Lock_S_Lock;
		}else{
			return NotificationType_Sharding_Lock_X_Lock;
		}
	}
	// release
	if(this->lockRequest->isSharedOrExclusive()){ // S lock
		return NotificationType_Sharding_Lock_S_UnLock;
	}else{
		return NotificationType_Sharding_Lock_X_UnLock;
	}

}
LockChange * LockingNotification::getLockRequest() const{
	return this->lockRequest;
}


void * LockingNotification::serialize(void * buffer) const{
	buffer = BroadcastNotification::serialize(buffer);
	if(lockRequest == NULL){
		ASSERT(false);
		return buffer;
	}
	buffer = lockRequest->serialize(buffer);
	return buffer;
}
unsigned LockingNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += lockRequest->getNumberOfBytes();
	return numberOfBytes;
}
void * LockingNotification::deserialize(void * buffer) const{
	buffer = BroadcastNotification::deserialize(buffer);
	if(lockRequest == NULL){
		ASSERT(false);
		return buffer;
	}
	buffer = lockRequest->deserialize(buffer);
	return buffer;
}
ShardingMessageType LockingNotification::messageType() const{
	return ShardingLockMessageType;
}


NotificationType NodeInitNotification::WELCOME::getType() const{
	return NotificationType_Sharding_NewNode_Welcome;
}
ShardingMessageType NodeInitNotification::WELCOME::messageType() const{
	return ShardingNewNodeWelcomeMessageType;
}


NotificationType NodeInitNotification::BUSY::getType() const{
	return NotificationType_Sharding_NewNode_Busy;
}
ShardingMessageType NodeInitNotification::BUSY::messageType() const{
	return ShardingNewNodeBusyMessageType;
}

NotificationType  NodeInitNotification::NEW_HOST::getType() const{
	return NotificationType_Sharding_NewNode_NewHost;
}
ShardingMessageType NodeInitNotification::NEW_HOST::messageType() const{
	return ShardingNewNodeNewHostMessageType;
}
NotificationType NodeInitNotification::SHARD_REQUEST::getType() const{
	return NotificationType_Sharding_NewNode_ShardRequest;
}
ShardingMessageType NodeInitNotification::SHARD_REQUEST::messageType() const{
	return ShardingNewNodeShardRequestMessageType;
}

NotificationType NodeInitNotification::SHARD_OFFER::getType() const{
	return	NotificationType_Sharding_NewNode_ShardOffer;
}
ShardingMessageType NodeInitNotification::SHARD_OFFER::messageType() const{
	return ShardingNewNodeShardOfferMessageType;
}

NotificationType NodeInitNotification::SHARDS_READY::getType() const{
	return NotificationType_Sharding_NewNode_ShardsReady;
}
ShardingMessageType NodeInitNotification::SHARDS_READY::messageType() const{
	return ShardingNewNodeShardsReadyMessageType;
}

NotificationType NodeInitNotification::JOIN_PERMIT::getType() const{
	return 	NotificationType_Sharding_NewNode_JoinPermit;
}
ShardingMessageType NodeInitNotification::JOIN_PERMIT::messageType() const{
	return ShardingNewNodeJoinPermitMessageType;
}

}
}
