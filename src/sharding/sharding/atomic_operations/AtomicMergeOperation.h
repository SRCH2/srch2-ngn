#ifndef __SHARDING_SHARDING_ATOMIC_MERGE_OPERATION_H__
#define __SHARDING_SHARDING_ATOMIC_MERGE_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../../configuration/ShardingConstants.h"

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
class AtomicMergeOperation: public OperationState {
public:

	AtomicMergeOperation(const unsigned operationId);
	~AtomicMergeOperation();

	// send merge notification to all nodes in the cluster
	// they will do the merge and reply MERGE::ACK
	// when everybody replied we are done.
	OperationState * entry();
	OperationState * handle(MergeNotification::ACK * ack);
	OperationState * handle(NodeFailureNotification * nodeFailure);
	OperationState * handle(Notification * notification);

	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	map<NodeId, bool> nodesStatus;

	OperationState * finalize();

	bool haveAllNodeseReplied() const;

	static void * localMerge(void * arg);
};


}
}

#endif // __SHARDING_SHARDING_ATOMIC_MERGE_OPERATION_H__
