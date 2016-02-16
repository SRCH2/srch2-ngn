/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
