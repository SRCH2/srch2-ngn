/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
