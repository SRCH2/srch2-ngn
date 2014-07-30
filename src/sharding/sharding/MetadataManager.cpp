#include "MetadataManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


NotificationType CommitNotification::ACK::getType() const{
	return NotificationType_Sharding_Commit_ACK;
}

//Returns the type of message which uses this kind of object as transport
ShardingMessageType CommitNotification::ACK::messageType() const{
	return ShardingCommitACKMessageType;
}


CommitNotification::CommitNotification(NodeId srcNodeId,
		unsigned srcOperationId,
		NodeId destNodeId,
		unsigned destOperationId,
		MetadataChange * metadataChange): BroadcastNotification(srcNodeId,
				srcOperationId, destNodeId, destOperationId){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
}

NotificationType CommitNotification::getType() const{
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

MetadataChange * CommitNotification::getMetadataChange() const{
	return this->metadataChange;
}


void * CommitNotification::serialize(void * buffer) const{
	buffer = BroadcastNotification::serialize(buffer);
	if(metadataChange == NULL){
		ASSERT(false);
		return buffer;
	}
	buffer = metadataChange->serialize(buffer);
	return buffer;
}
unsigned CommitNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += BroadcastNotification::getNumberOfBytes();
	if(metadataChange == NULL){
		ASSERT(false);
		return numberOfBytes;
	}
	numberOfBytes += metadataChange->getNumberOfBytes();
	return numberOfBytes;
}
void * CommitNotification::deserialize(void * buffer) const{
	buffer = BroadcastNotification::deserialize(buffer);
	if(metadataChange == NULL){
		ASSERT(false);
		return buffer;
	}
	buffer = metadataChange->deserialize(buffer);
	return buffer;
}
//Returns the type of message which uses this kind of object as transport
ShardingMessageType CommitNotification::messageType() const{
	return ShardingCommitMessageType;
}


CommitOperation::CommitOperation(unsigned operationId, MetadataChange * metadataChange):OperationState(operationId){
	this->metadataChange = metadataChange;
}
OperationStateType CommitOperation::getType(){
	return OperationStateType_Commit;
}
void CommitOperation::entry(map<NodeId, unsigned> specialTargets){

	// 1. fist commit the change on local metadata
	MetadataManager::getMetadataManager()->applyAndCommit(metadataChange);
	// 2. send commit notification to all other nodes
	// 2.a) first register this change broadcast in ShardManager buffer
	ShardManager::getShardManager()->registerChangeBroadcast(metadataChange);
	ClusterResourceMetadata_Writeview & writeview = MetadataManager::getMetadataManager()->getWriteview();
	for(map<NodeId, Node>::iterator nodeItr = writeview.nodes.begin(); nodeItr != writeview.nodes.end(); ++nodeItr){
		if(nodeItr->first == writeview.currentNodeId){
			continue;
		}
		haveReplied[nodeItr->first] = false;
		unsigned destOperationId = 0;
		if(specialTargets.find(nodeItr->first) != specialTargets.end()){
			destOperationId = specialTargets.find(nodeItr->first)->second;
		}
		CommitNotification * commitNotification = new CommitNotification(writeview.currentNodeId,
				this->getOperationId(), nodeItr->first, destOperationId, metadataChange );
		ShardManager::getShardManager()->send(commitNotification);
		delete commitNotification;
	}
}
// returns false when it's done.
bool CommitOperation::handle(CommitNotification::ACK * inputNotification){
	ASSERT(doesExpect(inputNotification));
	NodeOperationId srcOpId = inputNotification->getSrcOperationId();
	if(haveReplied.find(srcOpId.nodeId) == haveReplied.end()){
		return true;
	}
	if(haveReplied[srcOpId.nodeId] == true){
		// why did we receive two acks from this node ?
		ASSERT(false);
		return true;
	}
	haveReplied[srcOpId.nodeId] = true;

	if(hasAllNodesReplied()){
		// flush the change buffer
		ShardManager::getShardManager()->flushChangeHistory(metadataChange->getChangeId());
		return false;
	}
	return true;
}
bool CommitOperation::doesExpect(CommitNotification::ACK * inputNotification) const{
	if(inputNotification == NULL){
		ASSERT(false);
		return false;
	}
	return (this->getOperationId() == inputNotification->getDestOperationId());
}

bool CommitOperation::hasAllNodesReplied() const{
	for(map<NodeId, bool>::iterator nodeItr = haveReplied.begin(); nodeItr != haveReplied.end(); ++nodeItr){
		if(! nodeItr->second){
			return false;
		}
	}
	return true;
}

MetadataManager * MetadataManager::singleInstance = 0x0;
MetadataManager * MetadataManager::createMetadataManager(const ClusterResourceMetadata_Readview & readview){
	if(singleInstance != NULL){
		ASSERT(false);
		return singleInstance;
	}
	singleInstance = new MetadataManager(readview);
	return singleInstance;
}
MetadataManager * MetadataManager::getMetadataManager(){
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	return singleInstance;
}

MetadataManager::MetadataManager(const ClusterResourceMetadata_Readview & readview):
	writeview(readview){
}

ClusterResourceMetadata_Writeview & MetadataManager::getWriteview() const{
	return writeview;
}

void MetadataManager::resolve(CommitNotification * notification){
	if(notification == NULL){
		ASSERT(false);
		return;
	}
	applyAndCommit(notification->getMetadataChange());
	// send ACK back to the srcNode
	CommitNotification::ACK * ack = new CommitNotification::ACK(writeview.currentNodeId, 0, notification->getSrcOperationId().nodeId,
			notification->getSrcOperationId().operationId);
	this->getShardManager()->send(ack);
	delete ack;
}

unsigned MetadataManager::applyAndCommit(MetadataChange * metadataChange){
	// apply change on writeview and commit
	if(metadataChange == NULL){
		ASSERT(false);
		return;
	}
	metadataChange->apply(true, &writeview); // always do ?
	ClusterResourceMetadata_Readview * newReadview = writeview.getNewReadview();
	this->getShardManager()->getConfigManager()->commitClusterMetadata(newReadview);
	return writeview.versionId - 1;
}

ShardManager * MetadataManager::getShardManager(){
	return ShardManager::getShardManager();
}



}
}
