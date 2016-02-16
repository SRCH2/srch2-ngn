#ifndef __SHARDING_SHARDING_SHARDMANAGER_H__
#define __SHARDING_SHARDING_SHARDMANAGER_H__

#include "core/util/Assert.h"


#include "./notifications/LockingNotification.h"
#include "../transport/Message.h"
#include "../transport/TransportManager.h"
#include "../transport/CallbackHandler.h"
#include "../configuration/ConfigManager.h"
#include "../configuration/ShardingConstants.h"
#include "server/HTTPJsonResponse.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include "boost/shared_ptr.hpp"

/*
 * When transactions use MigrationManager, they have to wait
 * until it does its task and gets back to them by calling the consume
 * method of the consumer which used MM. The list of these waiting consumers
 * is maintained in mmSessionListenersGroup which is a list of maps. Each map
 * contains the reference to a "group" of transactions, and when a reference in one
 * map is going to be used, the map has to be locked. We divide transactions into groups
 * to reduce the chance of concurrent ones fall into the same map, so we have better
 * concurrency.
 * The following constant is the number of groups of transactions, i.e. number of such maps.
 * therefore, groupId = transactionId % MAX_NUM_TRANS_GROUPS
 */
#define MAX_NUM_TRANS_GROUPS 1000

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ClusterResourceMetadata_Readview;
class Cluster_Writeview;
class ClusterNodes_Writeview;
class StateMachine;
class LockManager;
class ResourceMetadataManager;
class MigrationManager;
struct ShardMigrationStatus;
class ProducerInterface;
class DPInternalRequestHandler;
class ConsumerInterface;

/*
 * This class is basically the well-known Distributed Processor.
 * TODO: later, after finishing the project, we may even want to change its name to
 * DistributedProcessor
 *
 * All data related logic is implemented in this class and classes that it uses.
 * Examples are distributed locking, state-full operations, transaction control, RESTful API, and etc.
 */
class ShardManager : public CallBackHandler{
public:

