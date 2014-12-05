
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
		const string & filePath):ProducerInterface(consumer), coreId(coreId), commandCode(commandCode){
	ASSERT(this->getTransaction() != NULL);
	this->filePath = filePath;
	this->dataSavedFlag = false;
	this->locker = NULL;
	this->releaser = NULL;
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
}

ShardCommand::~ShardCommand(){}


void ShardCommand::produce(){
	Logger::sharding(Logger::Step, "ShardCommand(code : %d)| Starting core shard command operation", commandCode);
	if(! dataSavedFlag){
		if (!  computeTargets(this->targets)){
			map<NodeOperationId , SP(ShardingNotification)> emptyResult;
			finalize(emptyResult);
			return;
		}
		switch (commandCode) {
			case ShardCommandCode_SaveData:
			case ShardCommandCode_SaveMetadata:
			case ShardCommandCode_SaveData_SaveMetadata:
			case ShardCommandCode_Merge:
			case ShardCommandCode_MergeSetOn:
			case ShardCommandCode_MergeSetOff:
				lock();
				return;
			default:
				break;
		}
	}

	performCommand();
}

void ShardCommand::performCommand(){
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	ShardManager::getReadview(clusterReadview);
	notifications.clear();
	for(unsigned i = 0 ; i < targets.size() ; ++i){
		notifications.push_back(std::make_pair(SP(CommandNotification)
				(new CommandNotification(clusterReadview, targets.at(i), commandCode, filePath)), targets.at(i).getNodeId()));
	}
	if(notifications.empty()){
		map<NodeOperationId , SP(ShardingNotification)> emptyResult;
		finalize(emptyResult);
		return;
	}
	ConcurrentNotifOperation * commandSender = new ConcurrentNotifOperation(StatusMessageType, notifications, this);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(commandSender);
}

void ShardCommand::lock(){
	vector<ClusterShardId> shardIds;
	getSortedListOfClusterShardIDs(shardIds);
	if(shardIds.empty()){
		performCommand();
		return;
	}
	stringstream ss;
	for(unsigned i = 0 ; i < shardIds.size(); ++i){
		if(i > 0){
			ss << "|";
		}
		ss << shardIds.at(i).toString();
	}
	this->locker = new AtomicLock(shardIds, currentOpId, LockLevel_S , this);
	Logger::sharding(Logger::Step, "ShardCommand(code : %d)| locking sorted list of shard Ids : %s", commandCode, ss.str().c_str());
	this->locker->produce();
}

void ShardCommand::release(){
	vector<ClusterShardId> shardIds;
	getSortedListOfClusterShardIDs(shardIds);
	stringstream ss;
	for(unsigned i = 0 ; i < shardIds.size(); ++i){
		if(i > 0){
			ss << "|";
		}
		ss << shardIds.at(i).toString();
	}
	this->releaser = new AtomicRelease(shardIds, currentOpId , this);
	Logger::sharding(Logger::Step, "ShardCommand(code : %d)| releasing sorted list of shard Ids : %s", commandCode, ss.str().c_str());
	this->releaser->produce();
}

void ShardCommand::consume(bool granted){
	if(! granted){
		// lock step : abort
		// release step : finish normally.
		if(releaser == NULL){
		    this->getTransaction()->getSession()->response->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_Json_Cannot_Acquire_Locks));
			map<NodeOperationId , SP(ShardingNotification)> emptyResult;
			finalize(emptyResult);
			return;
		}else{
			finalize(aggregatedResult);
			return;
		}
	}else{
		if(releaser == NULL){
			performCommand();
			return;
		}else{
			finalize(aggregatedResult);
			return;
		}
	}
};

bool ShardCommand::computeTargets(vector<NodeTargetShardInfo> & targets){

	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	ShardManager::getReadview(clusterReadview);
	if(coreId == (unsigned)-1){
		ASSERT(false); // we don't support this for now.
		vector<const CoreInfo_t *> cores;
		clusterReadview->getAllCores(cores);
		for(unsigned cid = 0 ; cid < cores.size() ; ++cid){
			CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(cores.at(cid)->getCoreId()));
			partitioner->getAllTargets(targets);
			delete partitioner;
		}
	}else{
		CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreId));
		if(partitioner == NULL || clusterReadview->getPartitioner(coreId)->isCoreLocked()){
			Logger::sharding(Logger::Detail, "ShardCommand(code : %d)| Core is currently locked. Request rejected.", commandCode);
		    this->getTransaction()->getSession()->response->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_All_Shards_Down_Error));
		    this->getTransaction()->getSession()->response->finalizeOK();
			delete partitioner;
			return false;
		}
		partitioner->getAllTargets(targets);
		delete partitioner;
	}
    // if there is no targets, return to consumer
    if(targets.size() > 0){
    	stringstream ss;
    	for(unsigned i = 0; i < targets.size(); ++i){
    		if(i != 0){
    			ss << "-";
    		}
    		ss << targets.at(i).toString();
    	}
    	Logger::sharding(Logger::Detail, "ShardCommand(code : %d)| Targets are : %s", commandCode, ss.str().c_str());
    	return true;
    }
	Logger::sharding(Logger::Detail, "ShardCommand(code : %d)| No targets found, Returning unattached.", commandCode);
    this->getTransaction()->getSession()->response->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_All_Shards_Down_Error));
    this->getTransaction()->getSession()->response->finalizeOK();
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
			    	Logger::sharding(Logger::Detail, "ShardCommand(code : %d)| Data shards save was successful.", commandCode);
					dataSavedFlag = true;
					// now let's start metadata save
					this->commandCode = ShardCommandCode_SaveMetadata;
					if(replies.empty()){
						map<NodeOperationId , SP(ShardingNotification)> emptyResult;
						finalize(emptyResult);
						return;
					}
					performCommand();
					return;
				}else{
			    	Logger::sharding(Logger::Detail, "ShardCommand(code : %d)| Data shards save was NOT successful.", commandCode);
			    	this->getTransaction()->getSession()->response->addError(JsonResponseHandler::
			    			getJsonSingleMessage(HTTP_Json_Cannot_Acquire_Locks));
					release();
					return;
				}
			}else{
				ASSERT(false);
				this->aggregatedResult = replies;
				release();
				return;
			}
		}
		case ShardCommandCode_SaveMetadata:
		case ShardCommandCode_SaveData:
	    case ShardCommandCode_Merge:
	    case ShardCommandCode_MergeSetOn:
	    case ShardCommandCode_MergeSetOff:
			this->aggregatedResult = replies;
			release();
			return;
		case ShardCommandCode_Export:
	    case ShardCommandCode_Commit:
	    case ShardCommandCode_ResetLogger:
	    	finalize(replies);
			return;
	    default:
	    {
	    	ASSERT(false);
			map<NodeOperationId , SP(ShardingNotification)> emptyResult;
			finalize(emptyResult);
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
	Logger::sharding(Logger::Step, "ShardCommand(code : %d)| Done. giving the results to the consumer : %s", commandCode,
			this->getConsumer() == NULL ? "NULL" : this->getConsumer()->getName().c_str());
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
		if(commandCode == ShardCommandCode_SaveMetadata || commandCode == ShardCommandCode_SaveData_SaveMetadata){
			CommandStatusNotification::ShardStatus * nodeShardStatus = new CommandStatusNotification::ShardStatus();
			nodeShardStatus->setStatusValue(true);
			result[nodeOperationId.nodeId].push_back(nodeShardStatus);
		}
	}

	// call callback from the consumer
	this->getConsumer()->consume(result);
}

}

}
