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
#ifndef __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__
#define __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"
#include "server/HTTPJsonResponse.h"

#include "../Transaction.h"
#include "../../state_machine/ConsumerProducer.h"
#include "../TransactionSession.h"
#include "./ShardCommnad.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "wrapper/URLParser.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Safely shuts down the engine.
 * 1. Save the cluster data shards by disabling final release.
 * 2. Shut down the cluster.
 */
class ShutdownCommand : public WriteviewTransaction, public ConsumerInterface {
public:
	static void runShutdown(evhttp_request *req){
		SP(ShutdownCommand) command = SP(ShutdownCommand)(new ShutdownCommand(req));
		Transaction::startTransaction(command);
	}
	~ShutdownCommand(){
		finalize();
		for(unsigned i = 0 ; i < saveOperations.size(); ++i){
			delete saveOperations.at(i);
		}
	}

	ShutdownCommand(evhttp_request *req){
		this->req = req;
		this->force = false;
		this->shouldShutdown = true;
		this->shouldSave = true;
	}

public:

	ShardingTransactionType getTransactionType(){
		return ShardingTransactionType_Shutdown;
	}

	void run();

	SP(Transaction) getTransaction(){
		return sharedPointer;
	}

	void save();
	void consume(bool status, map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) ;
	string getName() const {return "cluster-shutdown-command";};

	void finalizeWork(Transaction::Params * arg);

	static void _shutdown();
	static void * _shutdownAnotherThread(void * args);
private:

	vector<ShardCommand *> saveOperations;
	SP(ShutdownNotification) shutdownNotif;
	evhttp_request *req;
	bool force ;
	bool shouldSave;
	bool shouldShutdown;
};


}

}


#endif // __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__
