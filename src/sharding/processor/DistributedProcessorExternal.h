#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_EXTERNAL_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_EXTERNAL_H_

#include <instantsearch/Record.h>
#include <instantsearch/LogicalPlan.h>

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

///////////////////////////////////////// TEMPORARY ////////////////////////////////////////////
typedef unsigned TimeoutValue;

class RoutingManager{
public:


	unsigned getNumberOfShards(CoreShardInfo * coreShardInfo);


	template<typename T>
	void broadcast_wait_for_all_w_cb_n_timeout(void * msg, T obj , TimeoutValue t, const CoreShardInfo * coreShardInfo);
	template<typename T>
	void broadcast_w_cb_n_timeout(Message * msg, T obj, TimeoutValue t);
	template<typename T>
	void route_w_cb_n_timeout(void & msg, T * obj, TimeoutValue t);
	Message connect_w_response_n_timeout(Message * msg, unsigned shardIndex, TimeoutValue timeout);
	void route ( Message * msg, unsigned  shardID);
};

class SynchronizationManager{

};

struct CoreShardInfo {
	unsigned shardId;
};

////////////////////////////////////////////////////////////////////////////////////////////////


class DPExternalRequestHandler {

public:

	DPExternalRequestHandler(ConfigManager * configurationManager, RoutingManager * routingManager, SynchronizationManager * synchronizationManager){
		this->configurationManager = configurationManager;
		this->routingManager = routingManager;
		this->synchronizationManager = synchronizationManager;
		partitioner = new Partitioner(routingManager,configurationManager);
	}

	// Public API which can be used by other modules

	/*
	 * 1. Receives a search request from a client (not from another shard)
	 * 2. broadcasts this request to DPInternalRequestHandler objects of other shards
	 * 3. Gives ResultAggregator functions as callback function to TransportationManager
	 * 4. ResultAggregator callback functions will aggregate the results and print them on
	 *    http channel
	 */
	void externalSearchCommand(evhttp_request *req , CoreShardInfo * coreShardInfo);

	/*
	 * 1. Receives an insert request from a client (not from another shard)
	 * 2. Uses Partitioner to know which shard should handle this request
	 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
	 *    Since it's a blocking call, the results are retrieved at the same point and
	 *    printed on the HTTP channel.
	 */
	void externalInsertCommand(evhttp_request *req);

	/*
	 * 1. Receives an update request from a client (not from another shard)
	 * 2. Uses Partitioner to know which shard should handle this request
	 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
	 *    Since it's a blocking call, the results are retrieved at the same point and
	 *    printed on the HTTP channel.
	 */
	void externalUpdateCommand(evhttp_request *req);

	/*
	 * 1. Receives an delete request from a client (not from another shard)
	 * 2. Uses Partitioner to know which shard should handle this request
	 * 3. sends this request to DPInternalRequestHandler objects of the chosen shard
	 *    Since it's a blocking call, the results are retrieved at the same point and
	 *    printed on the HTTP channel.
	 */
	void externalDeleteCommand(evhttp_request *req);

	/*
	 * 1. Receives a GetInfo request from a client (not from another shard)
	 * 2. Broadcasts this command to all shards and blocks to get their response
	 * 3. prints Success or Failure on HTTP channel
	 */
	void externalGetInfoCommand(evhttp_request *req, CoreShardInfo * coreShardInfo);

	/*
	 * 1. Receives a SerializeIndex request from a client (not from another shard)
	 * 2. Broadcasts this command to all shards and blocks to get their response
	 * 3. prints Success or Failure on HTTP channel
	 */
	void externalSerializeIndexCommand(evhttp_request *req, CoreShardInfo * coreShardInfo);

	/*
	 * 1. Receives a SerializeRecords request from a client (not from another shard)
	 * 2. Broadcasts this command to all shards and blocks to get their response
	 * 3. prints Success or Failure on HTTP channel
	 */
	void externalSerializeRecordsCommand(evhttp_request *req, CoreShardInfo * coreShardInfo);

	/*
	 * 1. Receives a ResetLog request from a client (not from another shard)
	 * 2. Broadcasts this command to all shards and blocks to get their response
	 * 3. prints Success or Failure on HTTP channel
	 */
	void externalResetLogCommand(evhttp_request *req, CoreShardInfo * coreShardInfo);

	/*
	 * Receives a commit request and boardcasts it to other shards
	 */
	void externalCommitCommand(evhttp_request *req, CoreShardInfo * coreShardInfo);


private:
	ConfigManager * configurationManager;
	RoutingManager * routingManager;
	SynchronizationManager * synchronizationManager;

	// now, use Partitioner to choose a shard for this record
	Partitioner * partitioner;

};


}
}







#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_EXTERNAL_H_
