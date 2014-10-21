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



    /*
     * The main function responsible of aggregating status (success or failure) results
     */
    void callBack(PendingMessage<RequestWithStatusResponse, CommandStatus> * message){

        if(message == NULL){
            return;
        }

        if(message->getResponseObject() == NULL){
        	processTimeout(message, ResponseAggregatorMetadata());
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
					const WriteCommandNotification * sentInsetUpdateRequest = (const WriteCommandNotification *)(sentRequest);

					Json::Value recordShardResponse =
							HTTPJsonRecordOperationResponse::getRecordJsonResponse(sentInsetUpdateRequest->getRecord()->getPrimaryKey(),
							(sentInsetUpdateRequest->getInsertOrUpdate() == WriteCommandNotification::DP_INSERT?c_action_insert:c_action_update),
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
				default:
					ASSERT(false);
					break;
			}


		}

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
