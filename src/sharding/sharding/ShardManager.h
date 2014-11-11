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
 * This class is the main responsible class to do operations on shards of data in the cluster.
 * For example, Load balancing upon arrival of new nodes or failure of existing nodes is handled in this class.
 */
class ShardManager : public CallBackHandler{
public:

	static ShardManager * createShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager);
	static void deleteShardManager();
	static ShardManager * getShardManager();

	/*
	 * These functions are used in different tasks done in shard manager
	 * no lock should be acquired in these methods
	 */
	static NodeId getCurrentNodeId();
	static Cluster_Writeview * getWriteview_write(boost::unique_lock<boost::shared_mutex> & xLock);
	static const Cluster_Writeview * getWriteview_read(boost::shared_lock<boost::shared_mutex> & sLock);
	static SP(ClusterNodes_Writeview) getNodesWriteview_write();
	static SP(const ClusterNodes_Writeview) getNodesWriteview_read();
	static void getReadview(boost::shared_ptr<const ClusterResourceMetadata_Readview> & readview);
	static StateMachine * getStateMachine();
	static boost::shared_mutex & getShardManagerGuard();


	/*
	 * Entry point #0 --
	 * Construction. The very first module in the system. In the beginning of main.
	 */
	ShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager);
	~ShardManager();


	/*
	 * Entry point #1 --
	 * When discovery manager initializes node information
	 * start will start the work of shard manager and after isJoined()
	 * returns true it works normally.
	 * This function must have it's own shardManagerPtr protector and
	 * any threads and operations starting from this function must keep s lock on the pointer.
	 */
	void start();

	/*
	 * Entry point #2 --
	 * When this node is going to shutdown, either because of a nodeshutdown or a clustershutdown
	 * NO LOCK should be acquired here
	 */
	void _shutdown();

	/*
	 * Entry point #3 --
	 * Thread started for one of the internal communication messages.
	 * S on singleInstanceMutex
	 */
	bool resolveMessage(Message * msg, NodeId node);

	/*
	 * Entry point #4 --
	 * called from destructor of readview to notify shard manager that the readview is released and
	 * no more readers are using it
	 * S on singleInstanceMutex
	 */
	//
	static void * resolveReadviewRelease(void * metadataVersion);

	/*
	 * Entry point #5 --
	 * called from migration manager to inform us about the status of a migration
	 * S on singleInstanceMutex
	 * call preProcess and postProcess if transaction not going to be deleted;
	 * also lock to shardManagerMembersMutex must be acquired because we work with mmSessionListeners
	 */
	void resolveMMNotification(const ShardMigrationStatus & migrationStatus);

	// not an entry point.
	// lock to shardManagerMembersMutex must be acquired because we work with mmSessionListeners
	void registerMMSessionListener(const unsigned operationId, ConsumerInterface * listener);


	/*
	 * Entry point #6 --
	 * called from SM to inform us about the arrival
	 * S lock on singleInstanceMutex
	 */
	void resolveSMNodeArrival(const Node & newNode);

	/*
	 * Entry point #7 --
	 * called from SM to inform us about failure of a node
	 * S lock on singleInstanceMutex
	 */
	void resolveSMNodeFailure(const NodeId failedNodeId);

	struct ResolveLocalArgs{
		ResolveLocalArgs(SP(ShardingNotification) notif){
			this->notif = notif;
		}
		SP(ShardingNotification) notif;
	};


	/*
	 * !!!NOT AN ENTRY POINT!!!
	 * No lock should be acquired in this method, this method is only called to pass the execution
	 * to another thread for local notification resolving which starts in _resolveLocal(void * request)
	 */
	bool resolveLocal(SP(ShardingNotification) request);
private:
	/*
	 * Entry point #8 --
	 * S in single instance mutex
	 * NOTE : must be similar to resolveMessage because it's
	 * just like receiving a notification from the same node
	 */
	static 	void * _resolveLocal(void * request);
public:

	TransportManager * getTransportManager() const;
	ConfigManager * getConfigManager() const;
	ResourceMetadataManager * getMetadataManager() const;
//	ResourceLockManager * getLockManager() const;
	LockManager * _getLockManager() const;
	StateMachine * _getStateMachine() const;
	MigrationManager * getMigrationManager() const;
	void setDPInternal(DPInternalRequestHandler * dpInternal);
	DPInternalRequestHandler * getDPInternal() const;
	void initMigrationManager();
	void attachToTransportManager(TransportManager * tm);


    void initFirstNode(const bool shouldLock = true);
    // if writeviewLocked is NULL we lock the current writeview and use that
	void updateCurrentNodeId(Cluster_Writeview * writeviewLocked = NULL);

	void setJoined();
	bool isJoined();
	void setCancelled();

	/*
	 * IMPORTANT NOTE:
	 * *** This is not a normal entry point. This entry point to lock manager of
	 *     shard manager is only used from the lock manager for local notification handling.
	 *     Should NOT lock the global mutex.
	 */
	bool isCancelled() ;


	void setLoadBalancing();
	void resetLoadBalancing();
	bool isLoadBalancing();
	pthread_t * getLoadbalancingThread() ;


	void print();
private:


	/*
	 * This mutex is used to make sure main pointer of ShardManager is not deleted until all requests leave.
	 * so we have S lock on this mutex in the beginning of all entry points and also in the beginning and end of
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
	 */
    boost::shared_mutex shardManagerMembersMutex;

	NodeId currentNodeId;
    vector<SP(ShardingNotification)> bouncedNotifications;
	void saveBouncedNotification(SP(ShardingNotification) notif);
	void resendBouncedNotifications();
	void bounceNotification(SP(ShardingNotification) notif);
	//TODO : must become const, assumes S lock acquired
	void printBouncedNotifications();

	// map from operatioId to MM listener
	vector< std::pair< map<unsigned , ConsumerInterface *>, boost::mutex * > > mmSessionListenersGroup;
	vector< map<unsigned , SP(Transaction) > >mmSessionListenersGroup_TransSharedPointers;

	bool joinedFlag;
	bool cancelledFlag;
	bool loadBalancingFlag;
	pthread_t * loadBalancingThread;


	/*
	 * Periodically runs a piece of code on shard manager mainly for load balancing.
	 * S lock on singleInstanceLock must be in main scope and released when isCancelled() returns true
	 *
	 */
	static void * periodicWork(void *args) ;


public:
	/*
	 * Entry point #9 --
	 * S in single instance mutex
	 */
	void getNodeInfoJson(Json::Value & nodeInfo);


};


}
}

#endif // __SHARDING_SHARDING_SHARDMANAGER_H__
