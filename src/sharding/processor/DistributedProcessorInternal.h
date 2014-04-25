#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

class DPInternalRequestHandler {

public:

	// Public API which can be used by other modules

	/*
	 * 1. Receives a search request from a shard
	 * 2. Uses core to evaluate this search query
	 * 3. Sends the results to the shard which initiated this search query
	 */
	void internalSearchCommand(const RoutingManager::LogicalPlanMessage * msg,
			Srch2Server * server, RoutingManager::Message * responseMsg);


	/*
	 * This call back is always called for insert and update, it will use
	 * internalInsertCommand and internalUpdateCommand
	 */
	void internalInsertUpdateCommand();

	/*
	 * 1. Receives an insert request from a shard and makes sure this
	 *    shard is the correct reponsible of this record using Partitioner
	 * 2. Uses core execute this insert query
	 * 3. Sends the results to the shard which initiated this insert query (Failure or Success)
	 */
	void internalInsertCommand(RoutingManager::HTTPInsertRequestMessage * msg, unsigned sourceShard);
	/*
	 * 1. Receives a update request from a shard and makes sure this
	 *    shard is the correct reponsible of this record using Partitioner
	 * 2. Uses core execute this update query
	 * 3. Sends the results to the shard which initiated this update request (Failure or Success)
	 */
	void internalUpdateCommand(RoutingManager::HTTPUpdateRequestMessage * msg, unsigned sourceShard);

	/*
	 * 1. Receives a delete request from a shard and makes sure this
	 *    shard is the correct reponsible of this record using Partitioner
	 * 2. Uses core execute this delete query
	 * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
	 */
	void internalDeleteCommand(RoutingManager::HTTPDeleteRequestMessage * msg, unsigned sourceShard);



	/*
	 * 1. Receives a GetInfo request from a shard
	 * 2. Uses core to get info
	 * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
	 */
	void internalGetInfoCommand(RoutingManager::GetInfoMessage * msg, unsigned sourceShardID);


	/*
	 * This call back function is called for serialization. It uses internalSerializeIndexCommand
	 * and internalSerializeRecordsCommand for our two types of serialization.
	 */
	void internalSerializeCommand();

	/*
	 * 1. Receives a SerializeIndex request from a shard
	 * 2. Uses core to do the serialization
	 * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
	 */
	void internalSerializeIndexCommand(RoutingManager::SerializeIndexMessage * msg, unsigned sourceShardID);

	/*
	 * 1. Receives a SerializeRecords request from a shard
	 * 2. Uses core to do the serialization
	 * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
	 */
	void internalSerializeRecordsCommand(RoutingManager::SerializeRecordsMessage * msg, unsigned sourceShardID);

	/*
	 * 1. Receives a ResetLog request from a shard
	 * 2. Uses core to reset log
	 * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
	 */
	void internalResetLogCommand(RoutingManager::ResetLogMessage * msg, unsigned sourceShardID);


private:
	RoutingManager * routingManager;
	SynchronizationManager * synchronizationManager;
	ConfigManager * configurationManager;
};


}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
