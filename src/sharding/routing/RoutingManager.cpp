#include "RoutingManager.h"
#include "server/MongodbAdapter.h"

using namespace srch2::httpwrapper;

namespace srch2 {
namespace httpwrapper {


RoutingManager::RoutingManager(ConfigManager&  cm, DPInternalRequestHandler& dpInternal, TransportManager& transportManager)  :
                                    configurationManager(cm),
                                    dpInternal(dpInternal),
                                    transportManager(transportManager),
                                    internalMessageBroker(*this, dpInternal) {

    // share the internal message broker from RM to TM
    transportManager.setInternalMessageBroker(&internalMessageBroker);
    transportManager.setRoutingManager(this);

    this->pendingRequestsHandler = new PendingRequestsHandler(transportManager.getMessageAllocator());
}

ConfigManager* RoutingManager::getConfigurationManager() {
    return &this->configurationManager;

}

DPInternalRequestHandler* RoutingManager::getDpInternal() {
    return &this->dpInternal;
}

InternalMessageBroker * RoutingManager::getInternalMessageBroker(){
    return &this->internalMessageBroker;
}

PendingRequestsHandler * RoutingManager::getPendingRequestsHandler(){
    return this->pendingRequestsHandler;
}

TransportManager& RoutingManager::getTransportManager(){
    return transportManager;
}

Srch2ServerHandle RoutingManager::getSrch2ServerIndex(ShardId shardId){
	// access config manager cluster and get the shardHandle for this shardId
	//1. get cluster from config manager
	Cluster * cluster = configurationManager.getCluster();
	//2. TODO get S lock on the shardMap
	//....
	//3. read the shardMap
	std::map<ShardId, Shard, ShardIdComparator> * shardMap = NULL; // TODO
	//4. find the entry which is corresponding to this shardId
	std::map<ShardId, Shard, ShardIdComparator>::iterator shardEntryItr =
			shardMap->find(shardId);
	if(shardEntryItr == shardMap->end()){
		// shard not found in the map, return error
		return 0; // not found
	}
	if(shardEntryItr->second.getShardState() != SHARDSTATE_ALLOCATED){
		// error, shard not in operational state
		return -2;
	}
	ShardState state = shardEntryItr->second.getShardState();
	//5. TODO release the S lock
	// ....
	return state;
}

MessageAllocator * RoutingManager::getMessageAllocator() {
    return transportManager.getMessageAllocator();
}

void * RoutingManager::routeInternalMessage(void * arg) {
    std::pair<RoutingManager * , std::pair<Message *, NodeId> >  * rmAndMsgPointers =
            (std::pair<RoutingManager * , std::pair<Message * , NodeId> >  *)arg;

    RoutingManager * rm = rmAndMsgPointers->first;
    Message * msg = rmAndMsgPointers->second.first;
    NodeId nodeId = rmAndMsgPointers->second.second;

    ASSERT(msg->isInternal());
    rm->getInternalMessageBroker()->resolveMessage(msg, nodeId);
    // what if resolve returns NULL for something?
    delete rmAndMsgPointers;
    return NULL;
}

} }
//save indexes in deconstructor
