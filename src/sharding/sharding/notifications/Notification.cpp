#include "Notification.h"
#include "../ShardManager.h"
//#include "../metadata_manager/ResourceMetadataManager.h"
#include "../state_machine/StateMachine.h"
#include "../transactions/cluster_transactions/ShutdownCommand.h"
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

}
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

void ShardingNotification::swapSrcDest(){
	NodeOperationId temp = srcOperationId;
	srcOperationId = destOperationId;
	destOperationId = temp;
}

string ShardingNotification::getDescription(){
	stringstream ss;
	ss << getShardingMessageTypeStr(this->messageType()) << "(" << srcOperationId.toString() << " => " << destOperationId.toString();
	if(bounced){
		ss << ", bounced)";
	}else{
		ss << ")";
	}
	return ss.str();
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


Message * ShardingNotification::serialize(MessageAllocator * allocator) const{
	unsigned numberOfBytes = getNumberOfBytesAll();
	Message * msg = allocator->allocateMessage(numberOfBytes);
	char * buffer = Message::getBodyPointerFromMessagePointer(msg);
	serializeAll(buffer);
	return msg;
}

Message * ShardingNotification::createMessage(MessageAllocator * allocator) const{
	unsigned numberOfBytes = getNumberOfBytesHeader() + getNumberOfBytesBody();
	return allocator->allocateMessage(numberOfBytes);
}

void ShardingNotification::serializeHeaderInfo(Message * msg) const{
	char * buffer = Message::getBodyPointerFromMessagePointer(msg);
	serializeHeader(buffer);
}

void ShardingNotification::serializeContent(Message * msg) const{
	char * buffer = Message::getBodyPointerFromMessagePointer(msg);
	buffer = ((char *)buffer) + getNumberOfBytesHeader();
	serializeBody(buffer);
}

void ShardingNotification::deserializeHeader(Message * msg, NodeId srcNode,
		NodeOperationId & srcAddress, NodeOperationId & destAddress, bool & bounced){
	DummyShardingNotification notif;
	char * buffer = Message::getBodyPointerFromMessagePointer(msg);
	notif.deserializeHeader(buffer);
	srcAddress = notif.getSrc();
	destAddress = notif.getDest();
	bounced = notif.isBounced();
}

bool ShardingNotification::send(SP(ShardingNotification) notification){
	if(! notification){
		ASSERT(false);
		return false;
	}
	if(notification->messageType() == ShardingNewNodeReadMetadataReplyMessageType ||
	        notification->messageType() == ShardingNewNodeReadMetadataRequestMessageType){
	    if(notification->getSrc().nodeId == notification->getDest().nodeId){
	        ASSERT(false);
	        return false;
	    }
	}
	if(notification->getDest().nodeId == ShardManager::getCurrentNodeId()){
		return ShardManager::getShardManager()->resolveLocal(notification);
	}
	TransportManager * tm = ShardManager::getShardManager()->getTransportManager();
	Message * notificationMessage = notification->serialize(
			tm->getMessageAllocator());
	notificationMessage->setShardingMask();
	notificationMessage->setMessageId(tm->getUniqueMessageIdValue());
	notificationMessage->setType(notification->messageType());

//	Logger::sharding(Logger::Detail, "%s | Sending [Type: %s, ID: %d]. ", notification->getDescription().c_str(),
//			notificationMessage->getDescription().c_str(), notificationMessage->getMessageId());

	if(tm->sendMessage(notification->getDest().nodeId , notificationMessage, 0) == 0){
		tm->getMessageAllocator()->deallocateByMessagePointer(notificationMessage);
		return false;
	}
	tm->getMessageAllocator()->deallocateByMessagePointer(notificationMessage);
	// currently TM always send the message
	return true;
}

bool ShardingNotification::send(SP(ShardingNotification) notification, const vector<NodeOperationId> & destinations ){
	TransportManager * tm = ShardManager::getShardManager()->getTransportManager();
	// first serialize the body
	Message * notificationMessage = notification->serialize(tm->getMessageAllocator());
	for(unsigned i = 0 ; i < destinations.size(); ++i){
		// 	now serialize header for each destination
		notification->serializeHeaderInfo(notificationMessage);
		notificationMessage->setShardingMask();
		notificationMessage->setMessageId(tm->getUniqueMessageIdValue());
		notificationMessage->setType(notification->messageType());

//		Logger::sharding(Logger::Detail, "%s | Sending [Type: %s, ID: %d]. ", notification->getDescription().c_str(),
//				notificationMessage->getDescription().c_str(), notificationMessage->getMessageId());
		tm->sendMessage(notification->getDest().nodeId , notificationMessage, 0);
	}
	tm->getMessageAllocator()->deallocateByMessagePointer(notificationMessage);
	return true;
}


void * ShardingNotification::serializeHeader(void * buffer) const{
	buffer = srcOperationId.serialize(buffer);
	buffer = destOperationId.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(bounced, buffer);
	return buffer;
}
unsigned ShardingNotification::getNumberOfBytesHeader() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += srcOperationId.getNumberOfBytes();
	numberOfBytes += destOperationId.getNumberOfBytes();
	numberOfBytes += sizeof(bounced);
	return numberOfBytes;
}
void * ShardingNotification::deserializeHeader(void * buffer) {
	buffer = srcOperationId.deserialize(buffer);
	buffer = destOperationId.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, bounced);
	return buffer;
}

bool ShutdownNotification::resolveNotification(SP(ShardingNotification) _notif){
	ShutdownCommand::_shutdown();
	return true;
}

}
}
