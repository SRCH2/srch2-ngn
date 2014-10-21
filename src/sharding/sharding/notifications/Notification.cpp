#include "Notification.h"
#include "../ShardManager.h"
//#include "../metadata_manager/ResourceMetadataManager.h"
#include "../ClusterOperationContainer.h"

#include "core/util/SerializationHelper.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


NodeOperationId::NodeOperationId(){
	// temp init used for deserialization
}
NodeOperationId::NodeOperationId(const NodeOperationId & id){
	this->nodeId = id.nodeId;
	this->operationId = id.operationId;
}

NodeOperationId & NodeOperationId::operator=(const NodeOperationId & rhs){
	if(this == &rhs){
		return *this;
	}
	this->nodeId = rhs.nodeId;
	this->operationId = rhs.operationId;
	return *this;
}

NodeOperationId::NodeOperationId(NodeId nodeId, unsigned operationId){
	this->nodeId = nodeId;
	this->operationId = operationId;
}
void * NodeOperationId::serialize(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
	buffer = srch2::util::serializeFixedTypes(operationId, buffer);
	return buffer;
}
unsigned NodeOperationId::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += sizeof(nodeId);
	numberOfBytes += sizeof(operationId);
	return numberOfBytes;
}
void * NodeOperationId::deserialize(void * buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, nodeId);
	buffer = srch2::util::deserializeFixedTypes(buffer, operationId);
	return buffer;
}

bool NodeOperationId::operator==(const NodeOperationId & right) const{
	return (this->nodeId == right.nodeId) && (this->operationId == right.operationId);
}
bool NodeOperationId::operator>(const NodeOperationId & right) const{
	return (this->nodeId > right.nodeId) ||
			((this->nodeId == right.nodeId) && (this->operationId > right.operationId));
}
bool NodeOperationId::operator<(const NodeOperationId & right) const{
	return !((*this == right) || (*this > right));
}

string NodeOperationId::toString() const{
	stringstream ss;
	ss << "(" << nodeId << "," << operationId << ")";
	return ss.str();
}


ShardMigrationStatus::ShardMigrationStatus(const ShardMigrationStatus & status){
    this->srcOperationId = status.srcOperationId;
    this->dstOperationId = status.dstOperationId;
    this->sourceNodeId = status.sourceNodeId;
    this->destinationNodeId = status.destinationNodeId;
    this->shard = status.shard;
    this->status = status.status;

};
ShardMigrationStatus & ShardMigrationStatus::operator=(const ShardMigrationStatus & status){
    if(this == &status){
        return *this;
    }
    srcOperationId = status.srcOperationId;
    dstOperationId = status.dstOperationId;
    sourceNodeId = status.sourceNodeId;
    destinationNodeId = status.destinationNodeId;
    shard = status.shard;
    this->status = status.status;
    return *this;
}

void * ShardMigrationStatus::serialize(void * buffer) const{
    buffer = srch2::util::serializeFixedTypes(srcOperationId,buffer);
    buffer = srch2::util::serializeFixedTypes(dstOperationId,buffer);
    buffer = srch2::util::serializeFixedTypes(sourceNodeId,buffer);
    buffer = srch2::util::serializeFixedTypes(destinationNodeId,buffer);
    buffer = srch2::util::serializeFixedTypes(status,buffer);
    return buffer;
}
unsigned ShardMigrationStatus::getNumberOfBytes() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += sizeof(unsigned);
    numberOfBytes += sizeof(unsigned);
    numberOfBytes += sizeof(NodeId);
    numberOfBytes += sizeof(NodeId);
    numberOfBytes += sizeof(MIGRATION_STATUS);
    return numberOfBytes;
}
void * ShardMigrationStatus::deserialize(void * buffer) {
    buffer = srch2::util::deserializeFixedTypes(buffer, srcOperationId);
    buffer = srch2::util::deserializeFixedTypes(buffer, dstOperationId);
    buffer = srch2::util::deserializeFixedTypes(buffer, sourceNodeId);
    buffer = srch2::util::deserializeFixedTypes(buffer, destinationNodeId);
    buffer = srch2::util::deserializeFixedTypes(buffer, status);
    return buffer;
}

