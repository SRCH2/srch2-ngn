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
class DebugInfoCollector: public ReadviewTransaction, public ConsumerInterface {
public:

	static void collectInfo(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req){
        SP(DebugInfoCollector) debugInfoCollector =
        		SP(DebugInfoCollector)(new DebugInfoCollector(clusterReadview, req)); //
        Transaction::startTransaction(debugInfoCollector);
        return ;
	}

	~DebugInfoCollector(){
		finalize();
	}

private:

	DebugInfoCollector(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req):ReadviewTransaction(clusterReadview){
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
