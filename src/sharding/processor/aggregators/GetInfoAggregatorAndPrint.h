#ifndef __SHARDING_PROCESSOR_GET_INFO_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_GET_INFO_AGGREGATOR_AND_PRINT_H_

#include "sharding/processor/aggregators/DistributedProcessorAggregator.h"

#include "../serializables/SerializableGetInfoCommandInput.h"
#include "../serializables/SerializableGetInfoResults.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


class GetInfoResponseAggregator : public DistributedProcessorAggregator<GetInfoCommand,GetInfoCommandResults> {
public:
    GetInfoResponseAggregator(ConfigManager * configurationManager,
    		boost::shared_ptr<GetInfoJsonResponse > brokerSideShardInfo,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
    		unsigned coreId, bool debugRequest = false);

    ~GetInfoResponseAggregator();

    /*
     * This function is always called by RoutingManager as the first call back function
     */
    void preProcess(ResponseAggregatorMetadata metadata);
    /*
     * This function is called by RoutingManager if a timeout happens, The call to
     * this function must be between preProcessing(...) and callBack()
     */
    void processTimeout(PendingMessage<GetInfoCommand, GetInfoCommandResults> * message,
            ResponseAggregatorMetadata metadata);


    void callBack(PendingMessage<GetInfoCommand, GetInfoCommandResults> * message);

    /*
     * The main function responsible of aggregating status (success or failure) results
     */
    void callBack(vector<PendingMessage<GetInfoCommand, GetInfoCommandResults> * > messages);

    /*
     * The last call back function called by RoutingManager in all cases.
     * Example of call back call order for search :
     * 1. preProcessing()
     * 2. timeoutProcessing() [only if some shard times out]
     * 3. aggregateSearchResults()
     * 4. finalize()
     */
    void finalize(ResponseAggregatorMetadata metadata);



    void aggregateCoreInfo(IndexHealthInfo & aggregatedResult,
    		vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > & allPartitionResults,
    		vector<std::pair<GetInfoCommandResults::ShardResults * , IndexHealthInfo > > & nodeShardResults);

private:

    bool debugRequest;

    ConfigManager * configurationManager;
	boost::shared_ptr<GetInfoJsonResponse > brokerSideInformationJson ;
	GetInfoAggregateCriterion criterion;


    mutable boost::shared_mutex _access;

    vector<std::pair<string, GetInfoCommandResults::ShardResults * > > shardResults;

};

}
}

#endif // __SHARDING_PROCESSOR_GET_INFO_AGGREGATOR_AND_PRINT_H_