	/*
	 * ShardManager is a singleton object, use this method to create its object.
	 * The single object of ShardManager is created and maintained in Srch2ServerRuntime
	 */
	static ShardManager * createShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager);

	/*
	 * In the time of shutdown, we need to do a house-cleaning process which includes
	 * things like waiting for ongoing transactions to finish.
	 * This logic is encapsulated in this method.
	 */
	static void deleteShardManager();

	// getter method for singleton ShardManager method.
	static ShardManager * getShardManager();

	/*
	 * Returns the ID of the current node.
	 * Note : currentNodeId member is initialized in the startup process when discovery manager
	 * finds out whether this is the first node or what Id is assigned to this node...
	 */
	static NodeId getCurrentNodeId();

	/*
	 * Main access point to metadata writeview.
	 * Getter method for easier access to metadata writeview.
	 * Inside this call, we use xLock object -- which comes from where we make this call from --
	 * to X-lock the mutex which protects metadata writeview. When this passing object gets destroyed,
	 * the mutex lock will be freed.
	 */
	static Cluster_Writeview * getWriteview_write(boost::unique_lock<boost::shared_mutex> & xLock);

	/*
	 * TODO: the existence of this method is a sign of bad design. The reason is that there
	 * places that writeview is already X-locked in outer scopes and we shouldn't lock it anymore.
	 * Example : DataShardInitializer
	 * Note: Using 'recursive mutexes' solves this problem (which is getting into deadlock with self)
	 * but using recursive mutex is mostly avoided because it's an expensive kind of mutex (in fact
	 * if you have heard "using mutex is expensive" that's actually about recursive mutexes and not
	 * about all types of mutexes).
	 */
	static Cluster_Writeview * getWriteview_nolock();

	/*
	 * Grabs S lock on metadata writeview and returns a constant pointer to it.
	 */
	static const Cluster_Writeview * getWriteview_read(boost::shared_lock<boost::shared_mutex> & sLock);

	/*
	 * The class ClusterNodes_Writeview is a wrapper around only that part of metadata which is related to
	 * nodes (nodes information and currentNodeId). The following is why we returns shared pointer instead of
	 * normal pointers :
	 * Inside an object of type ClusterNodes_Writeview, we have a lock object which is used to lock
	 * the mutex which protects node information (this mutex is different than general writeview mutex because of
	 * deadlock issues, and also because node information is used must more frequently). when this object is destroyed,
	 * that lock object is also destroyed and mutex is automatically freed. So passing the pointer in a shared pointer
	 * gives the guarantee that this object will be deleted when we come out of the caller's scope. also, caller
	 * doesn't have to unlock any locks explicitly (and if it wants, it should just reset the shared pointer
	 * and the let the ClusterNodes_Writeview object destroy)
	 */
	static SP(ClusterNodes_Writeview) getNodesWriteview_write();

	/*
	 * Same as getNodesWriteview_write but for read. Acquires S lock on nodes info mutex of
	 * cluster metadata.
	 */
	static SP(const ClusterNodes_Writeview) getNodesWriteview_read();

	/*
	 * Main access point to metadata readview.
	 * Assigns the value of current cluster metadata readview shared pointer to 'readview'
	 */
	static void getReadview(boost::shared_ptr<const ClusterResourceMetadata_Readview> & readview);

	/*
	 * State machine getter method.
	 */
	static StateMachine * getStateMachine();

	/*
	 * Note: this method is only used for special caes:
	 * 1. In the time of delete, shard manager must lock to stop other new transactions to come in
	 * 2. A transaction puts S lock on singleInstanceMutex of shardManager in its lifetime, so deleteShardManager
	 * which needs X lock (to delete the shard manager entirely) has to block and shutdown gets blocked until
	 * all transactions leave.
	 */
	static boost::shared_mutex & getShardManagerGuard();


	/*
	 * Class Entry point #1 --
	 * Initializes the ShardManager [singleton] object
	 */
	ShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager);
	~ShardManager();


	/*
	 * Class Entry point #2 --
	 * When discovery manager initializes node information
	 * start() will begin the work of ShardManager which
	 * leads to the state that isJoined() eventually returns
	 * true after which ShardManager works normally.
	 *
	 * Locking Note: through this method, S lock on singleInstanceLock mutex is acquired.
	 */
	void start();

	/*
	 * Class Entry point #3 --
	 * implements the virtual method of CallbackHandler
	 * Thread started for an internal communication message by TM.
	 *
	 * A message which reaches here contains a ShardingNotification; this method takes
	 * care of msg (received from node) by calling the pure virtual resolveNotification method
	 * of the coming notification.
	 *
	 * Locking Note: through this method, S lock on singleInstanceLock mutex is acquired.
	 */
	bool resolveMessage(Message * msg, NodeId node);

	/*
	 * Entry point #4 --
	 * ** Function of a new thread.
	 * called from destructor of readview (ClusterResourceMetadata_Readview) to notify
	 * ShardManager that the readview is released and
	 * no more readers are using it.
	 *
	 * Passes this call to lock manager to unblock readview-release-pending lock requests.
	 *
	 * Locking Note: through this method, S lock on singleInstanceLock mutex is acquired.
	 */
	static void * resolveReadviewRelease(void * metadataVersion);

	/*
	 * Entry point #5 --
	 * called from migration manager to inform ShardManager about the status of a migration
	 *
	 * Passes this call to the waiting transaction. (the customer of the corresponding MM task)
	 *
	 * Locking Note: through this method, S lock on singleInstanceLock mutex is acquired.
	 */
	void resolveMMNotification(const ShardMigrationStatus & migrationStatus);


	/*
	 * NOT an entry point.
	 * Any code that is going to give a task to MigrationManager, must first register itself (or any
	 * module which is going to process the outcome) in the ShardManager for the time that an update
	 * on this task reaches ShardManager.
	 * operationId is the identifier of the customer operation.
	 * listener is the module which will process the outcome.
	 */
	void registerMMSessionListener(const unsigned operationId, ConsumerInterface * listener);


	/*
	 * Entry point #6 --
	 * called from SM to inform us about the arrival
	 *
	 * Only adds partial information of the arriving node to the nodes info part of
	 * metadata. the newNode will actually join the cluster later by a NodeJoiner transaction.
	 *
	 * Locking Note: through this method, S lock on singleInstanceLock mutex is acquired.
	 */
	void resolveSMNodeArrival(const Node & newNode);

	/*
	 * Entry point #7 --
	 * called from SM to inform us about failure of a node
	 *
	 * Passes failedNodeId, the id of the failed node, to metadataManager,
	 * lockManager, and the stateMachine to inform all modules of this event.
	 *
	 * Locking Note: through this method, S lock on singleInstanceLock mutex is acquired.
	 */
	void resolveSMNodeFailure(const NodeId failedNodeId);

	/*
	 * !!!NOT AN ENTRY POINT!!!
	 * No lock should be acquired in this method, this method is only called to pass the execution
	 * to another thread for local notification resolving which starts in _resolveLocal(void * request)
	 */
	bool resolveLocal(SP(ShardingNotification) request);
	// Structure used to pass arguments to the thread which does local processing.
	struct ResolveLocalArgs{
		ResolveLocalArgs(SP(ShardingNotification) notif){
			this->notif = notif;
		}
		SP(ShardingNotification) notif;
	};
