#ifndef __SHARDING_SHARDING_CLUSTER_SAVE_OPERATION_H__
#define __SHARDING_SHARDING_CLUSTER_SAVE_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../metadata_manager/Shard.h"
#include "server/HTTPJsonResponse.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Saves the state of cluster.
 * 1. puts S lock on all shards in the cluster
 * 2. merges all shards by using the AtimicMergeOperation
 * 3. saves the state of the cluster by using the AtomicSaveOperation
 * 4. releases all locks
 */
class ClusterSaveOperation: public OperationState {
public:

	// initialize class members
	ClusterSaveOperation(evhttp_request *req);
	ClusterSaveOperation(unsigned operationId);
	~ClusterSaveOperation();

	/*
	 * lock();
	 */
	OperationState * entry();

private:
	/*
	 * 1. prepare the lock request and create the lock operation to acquire S locks on all shards
	 * NOTE : lock request must be blocking
	 * NOTE : all other operation objects must be NULL
	 */
	OperationState * lock();

public:
	/*
	 * When the last lock ack is received,
	 * if we are in locking phase start the merge process :
	 * merge();
	 * else if we are in releasing process finalize :
	 * finalize();
	 */
	OperationState * handle(LockingNotification::ACK * lockAckNotif);
private:
	/*
	 * Create the AtomicMergeOperation and do a cluster wide merge
	 */
	OperationState * merge();
public:

	/*
	 * When the last merge ack is received, start the save process :
	 * save();
	 */
	OperationState * handle(MergeNotification::ACK * mergeAckNotif);
private:
	/*
	 * Create the AtomicSaveOperation and do a cluster wide save
	 */
	OperationState * save();
public:
	// operation should not return NULL upon receiving these acks
	OperationState * handle(SaveDataNotification::ACK * saveAckNotif);

	/*
	 * When the last save ack is received start the lock release process:
	 * release()
	 */
	OperationState * handle(SaveMetadataNotification::ACK * saveAckNotif);

private:
	// releases the locks on all resources
	OperationState * release();

	/*
	 * Write the response to the HTTP request channel that was received in construction
	 * and return NULL to finish this transaction.
	 */
	OperationState * finalize();

public:
	/*
	 * Based on the phase of this transaction
	 * (locking/merging/saving/releasing) give this notification to one of the operation members...
	 */
	OperationState * handle(NodeFailureNotification * nodeFailureNotif);

	OperationState * handle(Notification * notification);

	string getOperationName() const ;
	string getOperationStatus() const ;

private:

	evhttp_request *req;

	OperationState * lockOperation;
	OperationState * mergeOperation;
	OperationState * saveOperation;
	OperationState * releaseOperation;


	bool releaseFlag;
	bool printFlag;

	bool isInStartingPhase() const;
	bool isInLockingPhase() const;
	bool isInMergingPhase() const;
	bool isInSavingPhase() const;
	bool isInReleasingPhase() const;

};

}

}


#endif // __SHARDING_SHARDING_CLUSTER_SAVE_OPERATION_H__
