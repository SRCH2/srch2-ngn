#ifndef __SHARDING_PROCESSOR_COMMAND_STATUS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_COMMAND_STATUS_AGGREGATOR_AND_PRINT_H_

#include "./DistributedProcessorAggregator.h"
#include "../serializables/SerializableCommandStatus.h"
#include "../serializables/SerializableInsertUpdateCommandInput.h"
#include "../serializables/SerializableDeleteCommandInput.h"
#include "../serializables/SerializableSerializeCommandInput.h"
#include "../serializables/SerializableResetLogCommandInput.h"
#include "../serializables/SerializableCommitCommandInput.h"
#include "../PendingMessages.h"
#include "server/HTTPJsonResponse.h"
#include <string>
#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class RequestWithStatusResponse>
class StatusAggregator : public DistributedProcessorAggregator<RequestWithStatusResponse,CommandStatus> {
public:


    StatusAggregator(ConfigManager * configurationManager, evhttp_request *req,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, unsigned coreId, unsigned multiRouteMode = 0):
    			DistributedProcessorAggregator<RequestWithStatusResponse,CommandStatus>(clusterReadview, coreId),
    			requestType(RequestWithStatusResponse::messageType()){
        this->configurationManager = configurationManager;
        this->req = req;
        this->multiRouteMode = multiRouteMode; // this is the case where aggregator is shared with multiple callbacks
        this->preProcessCalled = false;
        this->numberOfFinalizedCallsSoFar = 0;
    }


    void setJsonRecordOperationResponse(boost::shared_ptr<HTTPJsonRecordOperationResponse > brokerSideRecordOpInfoJson){
        boost::unique_lock< boost::shared_mutex > lock(_access);
        this->brokerSideRecordOpInfoJson = brokerSideRecordOpInfoJson;
    }

    void setJsonShardOperationResponse(boost::shared_ptr<HTTPJsonShardOperationResponse > brokerSideShardOpInfoJson){
        boost::unique_lock< boost::shared_mutex > lock(_access);
        this->brokerSideShardOpInfoJson = brokerSideShardOpInfoJson;
    }

    /*
     * This function is always called by RoutingManager as the first call back function
     */
    void preProcess(ResponseAggregatorMetadata metadata){
        if(multiRouteMode > 0){
            // we need to make sure preProcess is only called once
            boost::unique_lock< boost::shared_mutex > lock(_access);
            if(preProcessCalled == true){ // already called once
                return;
            }else{ // not called yet, the first time
                preProcessCalled = true;
            }
        }
        // any preprocess code must be written here :
        // right now, nothing to do in preProcess.
    }

    /*
     * This function is called by RoutingManager if a timeout happens, The call to
     * this function must be between preProcessing(...) and callBack()
     */
    /*
     * This function is called by RoutingManager if a timeout happens, The call to
     * this function must be between preProcessing(...) and callBack()
     */
    void processTimeout(PendingMessage<RequestWithStatusResponse, CommandStatus> * message,
            ResponseAggregatorMetadata metadata){

        if(message == NULL){
            return;
        }
		boost::unique_lock< boost::shared_mutex > lock(_access);
		Json::Value timeoutWarning(Json::objectValue);
		timeoutWarning[c_message] = HTTPJsonResponse::getJsonSingleMessageStr(HTTP_JSON_Node_Timeout_Warning);
		timeoutWarning['node_name'] = Json::Value(this->getClusterReadview()->getNode(message->getNodeId()).getName());


		switch (requestType) {
			case InsertUpdateCommandMessageType:
			case DeleteCommandMessageType:
				this->brokerSideRecordOpInfoJson->addWarning(timeoutWarning);
				break;
			case SerializeCommandMessageType:
			case CommitCommandMessageType:
			case MergeCommandMessageType:
			case ResetLogCommandMessageType:
				this->brokerSideShardOpInfoJson->addWarning(timeoutWarning);
				break;
			default:
				ASSERT(false);
				break;
		}

    }

