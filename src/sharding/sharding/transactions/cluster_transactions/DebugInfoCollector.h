#ifndef __SHARDING_SHARDING_DEBUG_INFO_COLLECTOR_H__
#define __SHARDING_SHARDING_DEBUG_INFO_COLLECTOR_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommandStatusNotification.h"
#include "../../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"
#include "ShardCommnad.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Saves the indices and the cluster metadata on all nodes in the cluster.
 * NOTE : this operation assumes all shards are locked in S mode
 * 1. request all nodes to save their indices
 * 2. When all nodes saved their indices, request all nodes to save their cluster metadata
 * 3. When all nodes acked metadata save, write the metadata on disk and done.
 */
class DebugInfoCollector: public WriteviewTransaction, public ConsumerInterface {
public:

	static void collectInfo(evhttp_request *req){
        SP(DebugInfoCollector) debugInfoCollector =
        		SP(DebugInfoCollector)(new DebugInfoCollector(req)); //
        Transaction::startTransaction(debugInfoCollector);
        return ;
	}

	~DebugInfoCollector(){
		finalize();
	}

private:

	DebugInfoCollector(evhttp_request *req){
		this->req = req;
	}

	void run(){
		// 1. add writeview info
		if(! ShardManager::getShardManager()->isCancelled() && ShardManager::getShardManager()->isJoined()){
			ShardManager::getShardManager()->print(this->getSession()->response);
		}else{
			this->getSession()->response->setResponseAttribute("error",  "Engine is not ready yet.");
		}
		this->getSession()->response->finalizeOK();
	}

    void finalizeWork(Transaction::Params * params){
		this->getSession()->response->printHTTP(req);
    }

    SP(Transaction) getTransaction() {
        return sharedPointer;
    }

    ShardingTransactionType getTransactionType(){
        return ShardingTransactionType_DebugCollectInfo; // returns the unique type identifier of this transaction
    }

    string getName() const {return "debug-info-collector" ;};
private:
    evhttp_request *req;
};


}
}

#endif // __SHARDING_SHARDING_DEBUG_INFO_COLLECTOR_H__
