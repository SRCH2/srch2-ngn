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
#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_EXTERNAL_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_EXTERNAL_H_

#include <instantsearch/Record.h>
#include <instantsearch/LogicalPlan.h>
#include "DistributedProcessorMessageHandler.h"
#include "sharding/sharding/metadata_manager/Shard.h"
#include "sharding/sharding/metadata_manager/Cluster.h"
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>
#include "operation/AttributeAccessControl.h"

#include <vector>
/*
 * This file contains four classes :
 *
 * class DPExternalRequestHandler
 * Handles the logic of distributing external requests like search and insert on other shards
 *
 * class DPInternalRequestHandler
 * Implements operations like search and insert internally. Calling functions of this class
 * is the result of DPExternalRequestHandler asking this shard to do an operation
 *
 * class ResultAggregator
 * Implements callback functions to aggregate different types of results
 * such as search results, facet results, getInfo results and etc.
 *
 * class Partitioner
 * Implements a the logic of partitioning records over shards
 *
 */

namespace srch2is = srch2::instantsearch;
using namespace std;
using srch2is::Record;
using srch2is::QueryResults;

namespace srch2 {
namespace httpwrapper {

class ConfigManager;

class DPExternalRequestHandler {

public:

	DPExternalRequestHandler(ConfigManager & configurationManager,
			TransportManager& transportManager, DPInternalRequestHandler& dpInternal);

	// Public API which can be used by other modules
	/*
	 * 1. Receives a getinfo request from a client (not from another shard)
	 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
	 * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
	 * 	  results. Results will be aggregator by another thread since it's not a blocking call.
	 */
	void externalGetInfoCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId, PortType_t portType = srch2http::InfoPort);

	/*
	 * 1. Receives a save request from a client (not from another shard)
	 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
	 * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
	 * 	  results. Results will be aggregator by another thread since it's not a blocking call.
	 */
	void externalSerializeIndexCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId);
	/*
	 * 1. Receives a export request from a client (not from another shard)
	 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
	 * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
	 * 	  results. Results will be aggregator by another thread since it's not a blocking call.
	 */
	void externalSerializeRecordsCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId);

	/*
	 * 1. Receives a reset log request from a client (not from another shard)
	 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
	 * 3. Gives ResultAggregator object to PendingRequest framework and it's used to aggregate the
	 * 	  results. Results will be aggregator by another thread since it's not a blocking call.
	 */
	void externalResetLogCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId);

	/*
	 * Receives a commit request and boardcasts it to other shards
	 */
	void externalCommitCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId);
	void externalMergeCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId);

private:
	ConfigManager * configurationManager;
	DPMessageHandler dpMessageHandler;
	AttributeAccessControl* attributeAcl;
};

}
}

#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_EXTERNAL_H_
