

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
	ASSERT(target.isClusterShardsMode());
	readListOfAttributes(target);
}

void AclAttributeReadCommand::readListOfAttributes(NodeTargetShardInfo & target){
	this->target = target;
	// send attribute list read notification to target node
	SP(AclAttributeReadNotification) notif = SP(AclAttributeReadNotification)(
			new AclAttributeReadNotification(roleId, target, clusterReadview));
	ConcurrentNotifOperation * listSender =
			new ConcurrentNotifOperation(notif, ShardingAclAttrReadACKMessageType, ShardManager::getCurrentNodeId(), this);
	ShardManager::getStateMachine()->registerOperation(listSender);
}


bool AclAttributeReadCommand::shouldAbort(const NodeId & failedNode){
	if(failedNode == this->target.getNodeId()){
		messageCodes.push_back(HTTP_Json_Failed_Due_To_Node_Failure);
		finalize();
		return true;
	}
	return false;
}

// response which contains the list of attributes comes to this function
void AclAttributeReadCommand::end(map<NodeId, SP(ShardingNotification) > & replies){
	if(replies.size() != 1){
		ASSERT(false);
		Logger::sharding(Logger::Error, "%d replies are received while waiting for a list of attributes." , replies.size());
		finalize();
		return;
	}


	AclAttributeReadNotification::ACK * listNotif = (AclAttributeReadNotification::ACK *)replies.begin()->second.get();
	vector<unsigned> listOfSearchableAttributes = listNotif->getListOfSearchableAttributes();
	vector<unsigned> listOfRefiningAttributes = listNotif->getListOfRefiningAttributes();
	finalize(true, listOfSearchableAttributes, listOfRefiningAttributes);
}

void AclAttributeReadCommand::finalize(bool status, vector<unsigned> listOfRefiningAttributes,
		vector<unsigned> listOfSearchableAttributes){
	if(this->getConsumer() != NULL){
		this->getConsumer()->consume(status, listOfSearchableAttributes, listOfRefiningAttributes, messageCodes);
	}
}

}
}
