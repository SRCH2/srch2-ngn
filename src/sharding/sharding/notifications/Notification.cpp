#include "Notification.h"

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

}
}
