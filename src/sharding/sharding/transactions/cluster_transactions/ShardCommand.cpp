
#include "ShardCommnad.h"
#include "../../notifications/CommandNotification.h"
#include "../../metadata_manager/ResourceMetadataManager.h"
#include "../../state_machine/StateMachine.h"
#include "../../transactions/TransactionSession.h"
#include "sharding/processor/Partitioner.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardCommand::ShardCommand(ConsumerInterface * consumer,
		unsigned coreId, ShardCommandCode commandCode,
		const string & filePath):coreId(coreId), commandCode(commandCode), ProducerInterface(consumer){
	this->filePath = filePath;
	this->commandCode = ShardCommandCode_Merge;
	this->dataSavedFlag = false;
	this->trans = NULL;
	ShardManager::getReadview(clusterReadview);
}

ShardCommand::~ShardCommand(){}


void ShardCommand::produce(){
	if(! dataSavedFlag){
		if (!  partition(this->targets)){
			return;
		}
	}
	for(unsigned i = 0 ; i < targets.size() ; ++i){
		notifications.push_back(std::make_pair(SP(CommandNotification)
				(new CommandNotification(clusterReadview, targets.at(i), commandCode, filePath)), targets.at(i).getNodeId()));
	}
	ConcurrentNotifOperation * commandSender = new ConcurrentNotifOperation(StatusMessageType, notifications, this);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(commandSender);
}

bool ShardCommand::partition(vector<NodeTargetShardInfo> & targets){
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	ShardManager::getReadview(clusterReadview);
	if(coreId == (unsigned)-1){
		vector<const CoreInfo_t *> cores;
		clusterReadview->getAllCores(cores);
		for(unsigned cid = 0 ; cid < cores.size() ; ++cid){
			CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(cores.at(cid)->getCoreId()));
			partitioner->getAllTargets(targets);
			delete partitioner;
		}
	}else{
		CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
		partitioner->getAllTargets(targets);
		delete partitioner;
	}
    // if there is no targets, return to consumer
    if(targets.size() > 0){
    	return true;
    }
    this->getTransaction()->getSession()->response->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_All_Shards_Down_Error));
    this->getTransaction()->getSession()->response->finalizeOK();
	map<NodeOperationId , SP(ShardingNotification)> emptyResult;
	finalize(emptyResult);
	this->getTransaction()->setUnattached();
	return false;
}
// process coming back from distributed conversation to aggregate the results of
// this command
void ShardCommand::end_(map<NodeOperationId , SP(ShardingNotification)> & replies){
	switch (this->commandCode) {
		case ShardCommandCode_SaveData_SaveMetadata:
		{
			if(! dataSavedFlag){
				// data save replies has arrived
				if(isSaveSuccessful(replies)){
					dataSavedFlag = true;
					// now let's start metadata save
					this->commandCode = ShardCommandCode_SaveMetadata;
					produce();
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

bool ShardCommand::isSaveSuccessful(map<NodeOperationId , SP(ShardingNotification)> & replies) const{
	for(map<NodeOperationId , SP(ShardingNotification)>::iterator replyItr = replies.begin();
			replyItr != replies.end(); ++replyItr){
		SP(CommandStatusNotification) nodeStatus = boost::dynamic_pointer_cast<CommandStatusNotification>(replyItr->second);
		const vector<CommandStatusNotification::ShardStatus *> & shardsStatus =
				nodeStatus->getShardsStatus();
		for(unsigned i = 0 ; i < shardsStatus.size(); ++i){
			if(! shardsStatus.at(i)->getStatusValue()){
				return false;
			}
		}
	}
	return true;
}

void ShardCommand::finalize(map<NodeOperationId , SP(ShardingNotification)> & replies){

	// final result that we give out to the "CommandStatusCallbackInterface * consumer" member.
	map<NodeId, vector<CommandStatusNotification::ShardStatus *> > result;

	// the following logic is to remove the ShardingNotification wrapper which is
	// around ShardStatus * (key value)
	for(map<NodeOperationId , SP(ShardingNotification)>::iterator replyItr = replies.begin();
			replyItr != replies.end(); ++replyItr){
		NodeOperationId nodeOperationId = replyItr->first;
		SP(CommandStatusNotification) nodeStatus = boost::dynamic_pointer_cast<CommandStatusNotification>(replyItr->second);
		const vector<CommandStatusNotification::ShardStatus *> & shardsStatus =
				nodeStatus->getShardsStatus();
		// append the shards status in key value of nodeOperationId.nodeId
		if(result.find(nodeOperationId.nodeId) == result.end()){
			result[nodeOperationId.nodeId] = vector<CommandStatusNotification::ShardStatus *>();
		}
		// put all shardsStatus pointers in the result
		result[nodeOperationId.nodeId].insert(result[nodeOperationId.nodeId].begin(), shardsStatus.begin(), shardsStatus.end());
	}

	// call callback from the consumer
	this->getConsumer()->consume(result);
}

}

}
