#ifndef __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__
#define __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"
#include "server/HTTPJsonResponse.h"

#include "../Transaction.h"
#include "../TransactionSession.h"
#include "./ShardCommnad.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"

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
class ShutdownCommand : public Transaction, public ConsumerInterface {
public:
	static void runShutdown(evhttp_request *req){
		ShutdownCommand * command = new ShutdownCommand(req);
		Transaction::startTransaction(command);
	}
public:

	ShutdownCommand(evhttp_request *req){
		this->saveOperation = NULL;
		this->req = req;
		initSession();
	}
	~ShutdownCommand(){
		if(saveOperation != NULL){
			delete saveOperation;
		}
	}

	ShardingTransactionType getTransactionType(){
		return ShardingTransactionType_Shutdown;
	}

	bool run();

	Transaction * getTransaction(){
		return this;
	}

	void initSession(){
		setSession(new TransactionSession());
		ShardManager::getShardManager()->getReadview(getSession()->clusterReadview);
		getSession()->response = new JsonResponseHandler();
	}

	void save();
	void consume(map<NodeId, vector<CommandStatusNotification::ShardStatus *> > & result) ;
	void clusterShutdown();
	string getName() const {return "shutdown-command";};

private:

	ShardCommand * saveOperation;
	SP(ShutdownNotification) shutdownNotif;
	evhttp_request *req;

};


}

}


#endif // __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__
