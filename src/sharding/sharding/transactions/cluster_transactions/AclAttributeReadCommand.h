#ifndef __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__
#define __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__

#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommandStatusNotification.h"
#include "../../metadata_manager/Shard.h"
#include "../Transaction.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

#include <iostream>
#include <ctime>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class AclAttributeReadCommand: public ProducerInterface, public NodeIteratorListenerInterface {
public:

	// calls consume(const vector<string> & attributeIds, const vector<JsonMessageCode> & messages); from the consumer
	AclAttributeReadCommand(ConsumerInterface * consumer,
			const string & roleId,
			const CoreInfo_t * coreInfo):ProducerInterface(consumer),
			coreInfo(coreInfo),
			roleId(roleId){
		ASSERT(coreInfo != NULL);
		ASSERT(this->getConsumer() != NULL);
		ASSERT(this->getConsumer()->getTransaction());
		clusterReadview = ((ReadviewTransaction *)(this->getTransaction().get()))->getReadview();
	}
	~AclAttributeReadCommand();

	SP(Transaction) getTransaction(){
		if(this->getConsumer() == NULL){
			ASSERT(false);
			return SP(Transaction)();
		}
		return this->getConsumer()->getTransaction();
	}

	void produce(){

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

	void readListOfAttributes(NodeTargetShardInfo & target){
		// send attribute list read notification to target node
		SP(AclAttributeReadNotification) notif = SP(AclAttributeReadNotification)(
				new AclAttributeReadNotification(roleId, target, clusterReadview));
		ConcurrentNotifOperation * listSender =
				new ConcurrentNotifOperation(notif, ShardingAclAttrReadACKMessageType, ShardManager::getCurrentNodeId(), this);
		ShardManager::getStateMachine()->registerOperation(listSender);
	}

	// response which contains the list of attributes comes to this function
	void end(map<NodeId, SP(ShardingNotification) > & replies){
		if(replies.size() != 1){
			ASSERT(false);
			Logger::sharding(Logger::Error, "%d replies are received while waiting for a list of attributes." , replies.size());
			finalize();
			return;
		}


		AclAttributeReadNotification::ACK * listNotif = (AclAttributeReadNotification::ACK *)replies.begin()->second.get();
		vector<string> listOfAttributes = listNotif->getListOfAttributes();
		finalize(listOfAttributes);
	}

	void finalize(vector<string> attributes = vector<string>()){
		if(this->getConsumer() != NULL){
			this->getConsumer()->consume(attributes, messageCodes);
		}
	}

	string getName() const {return "acl-attribute-read-command";};
private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	const CoreInfo_t * coreInfo;
	const string roleId;

	vector<JsonMessageCode> messageCodes;

};


}
}

#endif // __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__
