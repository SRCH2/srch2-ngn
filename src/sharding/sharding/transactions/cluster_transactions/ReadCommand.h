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
#ifndef __SHARDING_SHARDING_READ_COMMAND_H__
#define __SHARDING_SHARDING_READ_COMMAND_H__

#include "sharding/configuration/ShardingConstants.h"
#include "wrapper/WrapperConstants.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "include/instantsearch/Record.h"
#include "include/instantsearch/LogicalPlan.h"
#include "server/HTTPJsonResponse.h"
#include "wrapper/QueryParser.h"
#include "wrapper/QueryRewriter.h"
#include "wrapper/QueryValidator.h"
#include "read_command/ClusterPhysicalOperators.h"
#include "read_command/ClusterPhysicalPlan.h"
#include "read_command/AggregateSortOperator.h"
#include "wrapper/ParsedParameterContainer.h"
#include "../../state_machine/ConsumerProducer.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {
class AclAttributeReadCommand;
/*
 * contains the state of write operation per partition
 */
class ReadCommand : public ProducerInterface, public NodeIteratorListenerInterface{
public:
	ReadCommand(const CoreInfo_t * coreInfo,
			const ParsedParameterContainer & paramContainer,
			ConsumerInterface * consumer);

	~ReadCommand(){
	}

	SP(Transaction) getTransaction();


	void produce();


	void consume(bool status, const vector<unsigned> & searchableAttributeIds,
				const vector<unsigned> & refiningAttributeIds,
				const vector<JsonMessageCode> & messages);


	void search();

	void end(map<NodeId, SP(ShardingNotification) > & replies);


	void processSearchResults(map<NodeId, SP(ShardingNotification) > & replies);


	void finalize(bool status = false);

	string getName() const {return "read-command";};

	unsigned getTotalSearchTime() const;
	unsigned getPayloadAccessTime() const;

private:

	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	const CoreInfo_t * coreInfo;
	vector<JsonMessageCode> messageCodes;
	vector<string> customMessageStrings;

	AclAttributeReadCommand * aclAttributeReadCommnad;

	bool prepareAttributeAclInfo();

	LogicalPlan * prepareLogicalPlan();

	// query related members
	ParsedParameterContainer paramContainer;
	srch2::instantsearch::LogicalPlan * logicalPlan;
	vector<unsigned> aclApprovedSearchAttributes;
	vector<unsigned> aclApprovedRefiningAttributes;

	// response related members
    vector<NodeTargetShardInfo> targets;
	unsigned validationRewriteTime;
	unsigned totalSearchTime;
	unsigned payloadAccessTime;
	ClusterSortOperator * sortOperator;
	ClusterFacetResultsAggregator facetAggregator;
	map<NodeId, SP(ShardingNotification) > results; // only protector of shared pointers, results actually go to physical pla
	unsigned aggregatedEstimatedNumberOfResults;
    bool isResultsApproximated;
    unsigned aggregatedSearcherTime;
	struct timespec tstart;
    SP(Json::Value) root;


	void preparePhysicalPlan(vector<QueryResults * > & networkOperators);

	// print results on HTTP channel
	void executeAndPrint();


	/**
	 * Iterate over the recordIDs in queryResults and get the record.
	 * Add the record information to the request.out string.
	 */
	boost::shared_ptr<Json::Value> printResults(const LogicalPlan &queryPlan,
	        const CoreInfo_t *indexDataConfig,
	        const vector<QueryResult * > allResults,
	        const Query *query,
	        const unsigned start, const unsigned end,
	        const unsigned retrievedResults,
	        const unsigned ts1 , const vector<RecordSnippet>& recordSnippets, unsigned hlTime, bool onlyFacets) ;


	/**
	 * Iterate over the recordIDs in queryResults and get the record.
	 * Add the record information to the request.out string.
	 */
	boost::shared_ptr<Json::Value> printOneResultRetrievedById(const LogicalPlan &queryPlan,
	        const CoreInfo_t *indexDataConfig,
	        const vector<QueryResult *> allResults,
	        const unsigned ts1);



	void genRecordJsonString(const srch2::instantsearch::Schema * schema, StoredRecordBuffer buffer,
	        const string& extrnalRecordId, string& sbuffer);
	void genRecordJsonString(const srch2::instantsearch::Schema * schema, StoredRecordBuffer buffer,
	        const string& externalRecordId, string& sbuffer, const vector<string>* attrToReturn);

	/*
	 *   This functions removes new line and non-printable characters from the input string
	 *   and returns a clean string.
	 */
	void cleanAndAppendToBuffer(const string& in, string& out) ;

	void genSnippetJSONString(const RecordSnippet& recordSnippet, string& sbuffer) ;


};


}
}


#endif // __SHARDING_SHARDING_READ_COMMAND_H__
