#ifndef __SHARDING_ROUTING_ROUTING_MANAGER_H__
#define __SHARDING_ROUTING_ROUTING_MANAGER_H__

#include<sys/time.h>
#include <map>

#include <sharding/configuration/ConfigManager.h>
#include <sharding/transport/TransportManager.h>
#include <sharding/processor/DistributedProcessorInternal.h>

#include <server/Srch2Server.h>
#include <sharding/processor/ResultsAggregatorAndPrint.h>
#include "Multiplexer.h"
#include "RMCallback.h"
#include "transport/MessageAllocator.h"
#include "InternalMessageBroker.h"

using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

/*
 * TODO: this struct must be replaced with something consistent with ConfigurationManager global structures ...
 */

/* All Objects sent and received from the RoutingManager must have the 
 * following functions calls:
 *
 *    //serializes the object to a byte array and places array into the region
 *    //allocated by given allocator
 *    void* serialize(std::allocator<char>);
 *
 *    //given a byte stream recreate the original object
 *    const Object& deserialize(void*);
 *
 *
 *    //Returns the type of message which uses this kind of object as transport
 *    MessageType messsageKind();
 */

/* All Callback Object used for recieving Messages must have the appropriate
 * callback corresponding with their message Type:
 *
 *   All must have:
 *      onFail();
 *
 *   QueryType must have:
 *      void search(ResultType&); //for single call back
 *      void search(MessageIterator<ResultType&>); //for wait all call back
 *   Any other type:
 *      void receive(ResultType&); //for single call back
 *      void receive(MessageIterator<ResultType&>); //for wait all call back
 *
 *   if Callback has a timeout the following method must be implemented:
 *      timeout();
 */


class RoutingManager {

public:

	RoutingManager(ConfigManager&  configurationManager, TransportManager& tm);



	/*
	 *  Transmits a given message to all shards. The broadcast will not wait for
	 *  confirmation from each receiving shard.
	 */
	template<typename RequestType> void broadcast(RequestType &,
			CoreShardInfo &);


	/*
	 *  Transmits a given message to all shards. The broadcast block until
	 *  confirmation from each shard is received. Returns false iff any
	 *  receiving shard confirms with MESSAGE_FAILED message.
	 */
	template<typename RequestType> bool broadcast_wait_for_all_confirmation(RequestType & requestObject,
			bool& timedout, timeval timeoutValue , CoreShardInfo & coreInfo);

	/*
	 *  Transmits a given message to all shards. Upon receipt of a response from
	 *  any shard, the callback is trigger with the corresponding Message.
	 *  The callback will be called for each shard.
	 */
	template<typename RequestType , typename ReseponseType>
	void broadcast_w_cb(RequestType& requestObj, ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator, CoreShardInfo & coreInfo);

	/*
	 *  Transmits a given message to all shards. The return messages for each
	 *  shard are held until all shard’s return messages received. Then the
	 *  callback is triggers with an array of message results from each shard.
	 */
	template<typename RequestType , typename ReseponseType>
	void broadcast_wait_for_all_w_cb(RequestType & requestObj,
			ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator, CoreShardInfo & coreInfo);


	/*
	 *  Timeout version of their corresponding function. So, after a period of
	 *  set milliseconds the timeout callback function is called
	 *
	 *       *** Potentially could alert sync layer to timed out message
	 *           from shard ***
	 */
	template<typename RequestType , typename ReseponseType>
	void broadcast_w_cb_n_timeout(RequestType& requestObj,ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator
			, timeval timeoutValue , CoreShardInfo & coreInfo );
	template<typename RequestType , typename ReseponseType>
	void broadcast_wait_for_all_w_cb_n_timeout(RequestType & requestObj,
			ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator , timeval timeoutValue, CoreShardInfo & coreInfo);


	/*
	 *  Transmits a given message to a particular shard in a non-blocking fashion
	 */
	template<typename RequestType> void route(RequestType& requestObj, ShardId & shardInfo);

	/*
	 *  Transmits a given message to a pariticular shards, and waits for
	 *  confirmation. Returns false iff shard confirms with MESSAGE_FAILED
	 *  message.
	 */
	template<typename RequestType> bool route_wait_for_confirmation(RequestType& requestObj, bool& timedout,
			timeval timeoutValue , ShardId shardInfo);

	/*
	 *  Transmits a given message to a particular shards. Upon receipt of a
	 *  response shard, the appropriate callback is trigger with the
	 *  corresponding Message.
	 */
	template<typename RequestType , typename ReseponseType>
	void route_w_cb(RequestType& requestObj, ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator , ShardId shardInfo);

	/*
	 *  Timeout version of their corresponding function. So, after a period of
	 *  set milliseconds the timeout callback function is called
	 *
	 *       *** Potentially could alert sync layer to timed out message
	 *           from shard ***
	 */
	template<typename RequestType , typename ReseponseType>
	void route_w_cb_n_timeout(RequestType & requestObj,ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator
			, timeval timeoutValue, ShardId shardInfo);


	MessageAllocator * getMessageAllocator() ;
	ConfigManager* getConfigurationManager();
	DPInternalRequestHandler* getDpInternal();
	InternalMessageBroker * getInternalMessageBroker();

	Srch2Server * getShardIndex(ShardId shardId){
		return &shardServers[shardId.coreId];
	}

private:
  void sendInternalMessage(ShardingMessageType, ShardId, void*);
	//std::map<ShardId, Srch2Server*> shardToIndex;
	ConfigManager& configurationManager;
	TransportManager& transportManager;
	DPInternalRequestHandler dpInternal;
	InternalMessageBroker internalMessageBroker;
	Srch2Server *shardServers;
};

}
}

#include "BroadcastInlines.h"

#endif //__SHARDING_ROUTING_ROUTING_MANAGER_H__
