#ifndef __SHARDING_SHARDING_SHARD_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_SHARD_COMMAND_HTTP_H__

#include "../../state_machine/State.h"
#include "../../state_machine/notifications/Notification.h"
#include "../../state_machine/notifications/CommandStatusNotification.h"
#include "../../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"
#include "ShardCommnad.h"

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
class ShardCommandHttpHandler: public Transaction, public CommandStatusAggregationCallbackInterface {
public:

	static void runCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId , ShardCommandCode commandCode){
		ShardCommandHttpHandler * commandHttpHandler = new ShardCommandHttpHandler(clusterReadview, req, coreId, commandCode);
		ShardManager::getShardManager()->getStateMachine()->registerTransaction(commandHttpHandler);
		if ( ! commandHttpHandler->run()){
			ShardManager::getShardManager()->getStateMachine()->removeTransaction(commandHttpHandler->getTID());
		}
		return ;
	}
private:
	ShardCommandHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId , ShardCommandCode commandCode){
		this->brokerSideInformationJson = boost::shared_ptr<HTTPJsonShardOperationResponse > (new HTTPJsonShardOperationResponse(req));
		this->req = req;
		this->coreId = coreId;
		this->commandCode = commandCode;
		initActionName();
		this->clusterReadview = clusterReadview;
	}

	~ShardCommandHttpHandler(){
		if(shardCommand != NULL){
			delete shardCommand;
		}
	}

	bool run(){
	    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCore(coreId);
	    switch (req->type) {
	    case EVHTTP_REQ_PUT: {
	    	string filePath = "";
	    	if(commandCode == ShardCommandCode_Export || commandCode == ShardCommandCode_ResetLogger){
	            if (indexDataContainerConf->getSearchResponseFormat() == RESPONSE_WITH_STORED_ATTR) {
	                std::stringstream log_str;
	                evkeyvalq headers;
	                evhttp_parse_query(req->uri, &headers);
	                char *exportedDataFileName = NULL;
	    	    	if(commandCode == ShardCommandCode_Export){
						exportedDataFileName = evhttp_find_header(&headers, URLParser::nameParamName);
	    	    	}else { ///commandCode == ShardCommandCode_ResetLogger
						exportedDataFileName = evhttp_find_header(&headers, URLParser::logNameParamName);
	    	    	}
	                // TODO : should we free exportedDataFileName?
	                if(exportedDataFileName){
	                	filePath = string(exportedDataFileName);
	                }else{
	                    brokerSideInformationJson->finalizeInvalid();
	                    return false;
	                }
	            }else{
	                brokerSideInformationJson->addError(HTTPJsonResponse::getJsonSingleMessage(HTTP_JSON_Search_Res_Format_Wrong_Error));
	            	brokerSideInformationJson->finalizeOK();
	            	return false;
	            }
	    	}
	    	if(commandCode == ShardCommandCode_Merge){
                evkeyvalq headers;
                evhttp_parse_query(req->uri, &headers);
                const char * flagSet = evhttp_find_header(&headers, URLParser::setParamName);
                if(flagSet){
                	if(((string)"0").compare(flagSet) == 0){
                		commandCode = ShardCommandCode_MergeSetOff;
                	}else if (((string)"1").compare(flagSet) == 0){
                		commandCode = ShardCommandCode_MergeSetOn;
                	}else{
    	                brokerSideInformationJson->addError(HTTPJsonResponse::getJsonSingleMessage(HTTP_JSON_Merge_Parameter_Not_Recognized));
    	            	brokerSideInformationJson->finalizeOK();
    	            	return false;
                	}
                }
	    	}
			shardCommand = new ShardCommand(this, coreId, commandCode, filePath);
			if(this->isDeleteTopDown()){
				return false;
			}
			shardCommand->start();
	    	return true;
	    }
	    default:
	    {
	        brokerSideInformationJson->finalizeInvalid();
	    	return false;
	    }
	    }
	    return false;
	}


	void receiveStatus(map<NodeId, vector<CommandStatusNotification::ShardStatus *> > shardsStatus){
		for(unsigned i = 0 ; i < shardsStatus.size(); ++i){
			CommandStatusNotification::ShardStatus * recordShardResult = shardsStatus.at(i);
			this->brokerSideInformationJson->addShardResponse(action_name.c_str(),
					recordShardResult->getStatusValue(), recordShardResult->messages);
			delete shardsStatus;
		}
	}
	TRANS_ID lastCallback(void * args){
		return this->getTID();
	}


	ShardingTransactionType getTransactionType(){
		return ShardingTransactionType_ShardCommandCode;
	}
private:
	boost::shared_ptr<HTTPJsonShardOperationResponse > brokerSideInformationJson;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	evhttp_request *req;
	unsigned coreId;
	ShardCommandCode commandCode;

	ShardCommand * shardCommand;

	void initActionName(){
		switch (commandCode) {
			case ShardCommandCode_SaveData_SaveMetadata:
				action_name = string(c_action_save);
				break;
			case ShardCommandCode_SaveData:
				action_name = string(c_action_save_data);
				break;
			case ShardCommandCode_SaveMetadata:
				action_name = string(c_action_save_metadata);
				break;
			case ShardCommandCode_Export:
				action_name = string(c_action_export);
				break;
			case ShardCommandCode_Commit:
				action_name = string(c_action_commit);
				break;
			case ShardCommandCode_Merge:
				action_name = string(c_action_merge);
				break;
			case ShardCommandCode_MergeSetOn:
				action_name = string(c_action_merge_on);
				break;
			case ShardCommandCode_MergeSetOff:
				action_name = string(c_action_merge_off);
				break;
			case ShardCommandCode_ResetLogger:
				action_name = string(c_action_reset_logger);
				break;
		}
	}
	string action_name;
};
}
}

#endif // __SHARDING_SHARDING_SHARD_COMMAND_HTTP_H__
