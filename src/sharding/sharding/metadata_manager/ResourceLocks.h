#ifndef __SHARDING_SHARDING_RESOURCE_LOCKS_H__
#define __SHARDING_SHARDING_RESOURCE_LOCKS_H__

#include "Shard.h"
#include "../notifications/Notification.h"
#include "../notifications/LockingNotification.h"
#include "../../configuration/ShardingConstants.h"
#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class NewNodeLockNotification;

const int LOCK_REQUEST_PRIORITY_LOAD_BALANCING = 50;
const int LOCK_REQUEST_PRIORITY_NODE_ARRIVAL = 100;

/*
 * This object describes a lock request for a single resource (shard or partition).
 * There are 3 different lock requests :
 * 1. Grab a lock : S, X or U. Lock type, resource id and lock holders is provided.
 * 2. Release a lock : only resource id and lock holders is provided.
 * 3. Upgrade/Downgrade a lock : only request type and resource id is provided.
 */
struct SingleResourceLockRequest{
	ClusterShardId resource;
	string primaryKey; // either "resource" or "primaryKey" is used.
	ResourceLockType lockType;
	ResourceLockRequestType requestType;
	vector<NodeOperationId> holders;

	// Lock
	SingleResourceLockRequest(const ClusterShardId & resource,const vector<NodeOperationId> & holders,ResourceLockType lockType);
	// Release
	SingleResourceLockRequest(const ClusterShardId & resource,const vector<NodeOperationId> & holders);
	// Lock
	SingleResourceLockRequest(const ClusterShardId & resource,const NodeOperationId & holder,ResourceLockType lockType);
	// Release
	SingleResourceLockRequest(const ClusterShardId & resource,const NodeOperationId & holders);
	// Upgrade or Downgrade
	SingleResourceLockRequest(const ClusterShardId & resource, ResourceLockRequestType requestType);


//	// Lock
//	SingleResourceLockRequest(const string & primaryKey,const vector<NodeOperationId> & holders,ResourceLockType lockType);
//	// Release
//	SingleResourceLockRequest(const string & primaryKey,const vector<NodeOperationId> & holders);
	// Lock
	SingleResourceLockRequest(const string & primaryKey,const NodeOperationId & holder,ResourceLockType lockType);
	// Release
	SingleResourceLockRequest(const string & primaryKey,const NodeOperationId & holders);

	SingleResourceLockRequest(const SingleResourceLockRequest & copy);
	SingleResourceLockRequest(){};
	~SingleResourceLockRequest(){

	};

	bool operator==(const SingleResourceLockRequest & right);

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);

	bool applyNodeFailure(const unsigned failedNodeId);

	string toString() const;
};

struct PendingLockRequest{

	PendingLockRequest(const NodeOperationId & requesterAddress,
			ShardingMessageType ackType,
			const unsigned priority,
			ResourceLockRequest * request);
	PendingLockRequest(const PendingLockRequest & copy);
	PendingLockRequest & operator=(const PendingLockRequest & rhs);
	PendingLockRequest(){
		request = NULL;
	}
	~PendingLockRequest();
	bool operator<(const PendingLockRequest & right) const;
	bool operator==(const PendingLockRequest & right) const;
	bool operator>(const PendingLockRequest & right) const;
	bool applyNodeFailure(const unsigned failedNodeId);

	ResourceLockRequest * request;
	NodeOperationId requesterAddress;
	unsigned priority;
	unsigned metadataVersionId;
	ShardingMessageType ackType;
	string  toString() const;
};

class PendingLockRequestBuffer{
public:

	void push(const PendingLockRequest & pendingRequest);
	// doesn't remove the request from the buffer, just to see what it is ...
	PendingLockRequest top(bool & hasMore);
	bool pop();
	void update(const PendingLockRequest & pendingRequest);
	void applyNodeFailure(const unsigned failedNodeId);
	void print() const;
private:
	vector<PendingLockRequest> pendingRequests;
};

struct LockHoldersRepository{
	LockHoldersRepository();
	LockHoldersRepository(const LockHoldersRepository & repos);
	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	bool isFree(const ClusterShardId & resource) const;
	bool isPartitionLocked(const ClusterPID & pid) const;
	void getAllLockedResources(vector<ClusterShardId> & resources) const;
	void applyNodeFailure(const unsigned failedNodeId);
	map<ClusterShardId, vector<NodeOperationId> > S_Holders;
	map<ClusterShardId, vector<NodeOperationId> > U_Holders; // multiple operationIds can have xLock of a resource together if they know they are consistent with
	map<ClusterShardId, vector<NodeOperationId> > X_Holders; // each other.

