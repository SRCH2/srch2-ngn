#ifndef __SHARDING_SHARDING_CLUSTER_TRANS_SHARD_COMMAND_H__
#define __SHARDING_SHARDING_CLUSTER_TRANS_SHARD_COMMAND_H__

#include "./ConcurrentNotifOperation.h"
#include "../../state_machine/State.h"
#include "../../state_machine/notifications/Notification.h"
#include "../../state_machine/notifications/CommandStatusNotification.h"
#include "../../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Saves the indices and the cluster metadata on all nodes in the cluster.
 * NOTE : this operation assumes all shards are locked in S mode
 * 1. request all nodes to save their indices
 * 2. When all nodes saved their indices, request all nodes to save their cluster metadata
 * 3. When all nodes acked metadata save, write the metadata on disk and done.
 */
class ShardCommand: public AggregatorCallbackInterface {
public:

	ShardCommand(CommandStatusAggregationCallbackInterface * consumer,
			unsigned coreId, ShardCommandCode commandCode = ShardCommandCode_Merge,
			const string & filePath = "");
	~ShardCommand();



	void partition(vector<NodeTargetShardInfo> & targets);


	void start();

	// process coming back from distributed conversation to aggregate the results of
	// this command
	void receiveReplies(map<NodeOperationId , ShardingNotification *> replies);

	TRANS_ID lastCallback(void * args);

	void abort(int error_code);

	void setMessageChannel(boost::shared_ptr<HTTPJsonShardOperationResponse > brokerSideInformationJson){
		this->brokerSideInformationJson = brokerSideInformationJson;
	}

private:
	const unsigned coreId;
	const ShardCommandCode commandCode;
	vector<NodeTargetShardInfo> targets;
	vector<std::pair<ShardingNotification * , NodeId> > notifications;
	CommandStatusAggregationCallbackInterface * consumer;

	bool finalizedFlag ;

	string filePath ; // holds either jsonFilePath or the newLogFilePath

	bool dataSavedFlag;

	boost::shared_ptr<HTTPJsonShardOperationResponse > brokerSideInformationJson;
//			boost::shared_ptr<HTTPJsonShardOperationResponse > (new HTTPJsonShardOperationResponse(req));


	bool isSaveSuccessful(map<NodeOperationId , ShardingNotification *> & replies) const;

	void finalize(map<NodeOperationId , ShardingNotification *> & replies);

};


}

}


#endif // __SHARDING_SHARDING_CLUSTER_TRANS_SHARD_COMMAND_H__