ShardingNotification::ShardingNotification(){
	setSrc(NodeOperationId(0,0));
	setDest(NodeOperationId(0,0));
	this->bounced = false;

}

NodeOperationId ShardingNotification::getSrc() const {
	return srcOperationId;
}
NodeOperationId ShardingNotification::getDest() const {
	return destOperationId;
}
void ShardingNotification::setSrc(const NodeOperationId & src) {
	this->srcOperationId = NodeOperationId(src);
}
void ShardingNotification::setDest(const NodeOperationId & dest) {
	this->destOperationId = NodeOperationId(dest);
}
void ShardingNotification::setBounced(){
	this->bounced = true;
}
void ShardingNotification::resetBounced(){
	this->bounced = false;
}
bool ShardingNotification::isBounced() const{
	return this->bounced;
}

void * ShardingNotification::serialize(void * buffer) const{
	buffer = srcOperationId.serialize(buffer);
	buffer = destOperationId.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(bounced, buffer);
	return buffer;
}
unsigned ShardingNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += srcOperationId.getNumberOfBytes();
	numberOfBytes += destOperationId.getNumberOfBytes();
	numberOfBytes += sizeof(bool);
	return numberOfBytes;
}
void * ShardingNotification::deserialize(void * buffer) {
	buffer = srcOperationId.deserialize(buffer);
	buffer = destOperationId.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, bounced);
	return buffer;
}

MMNotification::MMNotification(const ShardMigrationStatus & status):status(status){
    this->setSrc(NodeOperationId(this->status.sourceNodeId, this->status.srcOperationId));
    this->setDest(NodeOperationId(this->status.destinationNodeId, this->status.dstOperationId));
}
MMNotification::MMNotification(){};
ShardMigrationStatus MMNotification::getStatus() const{
    return this->status;
}
void MMNotification::setStatus(const ShardMigrationStatus & status){
    this->status = status;
}
ShardingMessageType MMNotification::messageType() const {
    return ShardingMMNotificationMessageType;
}

void * MMNotification::serialize(void * buffer) const{
    buffer = ShardingNotification::serialize(buffer);
    buffer = status.serialize(buffer);
    return buffer;
}
unsigned MMNotification::getNumberOfBytes() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += ShardingNotification::getNumberOfBytes();
    numberOfBytes += status.getNumberOfBytes();
    return numberOfBytes;
}
void * MMNotification::deserialize(void * buffer) {
    buffer = ShardingNotification::deserialize(buffer);
    buffer = status.deserialize(buffer);
    return buffer;
}


bool SaveDataNotification::resolveMessage(Message * msg, NodeId sendeNode){
	SaveDataNotification * saveDataNotif =
			ShardingNotification::deserializeAndConstruct<SaveDataNotification>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", saveDataNotif->getDescription().c_str());
	if(ShardManager::getShardManager()->handleBouncing(saveDataNotif)){
		return true;
	}

	ShardManager::getShardManager()->resolve(saveDataNotif);
	delete saveDataNotif;
	return false;
}

void * SaveDataNotification::serialize(void * buffer) const{
    buffer = ShardingNotification::serialize(buffer);
    buffer = targets.serialize(buffer);
    return buffer;

}
unsigned SaveDataNotification::getNumberOfBytes() const{
    unsigned numberOfBytes = 0;
    numberOfBytes += ShardingNotification::getNumberOfBytes();
    numberOfBytes += targets.getNumberOfBytes();
    return numberOfBytes;
}
void * SaveDataNotification::deserialize(void * buffer) {
    buffer = ShardingNotification::deserialize(buffer);
    buffer = targets.deserialize(buffer);
    return buffer;
}