	// for primary keys of records : insert/update/delete must acquire this lock before they can perform
	map<string, NodeOperationId > recordXLockHolders;

	void printLockHolders(const map<ClusterShardId, vector<NodeOperationId> > & holders, const string & tableName) const;
	void printRecordLockHolders() const;
	void print() const;
	bool operator==(const LockHoldersRepository & right) const;
	void clear();

private:
	void * serializeHolderList(void * buffer,
			const map<ClusterShardId, vector<NodeOperationId> > & holders) const;
	unsigned getNumberOfBytesHolderList(const map<ClusterShardId, vector<NodeOperationId> > & holders) const;
	void * deserializeHolderList(map<ClusterShardId, vector<NodeOperationId> > & holders, void * buffer);
};

class ResourceLockManager{
public:
	ResourceLockManager();
	ResourceLockManager(const ResourceLockManager & copy);


	void resolve(NewNodeLockNotification * notification);
	void resolve(LockingNotification * notification);
	void resolve(LockingNotification::RV_RELEASED * inputNotification);
	void resolve(NodeFailureNotification * nodeFailureNotif);


	bool canAcquireLock(const ClusterShardId & resource, ResourceLockType lockType);

	void setLockHolderRepository(LockHoldersRepository * shardLockHolders);
	LockHoldersRepository * getLockHolderRepository() const;
	// this functions either executes all requests in this batch or non of them
	// and either puts the request in pending requests or sends the ack
	bool resolveBatch(const NodeOperationId & requesterAddress, const unsigned priority,
			ResourceLockRequest * lockRequest, const ShardingMessageType & ackType);

	// it returns a map giving the result for each primaryKey in the lockRequest
	void resolveRecordLockBatch(const NodeOperationId & requesterAddress, ResourceLockRequest * lockRequest, map<string, bool> & results);

	LockHoldersRepository * getShardLockHolders();

	bool isPartitionLocked(const ClusterPID & pid);

	void getLockedPartitions(vector<ClusterPID> & lockedPartitions);


	void print() ;

	// NOTE : added to public API for testing.
	void executeRequest(const SingleResourceLockRequest & request);
	bool canGrantRequest(const SingleResourceLockRequest & request);
private:

	void tryPendingRequest();
	void sendAck(const PendingLockRequest & pendingRequest, const bool isGranted);

private:

	LockHoldersRepository * lockHolders;
	PendingLockRequestBuffer pendingLockRequestBuffer;
	vector<PendingLockRequest > pendingRVReleaseRequests;
    boost::mutex readviewReleaseMutex;

	void printRVReleasePendingRequests();

	void executeBatch(const vector<SingleResourceLockRequest *> & requestBatch, bool & needCommit);
	void executeRecordBatch(const vector<SingleResourceLockRequest *> & requestBatch);
	bool canGrantRequest(const ResourceLockRequest * lockRequest);
	// returns false if not found
	void release(const ClusterShardId & shardId, const vector<NodeOperationId> & holders, LockHoldersRepository & lockRepository);
	void release(const string & primaryKey);
	bool canRelease(const ClusterShardId & shardId, LockHoldersRepository & lockRepository);
	bool canRelease(const string & primaryKey);


	// returns false if it could not acquire the lock
	void lock(ResourceLockType lockType, const ClusterShardId & resource, const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository);
	bool canLock(ResourceLockType lockType, const ClusterShardId & resource, LockHoldersRepository & lockRepository);

	void lock(const string & primaryKey, const NodeOperationId & lockHolder);
	bool canLock(const string & primaryKey);

	void lock_S(const ClusterShardId & resource, const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository);
	bool canLock_S(const ClusterShardId & resource, LockHoldersRepository & lockRepository);
	void lock_U(const ClusterShardId & resource, const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository);
	bool canLock_U(const ClusterShardId & resource, LockHoldersRepository & lockRepository);
	void lock_X(const ClusterShardId & resource, const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository);
	bool canLock_X(const ClusterShardId & resource, LockHoldersRepository & lockRepository);

	// returns false if upgrade is not possible at this point
	// isValid is false if this U lock is not there to upgrade
	void upgrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository);
	bool canUpgrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository);


	void downgrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository);
	bool canDowngrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository);
};

}
}

#endif // __SHARDING_SHARDING_RESOURCE_LOCKS_H__
