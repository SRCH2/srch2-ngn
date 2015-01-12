#include "AclAttributeReplaceNotification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "../../sharding/ShardManager.h"
#include "sharding/processor/DistributedProcessorInternal.h"
#include "../state_machine/StateMachine.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {


AclAttributeReplaceNotification::AclAttributeReplaceNotification(const vector<string> & attributes, unsigned coreId){
	ShardManager::getReadview(clusterReadview);
	this->attributes = attributes;
	this->coreId = coreId;
}
AclAttributeReplaceNotification::AclAttributeReplaceNotification(){
	ShardManager::getReadview(clusterReadview);
	this->coreId = 0;
}

bool AclAttributeReplaceNotification::resolveNotification(SP(ShardingNotification) _notif){
	SP(AclAttributeReplaceNotification::ACK) response =
			ShardManager::getShardManager()->getDPInternal()->
			resolveAclAttributeReplaceDeletePhase(boost::dynamic_pointer_cast<AclAttributeReplaceNotification>(_notif));
	if(! response){
		response = create<AclAttributeReplaceNotification::ACK>();
	}
    response->setSrc(_notif->getDest());
    response->setDest(_notif->getSrc());
	send(response);
	return true;
}
bool AclAttributeReplaceNotification::hasResponse() const {
		return true;
}
ShardingMessageType AclAttributeReplaceNotification::messageType() const{
	return ShardingAclAttrReplaceMessageType;
}
void * AclAttributeReplaceNotification::serializeBody(void * buffer) const{
	buffer = srch2::util::serializeVectorOfString(attributes, buffer);
	buffer = srch2::util::serializeFixedTypes(coreId, buffer);
	return buffer;
}
unsigned AclAttributeReplaceNotification::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(attributes);
	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(coreId);
	return numberOfBytes;
}
void * AclAttributeReplaceNotification::deserializeBody(void * buffer) {
	buffer = srch2::util::deserializeVectorOfString(buffer, attributes);
	buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
	return buffer;
}


const vector<string> & AclAttributeReplaceNotification::getAttributes() const {
	return attributes;
}

const CoreInfo_t * AclAttributeReplaceNotification::getCoreInfo() const{
	return this->clusterReadview->getCore(this->coreId);
}
boost::shared_ptr<const ClusterResourceMetadata_Readview> AclAttributeReplaceNotification::getReadview() const{
	return clusterReadview;
}

bool AclAttributeReplaceNotification::ACK::resolveNotification(SP(ShardingNotification) _notif){
	ShardManager::getStateMachine()->handle(_notif);
	return true;
}
ShardingMessageType AclAttributeReplaceNotification::ACK::messageType() const{
	return ShardingAclAttrReadACKMessageType;
}

}
}