private:
	/*
	 * Entry point #8 --
	 * S in single instance mutex
	 * NOTE : must be similar to resolveMessage because it's
	 * just like receiving a notification from the same node
	 */
	static 	void * _resolveLocal(void * request);

	/*
	 * Clears the list of maps of waiting transactions for MM
	 *
	 * Locking Note: When this method is called, singleInstanceLock mutex is
	 * already locked properly.
	 */
	void clearMMRegistrations();

	/*
	 * NOT an entry point.
	 * It's called from periodicWork periodically, and upon this call, every module
	 * does a light weight consistency check, to make sure the mechanisms are not stuck.
	 *
	 * The reason if having this mechanism to recover problems (if they exist) and keep
	 * things moving. we can never be sure this system is bug-free.
	 */
	void resolveTimeoutNotification();

public:

	// ShardManager getter/setter methods.
	TransportManager * getTransportManager() const;
	ConfigManager * getConfigManager() const;
	ResourceMetadataManager * getMetadataManager() const;
	LockManager * _getLockManager() const;
	StateMachine * _getStateMachine() const;
	MigrationManager * getMigrationManager() const;
	pthread_t * getLoadbalancingThread() ;

	// Methods used to initialize the sub-modules or connect other modules to ShardManager.
	void setDPInternal(DPInternalRequestHandler * dpInternal);
	DPInternalRequestHandler * getDPInternal() const;
	void initMigrationManager();
	void attachToTransportManager(TransportManager * tm);

	/*
	 * Initializes this node to be the first node in the cluster.
	 * shouldLock :
	 * if true: metadata writeview is not protected and this node must lock it if it needs to use it.
	 * if false: the caller of this method has made the metadata secure so this method must
	 * not lock it again (or it goes into a deadlock)
	 */
    void initFirstNode(const bool shouldLock = true);

    /*
     * Update the value of currentNodeId member of ShardManager
     * if writeviewLocked is not NULL, it's a protected writeview object from which the
     * currentNodeId must be read
     * if writeviewLocked is NULL, the main metadata writeview object is locked and used to
     * read the value of current node's Id.
     */
	void updateCurrentNodeId(Cluster_Writeview * writeviewLocked = NULL);

	/*
	 * Sets the joinedFlag which indicates the process of joining to the cluster is finished.
	 */
	void setJoined();

	/*
	 * Returns true if the process of joining the cluster (or initializing a new one) is finished.
	 * returns false if we are still in that process.
	 */
	bool isJoined();

	/*
	 * Sets the cancelledFlag to true which stops ShardManager and avoids new things to be started.
	 * It's used in shutdown process.
	 */
	void setCancelled();

	/*
	 * Returns true if shutdown is in process and ShardManager tasks must stop.
	 */
	bool isCancelled() ;

	/*
	 * Sets the loadBalancingFlag which indicates a load balancing transaction is in progress.
	 * the usage is that we want to have no more than one load-balancing process at one time.
	 */
	void setLoadBalancing();

	/*
	 * Resets the loadBalancingFlag to let future possible load balancing task start.
	 * input is the results of last attempt which helps us know how much more time we should try
	 * to do load balancing.
	 */
	void resetLoadBalancing(bool timeChange);

	/*
	 * Returns true if there is a load balancing transaction in progress.
	 */
	bool isLoadBalancing();

	/*
	 * Returns the value of loadBalancingCheckInterval which is the number of micro-seconds
	 * we should wait before the next load-balancing attempt
	 */
	uint32_t getLoadBalancingCheckInterval();

	/*
	 * Prints the content of ShardManager (and all sub-modules)
	 * to the JsonResponseHandler, response, or to standard output if it is NULL
	 */
	void print(JsonResponseHandler * response = NULL);
