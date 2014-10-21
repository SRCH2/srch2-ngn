
#include "ShardCommnad.h"
#include "../../state_machine/notifications/CommandNotification.h"

#include "../../metadata_manager/ResourceMetadataManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardCommand::ShardCommand(CommandStatusAggregationCallbackInterface * consumer,
		unsigned coreId, ShardCommandCode commandCode = ShardCommandCode_Merge,
		const string & filePath = ""):coreId(coreId), commandCode(commandCode){
	ASSERT(consumer != NULL);
	this->consumer = consumer;
	this->filePath = filePath;
	this->commandCode = ShardCommandCode_Merge;
	this->dataSavedFlag = false;
	this->finalizedFlag = false;

	partition(this->targets);
}

ShardCommand::~ShardCommand(){}

void ShardCommand::partition(vector<NodeTargetShardInfo> & targets){
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	ShardManager::getReadview(clusterReadview);
	CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
    partitioner->getAllTargets(targets);
    delete partitioner;
    // if there is no targets, return to consumer
    if(targets.size() > 0){
    	return;
    }
    brokerSideInformationJson->addError(HTTPJsonResponse::getJsonSingleMessage(HTTP_JSON_All_Shards_Down_Error));
    brokerSideInformationJson->finalizeOK();
	this->consumer->setDeleteTopDown(); // top object must take care if it's deletion. Delete doesn't start from
	map<NodeOperationId , ShardingNotification *> emptyResult;
	finalize(emptyResult);
}
// process coming back from distributed conversation to aggregate the results of
// this command
void ShardCommand::receiveReplies(map<NodeOperationId , ShardingNotification *> replies){
	switch (this->commandCode) {
		case ShardCommandCode_SaveData_SaveMetadata:
		{
			if(! dataSavedFlag){
				// data save replies has arrived
				if(isSaveSuccessful(replies)){
					dataSavedFlag = true;
					// now let's start metadata save
					this->commandCode = ShardCommandCode_SaveMetadata;
					start();
					return;
				}
			}
			// state : metadata save replies
			// state : saving data was not successful
			// 		just return the results, wrapper layer will use messages
			// 		to notify the requester properly.
			// we are done
			finalize(replies);
			return;
		}
		case ShardCommandCode_SaveData:
		case ShardCommandCode_SaveMetadata:
		case ShardCommandCode_Export:
	    case ShardCommandCode_Commit:
	    case ShardCommandCode_Merge:
	    case ShardCommandCode_MergeSetOn:
	    case ShardCommandCode_MergeSetOff:
	    case ShardCommandCode_ResetLogger:
	    {
	    	finalize(replies);
			return;
	    }
	}
}

TRANS_ID ShardCommand::lastCallback(void * args){
	if(! finalizedFlag ){
		return TRANS_ID_NULL;
	}else{
		return consumer->lastCallback(args);
	}
}

void ShardCommand::abort(int error_code){
	Logger::error("Error received in shard command. Error code : %d", error_code);
	map<NodeOperationId , ShardingNotification *> noResults;
	finalize(noResults);
}


void ShardCommand::start(){
	for(unsigned i = 0 ; i < targets.size() ; ++i){
		notifications.push_back(std::make_pair(new CommandNotification(targets.at(i), commandCode, filePath), targets.at(i).getNodeId()));
	}
	ConcurrentNotifOperation * commandSender = new ConcurrentNotifOperation(StatusMessageType, notifications, this);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(commandSender);
}


bool ShardCommand::isSaveSuccessful(map<NodeOperationId , ShardingNotification *> & replies) const{
	for(map<NodeOperationId , ShardingNotification *>::iterator replyItr = replies.begin();
			replyItr != replies.end(); ++replyItr){
		CommandStatusNotification * nodeStatus = (CommandStatusNotification *) replyItr->second;
		vector<CommandStatusNotification::ShardStatus *> & shardsStatus =
				nodeStatus->getShardsStatus();
		for(unsigned i = 0 ; i < shardsStatus.size(); ++i){
			if(! shardsStatus.at(i)->getStatusValue()){
				return false;
			}
		}
	}
	return true;
}

void ShardCommand::finalize(map<NodeOperationId , ShardingNotification *> & replies){

	// final result that we give out to the "CommandStatusCallbackInterface * consumer" member.
	map<NodeId, vector<CommandStatusNotification::ShardStatus *> > result;

	// the following logic is to remove the ShardingNotification wrapper which is
	// around ShardStatus * (key value)
	for(map<NodeOperationId , ShardingNotification *>::iterator replyItr = replies.begin();
			replyItr != replies.end(); ++replyItr){
		NodeOperationId nodeOperationId = replyItr->first;
		CommandStatusNotification * nodeStatus = (CommandStatusNotification *) replyItr->second;
		vector<CommandStatusNotification::ShardStatus *> & shardsStatus =
				nodeStatus->getShardsStatus();
		// append the shards status in key value of nodeOperationId.nodeId
		if(result.find(nodeOperationId.nodeId) == result.end()){
			result[nodeOperationId.nodeId] = vector<CommandStatusNotification::ShardStatus *>();
		}
		// put all shardsStatus pointers in the result
		result[nodeOperationId.nodeId].insert(result[nodeOperationId.nodeId].begin(), shardsStatus.begin(), shardsStatus.end());
	}

	// call callback from the consumer
	this->finalizedFlag = true;
	consumer->receiveStatus(result);
}

}

}
