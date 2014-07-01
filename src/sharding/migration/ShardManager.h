#ifndef __SHARDING_MIGRATION_SHARDMANAGER_H__
#define __SHARDING_MIGRATION_SHARDMANAGER_H__

#include "sharding/migration/CommandAggregator.h"
#include "sharding/configuration/ShardingConstants.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/processor/DistributedProcessorInternal.h"
#include "core/util/Assert.h"
#include <queue>
#include <map>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class RoutingManager;

class ShardManager{
public:
	ShardManager(ConfigManager * config, DPInternalRequestHandler * dpInternal, RoutingManager * rm);
	~ShardManager(){
		shardManagerGlobalLock.lock();
		shouldExecute = false;
		shardManagerGlobalLock.unlock();
	}

	// Responsibility of this class to delete request object, and response object if created.
	// If this object returns NULL, it is responsible of deleting the request object
	// If it returns a response object, everything will be deleting by RM after reply is sent and communication is
	// complete.
	SHMRequestReport * resolveMessage(Message* requestMessage, SHMRequestReport * request, NodeId node);
	// public helper functions
	bool isMasterNode();
	void initializeLocalShards();
private:




	// this method periodically executes and performs the tasks of ShardManager
	static void * periodicExecute(void * args);
	// executes the task of Master in one wakeup time
	void executeMaster();
	// executes the task of a client in one wakeup time
	void executeClient();


	//TODO : called only in the starting phase of this node,
	// when this function is finished a thread periodically executes periodicExecute();
	// there is also onetimeExecute(); which can be called by other modules to do the same job
	void bootstrapCluster();

	/*
	 * The first thing done in startup :
     *   this function loads the existing shard indices and updates clusterWrite view
	*  Phase: Load
	*	All nodes load any shard that they can find in their directory structure.
	*
	*   Example 1:
	*	/node1/core1/C1_P1/ *.idx
	*	/node1/core1/C1_P3/ *.idx
	*
	*	Example 2:
	*
	*	/node1/core1/C1_P1/ *.idx
	*	/node1/core1/C1_P2/ *.idx
	*
	*	And add the ShardIds of those shards to the CoreShardContainer object of their own node in cluster WriteView.
	*	The state of cluster WriteView after reading indices from file.
	*	Example 1:
	*	Node 1 : 2 shards (C1_P1 and C1_P3)
	*
	*	Example 2:
	*	Node 1 : 2 shards (C1_P1 and C1_P2)
	 */
	void bootstrapLoad();

	/*
	 * This function synchronizes the ShardManagers of all nodes in the cluster
	 * in the startup phase
	 * Master Execution :
	 *
	*	12. Master waits for LOADING_DONE message from all nodes.
	*	13. (receives R11 from all nodes) aggregates shard information coming from all nodes
	*	and updates its own cluster WriteView. Also, remember in some structure what shards are available.
	*	14. Send the cluster WriteView to all nodes.
	*	and wait for confirmation.
	*	15. (receives R13 from all nodes) COMMIT.
	*	Continue with Number 16 in a separate thread.
	*
	 *
	 * non-Master Execution :
	 *
	*   11. Sends master a LOADING_DONE confirmation which includes its own shard information.
	*   12. (receives L14) receives the master’s cluster WriteView which must be consistent with
	*   its own WriteView. Saves master’s WriteView in its own cluster.
	*   13. COMMIT the cluster WriteView. And send confirmation (COMMIT_DONE) message to the master.
	*   Continue listening to messages from master in a separate thread.
	 *
	 */
	void bootstrapSynchronizeMaster(); // works in master
	void bootstrapSynchronizeClient(); // works in clients



	/*
	 * In this functions master starts a new transaction and makes sure all
	 * other nodes' currentTransactionIds is up to our current transaction id
	 * NOTE: all transactions are started by Master
	 */
	void startTransaction(); // works in master
	void resolveStartTransactionCommand(unsigned newTransactionId, unsigned requestMessageId, NodeId requestNodeId); // works in clients

	/*
	 * This functions is used only to finalize a transaction to make sure
	 * all other nodes have successfully finished this transaction.
	 * In this last communication of a transaction, we commit the cluster writeview to be used
	 * from Readview.
	 */
	void finalizeTransaction(); // works in master
	void resolveCommitCommand(Cluster * masterClusterWriteview, unsigned requestMessageId, NodeId requestNodeId); // works in clients
	void resolveCompleteCommand(); // works in clients



	ConfigManager * configManager;
	DPInternalRequestHandler * dpInternal;
	RoutingManager * routingManager;


	boost::mutex shardManagerGlobalLock;

	bool isMaster;
	bool shouldExecute;


	//////////////////////////// Transaction Properties ///////////////////////////////
	// spin lock protects this value and the map
	boost::shared_mutex transactionsStatusLock;
	// the value of current ongoing transaction
	unsigned currentTransactionId;
	// map from TID to TStatus
	std::map<unsigned, TransactionStatus> transactionsStatus;
	// increments the current transaction id and returns the new value
	void setCurrentTransactionIdAndRegister(unsigned newTransationId);
	TransactionStatus getTransactionStatus(unsigned transactionId);
	unsigned getCurrentTransactionId();
	// if status is Completed, we actually remove it from the map
	void setCurrentTransactionStatus(TransactionStatus status);


	// Command buffer: resolveMessage stores every message which cannot be handled very fast in here
	CommandBuffer commandBuffer;

	// Results synchronizer
	CommandResultsSynchronizer responseSynchronizer;

	// helper functions
	void getListOfNodeIds(vector<NodeId> & nodeIds);
	string getListOfNodeIdsString(const vector<NodeId> & nodeIds);
	void broadcastStartTransactionCommand(vector<NodeId> & destinations, unsigned newTransactionId);
};


}
}
#endif // __SHARDING_MIGRATION_SHARDMANAGER_H__