    /*
     * The main function responsible of aggregating status (success or failure) results
     */
    void callBack(PendingMessage<RequestWithStatusResponse, CommandStatus> * message){

        if(message == NULL){
            return;
        }
        boost::unique_lock< boost::shared_mutex > lock(_access);

        this->getClusterReadview()->getNode(message->getNodeId()).getName();
        const string coreName = this->getClusterReadview()->getCore(this->getCoreId())->getName();

        const RequestWithStatusResponse * sentRequest = message->getRequestObject();

        CommandStatus * cammandStatusResponse = message->getResponseObject();

        vector<CommandStatus::ShardResults *> allRecordShardResults = cammandStatusResponse->getShardResults();

		for(unsigned i = 0 ; i < allRecordShardResults.size(); ++i){
			CommandStatus::ShardResults * recordShardResult = allRecordShardResults.at(i);

			switch (requestType) {
				case InsertUpdateCommandMessageType:{
					const InsertUpdateCommand * sentInsetUpdateRequest = (const InsertUpdateCommand *)(sentRequest);

					Json::Value recordShardResponse =
							HTTPJsonRecordOperationResponse::getRecordJsonResponse(sentInsetUpdateRequest->getRecord()->getPrimaryKey(),
							(sentInsetUpdateRequest->getInsertOrUpdate() == InsertUpdateCommand::DP_INSERT?c_action_insert:c_action_update),
							recordShardResult->statusValue , coreName);
					HTTPJsonResponse::appendDetails(recordShardResponse, recordShardResult->messages);
					this->brokerSideRecordOpInfoJson->addRecordShardResponse(recordShardResponse);
					break;
				}
				case DeleteCommandMessageType:{
		        	const DeleteCommand * sentDeleteRequest = (const DeleteCommand *)(sentRequest);
					Json::Value recordShardResponse =
							HTTPJsonRecordOperationResponse::getRecordJsonResponse(sentDeleteRequest->getPrimaryKey(),
							c_action_delete, recordShardResult->statusValue , coreName);
					HTTPJsonResponse::appendDetails(recordShardResponse, recordShardResult->messages);
					this->brokerSideRecordOpInfoJson->addRecordShardResponse(recordShardResponse);
					break;
				}
				case SerializeCommandMessageType:{
		        	const SerializeCommand * sentSerializeRequest = (const SerializeCommand *)(sentRequest);
		        	switch (sentSerializeRequest->getIndexOrRecord()) {
						case SerializeCommand::SERIALIZE_INDEX:
							this->brokerSideShardOpInfoJson->addShardResponse(c_action_save,
									recordShardResult->statusValue, recordShardResult->messages);
							break;
						case SerializeCommand::SERIALIZE_RECORDS:
							this->brokerSideShardOpInfoJson->addShardResponse(c_action_export,
									recordShardResult->statusValue, recordShardResult->messages);
							break;
					}
					break;
				}
				case CommitCommandMessageType:{
					this->brokerSideShardOpInfoJson->addShardResponse(c_action_commit, recordShardResult->statusValue, recordShardResult->messages);
					break;
				}
				case MergeCommandMessageType:{
					this->brokerSideShardOpInfoJson->addShardResponse(c_action_merge, recordShardResult->statusValue, recordShardResult->messages);
					break;
				}
				case ResetLogCommandMessageType:{
					this->brokerSideShardOpInfoJson->addShardResponse(c_action_reset_logger, recordShardResult->statusValue, recordShardResult->messages);
					break;
				}
				default:
					ASSERT(false);
					break;
			}


		}

    }

    void callBack(vector<PendingMessage<RequestWithStatusResponse, CommandStatus> * > messagesArg){

        //TODO shard info can be better than just an index
        for(typename vector<PendingMessage<RequestWithStatusResponse, CommandStatus> * >::iterator
                messageItr = messagesArg.begin(); messageItr != messagesArg.end(); ++messageItr){
        	callBack(*messageItr);
        }
    }
    /*
     * The last call back function called by RoutingManager in all cases.
     * Example of call back call order for search :
     * 1. preProcessing()
     * 2. timeoutProcessing() [only if some shard times out]
     * 3. aggregateSearchResults()
     * 4. finalize()
     */
    void finalize(ResponseAggregatorMetadata metadata){
        if(multiRouteMode > 0){
            // we need to make sure finalize is only called once
            boost::unique_lock< boost::shared_mutex > lock(_access);
            numberOfFinalizedCallsSoFar ++;
            if(multiRouteMode != numberOfFinalizedCallsSoFar){
                return;
            }
            // ready to be finalized because all requests are finalized
        }
        //... code here
    }


private:



    ConfigManager * configurationManager;
    evhttp_request *req;
    mutable boost::shared_mutex _access;
    unsigned multiRouteMode;
    bool preProcessCalled;
    unsigned numberOfFinalizedCallsSoFar;

	const ShardingMessageType requestType ;

    boost::shared_ptr<HTTPJsonRecordOperationResponse >brokerSideRecordOpInfoJson;
    boost::shared_ptr<HTTPJsonShardOperationResponse > brokerSideShardOpInfoJson;
};

}
}

#endif // __SHARDING_PROCESSOR_COMMAND_STATUS_AGGREGATOR_AND_PRINT_H_
