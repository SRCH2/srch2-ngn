#include "GetInfoAggregatorAndPrint.h"
#include "sharding/processor/PendingMessages.h"
#include "sharding/sharding/ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


GetInfoResponseAggregator::GetInfoResponseAggregator(ConfigManager * configurationManager,
		boost::shared_ptr<GetInfoJsonResponse > brokerSideShardInfo,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, unsigned coreId, bool debugRequest):
		DistributedProcessorAggregator<GetInfoCommand,GetInfoCommandResults>(clusterReadview, coreId){
	this->debugRequest = debugRequest;
    this->configurationManager = configurationManager;
    this->brokerSideInformationJson = brokerSideShardInfo;
    this->criterion = GetInfoAggregateCriterion_Core_Shard;
}

/*
 * This function is always called by RoutingManager as the first call back function
 */
void GetInfoResponseAggregator::preProcess(ResponseAggregatorMetadata metadata){

}
/*
 * This function is called by RoutingManager if a timeout happens, The call to
 * this function must be between preProcessing(...) and callBack()
 */
void GetInfoResponseAggregator::processTimeout(PendingMessage<GetInfoCommand,
        GetInfoCommandResults> * message,
        ResponseAggregatorMetadata metadata){
    if(message == NULL){
        return;
    }
	boost::unique_lock< boost::shared_mutex > lock(_access);
	Json::Value warinigNode = JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Node_Timeout_Warning);
	warinigNode[c_node_name] = getClusterReadview()->getNode(message->getNodeId()).getName();
	this->brokerSideInformationJson->addWarning(warinigNode);
}

void GetInfoResponseAggregator::callBack(PendingMessage<GetInfoCommand, GetInfoCommandResults> * message){

    boost::unique_lock< boost::shared_mutex > lock(_access);

    const string nodeName = getClusterReadview()->getNode(message->getNodeId()).getName();

	Json::Value nodeShardsJson(Json::objectValue);
	if(debugRequest){
		nodeShardsJson[c_node_name] = nodeName;
		nodeShardsJson[c_nodes_shards] = Json::Value(Json::arrayValue);
	}
    vector<GetInfoCommandResults::ShardResults *> shardResults = message->getResponseObject()->getShardResults();
    for(unsigned shardIdx = 0 ; shardIdx < shardResults.size(); ++shardIdx){
    	GetInfoCommandResults::ShardResults * shardResult = shardResults.at(shardIdx);
    	this->shardResults.push_back(std::make_pair(nodeName, shardResult));
    	if(debugRequest){
    		nodeShardsJson[c_nodes_shards].append(shardResult->shardId->toString());
    	}
    }
	if(debugRequest){
		this->brokerSideInformationJson->getNodeShardsRoot().append(nodeShardsJson);
	}
}
/*
 * The main function responsible of aggregating status (success or failure) results
 */