private:


	/*
	 * This mutex is used to make sure main pointer of ShardManager is not deleted until all requests leave.
	 *
	 * IMPORTANT : So we have S lock on this mutex in the beginning of all entry points and also in the beginning and end of
	 * all transactions;
	 */
    static boost::shared_mutex singleInstanceLock;
	static ShardManager * singleInstance;


	TransportManager * transportManager;
	ConfigManager * configManager;
	DPInternalRequestHandler * dpInternal;
	MigrationManager *migrationManager;
	LockManager * _lockManager;
    ResourceMetadataManager * metadataManager;
	StateMachine * stateMachine;

	/*
	 * mutex to protect members of shard manager that are accessed by multiple threads
	 * these members are :
	 * NodeId currentNodeId
	 * bool joinedFlag;
	 * bool cancelledFlag;
	 * bool loadBalancingFlag;
	 * uint32_t loadBalancingCheckInterval;
	 * vector<SP(ShardingNotification)> bouncedNotifications
	 * map<NodeId, unsigned> failedNodesHandledByTimeout
	 * vector< std::pair< map<unsigned , ConsumerInterface *>, boost::mutex * > > mmSessionListenersGroup
	 * vector< map<unsigned , SP(Transaction) > >mmSessionListenersGroup_TransSharedPointers
	 */
    boost::shared_mutex shardManagerMembersMutex;

    // updated in startup process, always contains the node ID of this node.
	NodeId currentNodeId;

	// structure used with periodic timeout notifications to make sure failedNodes
	// are actually recognized as failed nodes in the system. More explanation in
	// resolveTimeoutNotification
    map<NodeId, unsigned> failedNodesHandledByTimeout;

    // list of maps. Each map is a group of transactions waiting for MM
    // Two transactions have to compete for the " boost::mutex *" in these pairs only if they belong to the
    // same group.
	// map from operatioId to MM listener
	vector< std::pair< map<unsigned , ConsumerInterface *>, boost::mutex * > > mmSessionListenersGroup;

	// In addition to ConsumerInterface pointers, we must also maintain the shared pointer
	// to the transaction owning that consumer because transactions destroy when there is no reference to them.
	vector< map<unsigned , SP(Transaction) > >mmSessionListenersGroup_TransSharedPointers;
	bool joinedFlag;
	bool cancelledFlag;
	bool loadBalancingFlag;
	uint32_t loadBalancingCheckInterval;

	/*
	 * If a notification reaches a node before it's completely joined the cluster,
	 * it bounces the notification to the sender, and the sender sends that again after
	 * some time. This structure is the list of all outgoing notifications of this node
	 * that have bounced back.
	 */
    vector<SP(ShardingNotification)> bouncedNotifications;

    // A bounced notification has returned, save it to send it again later.
	void saveBouncedNotification(SP(ShardingNotification) notif);

	// Resent all saved bounced notifications to their original destinations.
	void resendBouncedNotifications();

	// Bounce the notification to its sender.
	void bounceNotification(SP(ShardingNotification) notif);

	// Print the information of bonding notifications.
	void printBouncedNotifications(JsonResponseHandler * response = NULL);

	// pointer to thread which periodically performs some tasks
	pthread_t * loadBalancingThread;

	// Used in shutdown, to stop the work
	void cancelAllThreads(bool shouldLock = true);

	/*
	 * Periodically runs a piece of code on shard manager mainly for load balancing.
	 * S lock on singleInstanceLock must be in main scope and released when isCancelled() returns true
	 *
	 */
	static void * periodicWork(void *args) ;


public:
	/*
	 * Entry point #9 --
	 *
	 * Used for GetInfo RESTful request.
	 *
	 * S in single instance mutex
	 */
	void getNodeInfoJson(Json::Value & nodeInfo);


};


}
}

#endif // __SHARDING_SHARDING_SHARDMANAGER_H__
