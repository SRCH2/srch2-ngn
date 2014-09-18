#ifndef __SHARDING_SHARDING_SHARDMANAGER_H__
#define __SHARDING_SHARDING_SHARDMANAGER_H__

#include "core/util/Assert.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include "boost/shared_ptr.hpp"

#include "sharding/transport/Message.h"
#include "sharding/transport/TransportManager.h"
#include "sharding/configuration/ConfigManager.h"
#include "sharding/configuration/ShardingConstants.h"
#include "sharding/transport/CallbackHandler.h"
#include "notifications/LockingNotification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ClusterResourceMetadata_Readview;
class Cluster_Writeview;
class ClusterOperationStateMachine;
class ResourceLockManager ;
class ResourceMetadataManager;
class MigrationManager;
struct ShardMigrationStatus;

/*
 * This class is the main responsible class to do operations on shards of data in the cluster.
 * For example, Load balancing upon arrival of new nodes or failure of existing nodes is handled in this class.
 */
class ShardManager : public CallBackHandler{
public:

	static ShardManager * createShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager);
	static void deleteShardManager();
	static ShardManager * getShardManager();
	static NodeId getCurrentNodeId();
	static Cluster_Writeview * getWriteview();
	static void getReadview(boost::shared_ptr<const ClusterResourceMetadata_Readview> & readview);

	ShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager);
	~ShardManager();
	void start();

	// sends this sharding notification to destination using TM
	bool send(ShardingNotification * notification);
	// callback provided to TransportManager, it deserializes the messages and passes them to
	// notification resolve methods
	bool resolveMessage(Message * msg, NodeId node);

	void resolve(LockingNotification::ACK * lockAckNotif);
	// called from destructor of readview to notify shard manager that the readview is released and
	// no more readers are using it
	void resolveReadviewRelease(unsigned metadataVersion);
	// called from migration manager to inform us about the status of a migration
	void resolveMMNotification(const ShardMigrationStatus & migrationStatus);
	// called from SM to inform us about the arrival or failure of a node
	void resolveSMNodeArrival(const Node & newNode);

	void resolveSMNodeFailure(const NodeId failedNodeId);

	// getter functions
	TransportManager * getTransportManager() const;
	ConfigManager * getConfigManager() const;
	ResourceMetadataManager * getMetadataManager() const;
	ResourceLockManager * getLockManager() const;
	ClusterOperationStateMachine * getStateMachine() const;
	void initMigrationManager();
	void attachToTransportManager(TransportManager * tm);
    boost::mutex shardManagerGlobalMutex;

	void setJoined();
	bool isJoined() const;
	void setCancelled();
	bool isCancelled() ;
	void setLoadBalancing();
	void resetLoadBalancing();
	bool isLoadBalancing() const;

	void print();
private:


	static ShardManager * singleInstance;

	TransportManager * transportManager;
	ConfigManager * configManager;

	MigrationManager *migrationManager;

	ResourceLockManager * lockManager;
    ResourceMetadataManager * metadataManager;



	ClusterOperationStateMachine * stateMachine;

	vector<ShardingNotification *> bouncedNotifications;

	bool joinedFlag;
	bool cancelledFlag;
	bool loadBalancingFlag;

	static void * periodicWork(void *args) ;

	void saveBouncedNotification(ShardingNotification * notif);
	void bounceNotification(ShardingNotification * notif);

};


}
}

#endif // __SHARDING_SHARDING_SHARDMANAGER_H__
