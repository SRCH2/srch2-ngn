#include "RoutingManager.h"
#include "server/MongodbAdapter.h"

using namespace srch2::httpwrapper;

namespace srch2 {
namespace httpwrapper {


RoutingManager::RoutingManager(ConfigManager&  cm, TransportManager& tm)  : 
    		configurationManager(cm),  tm(tm), dpInternal(&cm) {

	// create a server (core) for each data source in config file
	for(ConfigManager::CoreInfoMap_t::const_iterator iterator =
			cm.coreInfoIterateBegin(); iterator != cm.coreInfoIterateEnd();
			iterator++) {
		Srch2Server *core = new Srch2Server;
		core->setCoreName(iterator->second->getName());
		//   shardToIndex[iterator->second->coreId] = core;

		if(iterator->second->getDataSourceType() ==
				srch2::httpwrapper::DATA_SOURCE_MONGO_DB) {
			// set current time as cut off time for further updates
			// this is a temporary solution. TODO
			MongoDataSource::bulkLoadEndTime = time(NULL);
			//require srch2Server
			MongoDataSource::spawnUpdateListener(core);
		}

		//load the index from the data source
		try{
			core->init(&cm);
		} catch(exception& ex) {
			/*
			 *  We got some fatal error during server initialization. Print the error
			 *  message and exit the process.
			 *
			 *  Note: Other internal modules should make sure that no recoverable
			 *        exception reaches this point. All exceptions that reach here are
			 *        considered fatal and the server will stop.
			 */
			Logger::error(ex.what());
			exit(-1);
		}
	}
}

ConfigManager* RoutingManager::getConfigurationManager(){
	return &this->configurationManager;

}
DPInternalRequestHandler* RoutingManager::getDpInternal(){
	return &this->dpInternal;
}
std::map<ShardId, Srch2Server*> RoutingManager::getShardToIndexMap(){
	return this->shardToIndex;
}

//
///*
// *  Transmits a given message to all shards. The broadcast will not wait for
// *  confirmation from each receiving shard.
// */
//template<typename RequestType> void RoutingManager::broadcast(RequestType & , CoreShardInfo & ){
//
//}
///*
// *  Transmits a given message to all shards. The broadcast block until
// *  confirmation from each shard is received. Returns false iff any
// *  receiving shard confirms with MESSAGE_FAILED message.
// */
//template<typename RequestType> bool RoutingManager::broadcast_wait_for_all_confirmation(RequestType & requestObject,
//		bool& timedout, timeval timeoutValue , CoreShardInfo & coreInfo){
//
//}
///*
// *  Transmits a given message to all shards. Upon receipt of a response from
// *  any shard, the callback is trigger with the corresponding Message.
// *  The callback will be called for each shard.
// */
//template<typename RequestType , typename ReseponseType>
//void RoutingManager::broadcast_w_cb(RequestType& requestObj,
//		ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator, CoreShardInfo & coreInfo){
//
//}
///*
// *  Transmits a given message to all shards. The return messages for each
// *  shard are held until all shard’s return messages received. Then the
// *  callback is triggers with an array of message results from each shard.
// */
//template<typename RequestType , typename ReseponseType>
//void RoutingManager::broadcast_wait_for_all_w_cb(RequestType & requestObj,
//		ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator, CoreShardInfo & coreInfo){
//
//}
///*
// *  Timeout version of their corresponding function. So, after a period of
// *  set milliseconds the timeout callback function is called
// *
// *       *** Potentially could alert sync layer to timed out message
// *           from shard ***
// */
//template<typename RequestType , typename ReseponseType>
//void RoutingManager::broadcast_w_cb_n_timeout(RequestType& requestObj,ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator
//		, timeval timeoutValue , CoreShardInfo & coreInfo ){}
//
//
//template<typename RequestType , typename ReseponseType>
//void RoutingManager::broadcast_wait_for_all_w_cb_n_timeout(RequestType & requestObj,
//		ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator , timeval timeoutValue, CoreShardInfo & coreInfo){}
//
///*
// *  Transmits a given message to a particular shard in a non-blocking fashion
// */
//template<typename RequestType> void RoutingManager::route(RequestType& requestObj, ShardId & shardInfo){}
///*
// *  Transmits a given message to a pariticular shards, and waits for
// *  confirmation. Returns false iff shard confirms with MESSAGE_FAILED
// *  message.
// */
//template<typename RequestType> bool RoutingManager::route_wait_for_confirmation(RequestType& requestObj, bool& timedout,
//		timeval timeoutValue , ShardId shardInfo){}
///*
// *  Transmits a given message to a particular shards. Upon receipt of a
// *  response shard, the appropriate callback is trigger with the
// *  corresponding Message.
// */
//template<typename RequestType , typename ReseponseType>
//void RoutingManager::route_w_cb(RequestType& requestObj, ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator , ShardId shardInfo){}
///*
// *  Timeout version of their corresponding function. So, after a period of
// *  set milliseconds the timeout callback function is called
// *
// *       *** Potentially could alert sync layer to timed out message
// *           from shard ***
// */
//template<typename RequestType , typename ReseponseType>
//void RoutingManager::route_w_cb_n_timeout(RequestType & requestObj,ResultAggregatorAndPrint<RequestType , ReseponseType> * aggregator
//		, timeval timeoutValue, ShardId shardInfo){}

}
}
//save indexes in deconstructor
