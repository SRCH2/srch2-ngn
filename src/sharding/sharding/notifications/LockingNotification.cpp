#include "LockingNotification.h"

#include "../metadata_manager/ResourceLocks.h"
#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

LockingNotification::LockingNotification(ResourceLockRequest * lq){
	this->lockRequest = lq;
}

LockingNotification::LockingNotification(){
	// for deserialization
}


void * LockingNotification::serialize(void * buffer) const{
	buffer = ShardingNotification::serialize(buffer);
	buffer = lockRequest->serialize(buffer);
	return buffer;
}
unsigned LockingNotification::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += ShardingNotification::getNumberOfBytes();
	numberOfBytes += lockRequest->getNumberOfBytes();
	return numberOfBytes;
}
void * LockingNotification::deserialize(void * buffer){
	buffer = ShardingNotification::deserialize(buffer);
	lockRequest = new ResourceLockRequest();
	buffer = lockRequest->deserialize(buffer);
	return buffer;
}

ShardingMessageType LockingNotification::messageType() const{
	return ShardingLockMessageType;
}

ResourceLockRequest * LockingNotification::getLockRequest() const {
	return this->lockRequest;
}

bool LockingNotification::operator==(const LockingNotification & lockingNotification){
	return *lockRequest == *(lockingNotification.lockRequest);
}

LockingNotification::ACK::ACK(bool grantedFlag){
	this->grantedFlag = grantedFlag;
};
ShardingMessageType LockingNotification::ACK::messageType() const{
	return ShardingLockACKMessageType;
}

void * LockingNotification::ACK::serialize(void * buffer) const{
	buffer = ShardingNotification::serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(grantedFlag, buffer);
	return buffer;
}
unsigned LockingNotification::ACK::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += ShardingNotification::getNumberOfBytes();
	numberOfBytes += sizeof(bool);
	return numberOfBytes;
}
void * LockingNotification::ACK::deserialize(void * buffer){
	buffer = ShardingNotification::deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, grantedFlag);
	return buffer;
}

bool LockingNotification::ACK::isGranted() const{
	return grantedFlag;
}

bool LockingNotification::ACK::operator==(const LockingNotification::ACK & right){
	return grantedFlag == right.grantedFlag;
}

LockingNotification::RV_RELEASED::RV_RELEASED(unsigned metadataVersionId){
	this->metadataVersionId = metadataVersionId;
}
ShardingMessageType LockingNotification::RV_RELEASED::messageType() const{
	return ShardingLockRVReleasedMessageType;
}
unsigned LockingNotification::RV_RELEASED::getMetadataVersionId() const{
	return metadataVersionId;
}


}
}
