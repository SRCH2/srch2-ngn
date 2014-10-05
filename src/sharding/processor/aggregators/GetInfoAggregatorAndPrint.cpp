#include "GetInfoAggregatorAndPrint.h"
#include "sharding/processor/PendingMessages.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


GetInfoResponseAggregator::GetInfoResponseAggregator(ConfigManager * configurationManager,
		boost::shared_ptr<HTTPJsonGetInfoResponse > brokerSideShardInfo,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, unsigned coreId):
		DistributedProcessorAggregator<GetInfoCommand,GetInfoCommandResults>(clusterReadview, coreId){
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
	Json::Value warinigNode = HTTPJsonResponse::getJsonSingleMessage(HTTP_JSON_Node_Timeout_Warning);
	warinigNode[c_node_name] = getClusterReadview()->getNode(message->getNodeId()).getName();
	this->brokerSideInformationJson->addWarning(warinigNode);
}

void GetInfoResponseAggregator::callBack(PendingMessage<GetInfoCommand, GetInfoCommandResults> * message){

    boost::unique_lock< boost::shared_mutex > lock(_access);

    const string nodeName = getClusterReadview()->getNode(message->getNodeId()).getName();

    vector<GetInfoCommandResults::ShardResults *> shardResults = message->getResponseObject()->getShardResults();
    for(unsigned shardIdx = 0 ; shardIdx < shardResults.size(); ++shardIdx){
    	GetInfoCommandResults::ShardResults * shardResult = shardResults.at(shardIdx);
    	this->shardResults.push_back(std::make_pair(nodeName, shardResult));
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
				vector<std::pair<string , IndexHealthInfo > > corePrimaryShardResults;
				vector<IndexHealthInfo> corePartitionResults;
				vector<ShardId *> corePartitionShardIds;
				vector<std::pair<string , IndexHealthInfo > > nodeShardResults;
				// NOTE : we ignore version id for core aggregation
				for(unsigned sid = 0 ; sid < shardResults.size(); ++sid){
					GetInfoCommandResults::ShardResults * shardResult = shardResults.at(sid).second;
					if(core->getCoreId() != shardResult->shardId->coreId){
						continue;
					}
					if(! shardResult->shardId->isClusterShard()){
						nodeShardResults.push_back(std::make_pair( shardResult->shardId->toString() , shardResult->healthInfo) );
						continue;
					}
					bool replicaExists = false;
					for(unsigned cpId = 0; cpId < corePartitionShardIds.size(); ++cpId){
						if(corePartitionShardIds.at(cpId)->isReplica(shardResult->shardId)){
							replicaExists = true;
							corePartitionResults.at(cpId).isMergeRequired =
									corePartitionResults.at(cpId).isMergeRequired ||
									shardResult->healthInfo.isMergeRequired;
							corePartitionResults.at(cpId).isBulkLoadDone =
									corePartitionResults.at(cpId).isBulkLoadDone &&
									shardResult->healthInfo.isBulkLoadDone;
							corePartitionResults.at(cpId).readCount += shardResult->healthInfo.readCount;
							break;
						}
					}
					if(! replicaExists){
						corePartitionResults.push_back(shardResult->healthInfo);
						corePartitionShardIds.push_back(shardResult->shardId);
						corePrimaryShardResults.push_back(std::make_pair(shardResult->shardId->toString(), shardResult->healthInfo));
					}
				}
				IndexHealthInfo aggregatedCoreInfo;
				aggregateCoreInfo(aggregatedCoreInfo, corePartitionResults, nodeShardResults);

				totalNumDocsInCluster += aggregatedCoreInfo.docCount;

				this->brokerSideInformationJson->addCoreInfo(core,aggregatedCoreInfo, corePrimaryShardResults, corePartitionResults, nodeShardResults);
			}

			this->brokerSideInformationJson->setResponseAttribute(c_cluster_total_number_of_documnets, Json::Value(totalNumDocsInCluster));
			Json::Value nodesJson(Json::objectValue);
			vector<Node> allNodes;
			getClusterReadview()->getAllNodes(allNodes);
			nodesJson[c_nodes_count] = Json::Value((unsigned)allNodes.size());
			this->brokerSideInformationJson->setResponseAttribute(c_nodes, nodesJson);
			break;

		}
		default:
			break;
	}

}

void GetInfoResponseAggregator::aggregateCoreInfo(IndexHealthInfo & aggregatedResult,
		vector<IndexHealthInfo> & allPartitionResults,
		vector<std::pair<string , IndexHealthInfo > > nodeShardResults){
	for(unsigned i = 0 ; i < allPartitionResults.size(); ++i){
		IndexHealthInfo & pInfo = allPartitionResults.at(i);

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

