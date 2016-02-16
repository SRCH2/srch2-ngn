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


#include "AclAttributeReadCommand.h"
#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommandStatusNotification.h"
#include "../../notifications/AclAttributeReadNotification.h"
#include "../../metadata_manager/Shard.h"
#include "../Transaction.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"
#include "../../state_machine/StateMachine.h"
#include "../../metadata_manager/Partition.h"
#include "processor/Partitioner.h"

#include <iostream>
#include <ctime>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

AclAttributeReadCommand::AclAttributeReadCommand(ConsumerInterface * consumer,
		const string & roleId,
		const CoreInfo_t * coreInfo):ProducerInterface(consumer),
		coreInfo(coreInfo),
		roleId(roleId){
	ASSERT(coreInfo != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction());
	clusterReadview = ((ReadviewTransaction *)(this->getTransaction().get()))->getReadview();
}

SP(Transaction) AclAttributeReadCommand::getTransaction(){
	if(this->getConsumer() == NULL){
		ASSERT(false);
		return SP(Transaction)();
	}
	return this->getConsumer()->getTransaction();
}

void AclAttributeReadCommand::produce(){

	/*
	 * 1. find one target for this roleId (which is our primary key)
	 */
	const CorePartitionContianer * corePartContainer = clusterReadview->getCorePartitionContianer(coreInfo->getCoreId());
	CorePartitioner * partitioner = new CorePartitioner(corePartContainer);
	vector<NodeTargetShardInfo> allWriteTargets;
	partitioner->getAllWriteTargets(partitioner->getRecordValueToHash(roleId), ShardManager::getCurrentNodeId(), allWriteTargets);
	if(allWriteTargets.empty()){
		messageCodes.push_back(HTTP_Json_Role_Id_Does_Not_Exist);
		finalize();
		return;
	}
	// if current node is among targets choose it otherwise just pick one
	unsigned chosenTarget = 0;
	for(chosenTarget = 0; chosenTarget < allWriteTargets.size(); ++chosenTarget){
		if(allWriteTargets.at(chosenTarget).getNodeId() == ShardManager::getCurrentNodeId()){
			break;
		}
	}
	if(chosenTarget >= allWriteTargets.size()){
		srand(time(NULL));
		chosenTarget = rand() % allWriteTargets.size();
	}
	NodeTargetShardInfo & target = allWriteTargets.at(chosenTarget);
	// ASSERT(target.isClusterShardsMode());   // commenting out ASSERT ..ACL should be available for both node/cluster shards.
	readListOfAttributes(target);
}

void AclAttributeReadCommand::readListOfAttributes(NodeTargetShardInfo & target){
	this->target = target;
	// send attribute list read notification to target node
	SP(AclAttributeReadNotification) notif = SP(AclAttributeReadNotification)(
			new AclAttributeReadNotification(roleId, target, clusterReadview));
	ConcurrentNotifOperation * listSender =
			new ConcurrentNotifOperation(notif, ShardingAclAttrReadACKMessageType,this->target.getNodeId(), this);
	ShardManager::getStateMachine()->registerOperation(listSender);
}

// response which contains the list of attributes comes to this function
void AclAttributeReadCommand::end(map<NodeId, SP(ShardingNotification) > & replies){
	if(replies.size() < 1){
		Logger::sharding(Logger::Error, "%d replies are received while waiting for a list of attributes." , replies.size());
		finalize();
		return;
	}


	AclAttributeReadNotification::ACK * listNotif = (AclAttributeReadNotification::ACK *)replies.begin()->second.get();
	const vector<unsigned>& listOfSearchableAttributes = listNotif->getListOfSearchableAttributes();
	const vector<unsigned>& listOfRefiningAttributes = listNotif->getListOfRefiningAttributes();
	finalize(true, listOfSearchableAttributes, listOfRefiningAttributes);
}

void AclAttributeReadCommand::finalize(bool status, const vector<unsigned>& listOfSearchableAttributes,
	const vector<unsigned>& listOfRefiningAttributes){
	if(this->getConsumer() != NULL){
		this->getConsumer()->consume(status, listOfSearchableAttributes, listOfRefiningAttributes, messageCodes);
	}
}

void AclAttributeReadCommand::finalize(){
	if(this->getConsumer() != NULL){
		this->getConsumer()->consume(false, vector<unsigned>(), vector<unsigned>(), messageCodes);
	}
}

}
}
