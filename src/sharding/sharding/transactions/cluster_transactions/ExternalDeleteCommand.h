#ifndef __SHARDING_SHARDING_CLUSTER_TRANS_DELETE_COMMAND_H__
#define __SHARDING_SHARDING_CLUSTER_TRANS_DELETE_COMMAND_H__

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
class DeleteCommand: public AggregatorCallbackInterface {
public:

	DeleteCommand(CommandStatusAggregationCallbackInterface * consumer,
			boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
					evhttp_request *req, unsigned coreId){
		this->consumer = consumer;
		this->clusterReadview = clusterReadview;
		this->req = req;
		this->coreInfo = clusterReadview->getCore(coreId);
	}
	~DeleteCommand(){

	}



	void partition(vector<NodeTargetShardInfo> & targets){
	    evkeyvalq headers;
	    evhttp_parse_query(req->uri, &headers);
	    brokerSideInformationJson->setHeaders(&headers);
	    //set the primary key of the record we want to delete
	    std::string primaryKeyName = coreInfo->getPrimaryKey();
	    const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());
		size_t sz;
		char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);
		// TODO : should we free pKeyParamName_cstar?

		//std::cout << "[" << termBoostsParamName_cstar << "]" << std::endl;
		const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
		free(pKeyParamName_cstar);

	    //TODO : we should parse more than primary key later
	    if (pKeyParamName){
			CorePartitioner * partitioner = new CorePartitioner(clusterReadview->getPartitioner(coreInfo->getCoreId()));
	        vector<NodeTargetShardInfo> targets;
	        partitioner->getAllWriteTargets(partitioner->hashDJB2(primaryKeyStringValue.c_str()),
	        		clusterReadview->getCurrentNodeId(), targets);
	        if(targets.size() == 0){
	        	Json::Value recordJsonResponse =
	        			HTTPJsonRecordOperationResponse::getRecordJsonResponse(primaryKeyStringValue,
	        					c_action_insert , false , coreInfo->getName());
	        	HTTPJsonRecordOperationResponse::addRecordError(recordJsonResponse, HTTP_JSON_All_Shards_Down_Error);
	        	brokerSideInformationJson->addRecordShardResponse(recordJsonResponse);
				delete partitioner;
				return;
	        }


	    }else{
	        brokerSideInformationJson->finalizeOK();
	        brokerSideInformationJson->addError(HTTPJsonResponse::getJsonSingleMessageStr(HTTP_JSON_PK_NOT_PROVIDED));
	        // Free the objects
	        evhttp_clear_headers(&headers);
	        return;
	    }
	}


	void start(){

	}

	// process coming back from distributed conversation to aggregate the results of
	// this command
	void receiveReplies(map<NodeOperationId , ShardingNotification *> replies){

	}

	TRANS_ID lastCallback(void * args);

	void abort(int error_code){

	}

	void setMessageChannel(boost::shared_ptr<HTTPJsonShardOperationResponse > brokerSideInformationJson){
		this->brokerSideInformationJson = brokerSideInformationJson;
	}

private:
	CommandStatusAggregationCallbackInterface * consumer;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	evhttp_request *req;
	const CoreInfo_t * coreInfo;
	CommandStatusAggregationCallbackInterface * consumer;

	bool finalizedFlag ;

	boost::shared_ptr<HTTPJsonRecordOperationResponse > brokerSideInformationJson;

	bool isSaveSuccessful(map<NodeOperationId , ShardingNotification *> & replies) const;

	void finalize(map<NodeOperationId , ShardingNotification *> & replies);

};


}

}


#endif // __SHARDING_SHARDING_CLUSTER_TRANS_DELETE_COMMAND_H__