void GetInfoResponseAggregator::callBack(vector<PendingMessage<GetInfoCommand,
        GetInfoCommandResults> * > messages){

    for(vector<PendingMessage<GetInfoCommand,
            GetInfoCommandResults> * >::iterator messageItr = messages.begin();
            messageItr != messages.end() ; ++messageItr){
        if(*messageItr == NULL || (*messageItr)->getResponseObject() == NULL){
            continue;
        }
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
void GetInfoResponseAggregator::finalize(ResponseAggregatorMetadata metadata){

	this->brokerSideInformationJson->setResponseAttribute(c_cluster_name, getClusterReadview()->getClusterName());

	switch (criterion) {
		case GetInfoAggregateCriterion_Core_Shard:
		{
			vector<const CoreInfo_t *> allCores;
			getClusterReadview()->getAllCores(allCores);

			unsigned totalNumDocsInCluster = 0;

			for(unsigned cid = 0 ; cid < allCores.size(); ++cid){ // iterate on cores
				const CoreInfo_t * core = allCores.at(cid);
				vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > corePrimaryShardResults;
				vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > corePartitionResults;
				vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > nodeShardResults;
				vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > allShardResults;
				// NOTE : we ignore version id for core aggregation
				for(unsigned sid = 0 ; sid < shardResults.size(); ++sid){
					GetInfoCommandResults::ShardResults * shardResult = shardResults.at(sid).second;
					if(core->getCoreId() != shardResult->shardId->coreId){
						continue;
					}
					if(! shardResult->shardId->isClusterShard()){
						nodeShardResults.push_back(std::make_pair( shardResult , shardResult->healthInfo) );
						continue;
					}
					allShardResults.push_back(std::make_pair(shardResult , shardResult->healthInfo));

					bool replicaExists = false;
					for(unsigned cpId = 0; cpId < corePrimaryShardResults.size(); ++cpId){
						if(corePrimaryShardResults.at(cpId).first->shardId->isReplica(shardResult->shardId)){
							replicaExists = true;
							corePartitionResults.at(cpId).second.isMergeRequired =
									corePartitionResults.at(cpId).second.isMergeRequired ||
									shardResult->healthInfo.isMergeRequired;
							corePartitionResults.at(cpId).second.isBulkLoadDone =
									corePartitionResults.at(cpId).second.isBulkLoadDone &&
									shardResult->healthInfo.isBulkLoadDone;
							corePartitionResults.at(cpId).second.readCount += shardResult->healthInfo.readCount;
							break;
						}
					}
					if(! replicaExists){
						corePartitionResults.push_back(std::make_pair(shardResult, shardResult->healthInfo));
						corePrimaryShardResults.push_back(std::make_pair(shardResult, shardResult->healthInfo));
					}
				}
				IndexHealthInfo aggregatedCoreInfo;
				aggregateCoreInfo(aggregatedCoreInfo, corePartitionResults, nodeShardResults);

				totalNumDocsInCluster += aggregatedCoreInfo.docCount;

				this->brokerSideInformationJson->addCoreInfo(core,aggregatedCoreInfo,
						corePrimaryShardResults, corePartitionResults,
						nodeShardResults , allShardResults, debugRequest);
			}

			this->brokerSideInformationJson->setResponseAttribute(c_cluster_total_number_of_documnets, Json::Value(totalNumDocsInCluster));
			Json::Value nodesJson(Json::objectValue);
			vector<Node> allNodes;
			getClusterReadview()->getAllNodes(allNodes);
			nodesJson[c_nodes_count] = Json::Value((unsigned)allNodes.size());
			this->brokerSideInformationJson->setResponseAttribute(c_nodes, nodesJson);
			if(debugRequest){
				// Extra information only exposed in debugRequest.
				this->brokerSideInformationJson->getNodesRoot()[c_detail] = Json::Value(Json::arrayValue);
				ShardManager::getShardManager()->getNodeInfoJson(this->
						brokerSideInformationJson->getNodesRoot()[c_detail]);
			}
			break;

		}
		default:
			break;
	}

}

void GetInfoResponseAggregator::aggregateCoreInfo(IndexHealthInfo & aggregatedResult,
		vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > & allPartitionResults,
		vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > & nodeShardResults){
	for(unsigned i = 0 ; i < allPartitionResults.size(); ++i){
		IndexHealthInfo & pInfo = allPartitionResults.at(i).second;

		aggregatedResult.docCount += pInfo.docCount;
		aggregatedResult.writeCount += pInfo.writeCount;
		aggregatedResult.readCount += pInfo.readCount;
		aggregatedResult.isMergeRequired = aggregatedResult.isMergeRequired || pInfo.isMergeRequired;
		aggregatedResult.isBulkLoadDone = aggregatedResult.isBulkLoadDone && pInfo.isBulkLoadDone;
	}

	for(unsigned i = 0 ; i < nodeShardResults.size(); ++i){
		IndexHealthInfo & nodeShardInfo = nodeShardResults.at(i).second;

		aggregatedResult.docCount += nodeShardInfo.docCount;
		aggregatedResult.writeCount += nodeShardInfo.writeCount;
		aggregatedResult.readCount += nodeShardInfo.readCount;
		aggregatedResult.isMergeRequired = aggregatedResult.isMergeRequired || nodeShardInfo.isMergeRequired;
		aggregatedResult.isBulkLoadDone = aggregatedResult.isBulkLoadDone && nodeShardInfo.isBulkLoadDone;
	}

}


}
}