bool SaveDataNotification::ACK::resolveMessage(Message * msg, NodeId sendeNode){
	SaveDataNotification::ACK * saveDataNotif =
			ShardingNotification::deserializeAndConstruct<SaveDataNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", saveDataNotif->getDescription().c_str());
	if(saveDataNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete saveDataNotif;
		return true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(saveDataNotif);
	delete saveDataNotif;
	return false;
}


bool SaveMetadataNotification::resolveMessage(Message * msg, NodeId sendeNode){
	SaveMetadataNotification * saveMetadataNotif =
			ShardingNotification::deserializeAndConstruct<SaveMetadataNotification>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", saveMetadataNotif->getDescription().c_str());
	if(ShardManager::getShardManager()->handleBouncing(saveMetadataNotif)){
		return true;
	}

	ShardManager::getShardManager()->resolve(saveMetadataNotif);
	delete saveMetadataNotif;
	return false;
}


bool SaveMetadataNotification::ACK::resolveMessage(Message * msg, NodeId sendeNode){
	SaveMetadataNotification::ACK * saveMetadataNotif =
			ShardingNotification::deserializeAndConstruct<SaveMetadataNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", saveMetadataNotif->getDescription().c_str());
	if(saveMetadataNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete saveMetadataNotif;
		return true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(saveMetadataNotif);
	delete saveMetadataNotif;
	return false;
}

bool MergeNotification::resolveMessage(Message * msg, NodeId sendeNode){
	MergeNotification * mergeNotification =
			ShardingNotification::deserializeAndConstruct<MergeNotification>(Message::getBodyPointerFromMessagePointer(msg));
	if(ShardManager::getShardManager()->handleBouncing(mergeNotification)){
		return true;
	}
	ShardManager::getShardManager()->resolve(mergeNotification);
	delete mergeNotification;
	return false;
}

MergeOperationType MergeNotification::getMergeOperationType() const{
	return this->operationType;
}
void * MergeNotification::serialize(void * buffer) const{
    buffer = ShardingNotification::serialize(buffer);
    buffer = srch2::util::serializeFixedTypes(operationType, buffer);
    switch (operationType) {
		case MergeOperationType_Merge:
		{
			break;
		}
		case MergeOperationType_SetON:
		case MergeOperationType_SetOFF:
		{
			buffer = srch2::util::serializeVectorOfDynamicTypes(clusterShardIds, buffer);
			buffer = srch2::util::serializeVectorOfDynamicTypes(nodeShardIds, buffer);
			break;
		}
	}
    return buffer;
}
unsigned MergeNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += ShardingNotification::getNumberOfBytes();
	numberOfBytes += sizeof(operationType);
    switch (operationType) {
		case MergeOperationType_Merge:
		{
			break;
		}
		case MergeOperationType_SetON:
		case MergeOperationType_SetOFF:
		{
			numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypes(clusterShardIds);
			numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypes(nodeShardIds);
			break;
		}
	}
	return numberOfBytes;
}
void * MergeNotification::deserialize(void * buffer) {
    buffer = ShardingNotification::deserialize(buffer);
    buffer = srch2::util::deserializeFixedTypes(buffer, operationType);
    switch (operationType) {
		case MergeOperationType_Merge:
		{
			break;
		}
		case MergeOperationType_SetON:
		case MergeOperationType_SetOFF:
		{
			buffer = srch2::util::deserializeVectorOfDynamicTypes(buffer, clusterShardIds);
			buffer = srch2::util::deserializeVectorOfDynamicTypes(buffer, nodeShardIds);
			break;
		}
	}
    return buffer;
}


bool MergeNotification::ACK::resolveMessage(Message * msg, NodeId sendeNode){
	MergeNotification::ACK * mergeNotification =
			ShardingNotification::deserializeAndConstruct<MergeNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
	if(mergeNotification->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete mergeNotification;
		true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(mergeNotification);
	delete mergeNotification;
	return false;
}

bool ShutdownNotification::resolveMessage(Message * msg, NodeId sendeNode){
	ShutdownNotification * shutdownNotification =
			ShardingNotification::deserializeAndConstruct<ShutdownNotification>(Message::getBodyPointerFromMessagePointer(msg));
	if(ShardManager::getShardManager()->handleBouncing(shutdownNotification)){
		return true;
	}
	ShardManager::getShardManager()->_shutdown();
	return false;
}

}
}
