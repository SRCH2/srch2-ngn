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
#include "MetadataReport.h"
#include "../ShardManager.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../state_machine/StateMachine.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

bool MetadataReport::resolveNotification(SP(ShardingNotification) readAckNotif){
	ShardManager::getShardManager()->getStateMachine()->handle(readAckNotif);
	return true;
}
ShardingMessageType MetadataReport::messageType() const{
	return ShardingNewNodeReadMetadataReplyMessageType;
}
void * MetadataReport::serializeBody(void * buffer) const{
	buffer = writeview->serialize(buffer, false);
	return buffer;
}
unsigned MetadataReport::getNumberOfBytesBody() const{
	unsigned numberOfBytes= 0;
	numberOfBytes += writeview->getNumberOfBytes(false);
	return numberOfBytes;
}
void * MetadataReport::deserializeBody(void * buffer) {
	writeview = new Cluster_Writeview();
	buffer = writeview->deserialize(buffer, false);
	return buffer;
}
Cluster_Writeview * MetadataReport::getWriteview() const{
	return writeview;
}

bool MetadataReport::operator==(const MetadataReport & report){
	return *writeview == *(report.writeview);
}


ShardingMessageType MetadataReport::REQUEST::messageType() const{
	return ShardingNewNodeReadMetadataRequestMessageType;
}

bool MetadataReport::REQUEST::resolveNotification(SP(ShardingNotification) readNotif){
	ShardManager::getShardManager()->getMetadataManager()->resolve(boost::dynamic_pointer_cast<MetadataReport::REQUEST>(readNotif));
	return true;
}


}
}
