#ifndef __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__
#define __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__

#include "../../state_machine/State.h"
#include "../../state_machine/ConsumerProducer.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"
#include "../Transaction.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"

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
			const CoreInfo_t * coreInfo);
	~AclAttributeReadCommand(){};

	SP(Transaction) getTransaction();

	void produce();

	void readListOfAttributes(NodeTargetShardInfo & target);


	bool shouldAbort(const NodeId & failedNode);

	// response which contains the list of attributes comes to this function
	void end(map<NodeId, SP(ShardingNotification) > & replies);

	void finalize(bool status = false, vector<unsigned> listOfRefiningAttributes = vector<unsigned>(),
			vector<unsigned> listOfSearchableAttributes = vector<unsigned>());

	string getName() const {return "acl-attribute-read-command";};
private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	const CoreInfo_t * coreInfo;
	const string roleId;
	NodeTargetShardInfo target;

	vector<JsonMessageCode> messageCodes;

};


}
}

#endif // __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__
