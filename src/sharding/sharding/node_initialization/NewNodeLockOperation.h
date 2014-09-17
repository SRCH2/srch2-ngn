#ifndef __SHARDING_SHARDING_NEW_NODE_LOCK_OPERATION_H__
#define __SHARDING_SHARDING_NEW_NODE_LOCK_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../notifications/NewNodeLockNotification.h"
#include "../metadata_manager/ResourceLocks.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../metadata_manager/ResourceMetadataManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


/*
 * Note about the effect of node arrival and node failure on this operation :
 *
 * Node arrival does not affect this operator because this operator is only interested in those nodes
 * that have some smaller node ID. If a new node comes, it will have a greater node ID and therefore
 * no change needs to be done on this node.
 * Node failure: when it happens,
 * 1. We have already passed that node ID : only nodeIndex must decrement because that node
 *    is going to be removed from the allNodesUpToCurrentNode vector.
 * 2. We are waiting for this node: we move to the next node or if there is no more nodes, we
 *    are done.
 * 3. We have not reached to this node, no need to do anything, just remove the node from that vector.
 */

class NewNodeLockOperation : public OperationState{
public:

	NewNodeLockOperation(const unsigned & operationId);

	~NewNodeLockOperation();

	// sends the node list and lock request to the node with minimum id
	OperationState * entry();
	OperationState * handle(Notification * notification){
		if(notification == NULL){
			ASSERT(false);
			return this;
		}
		switch (notification->messageType()) {
			case ShardingNodeFailureNotificationMessageType:
				return handle((NodeFailureNotification *)notification);
			case ShardingNewNodeLockACKMessageType:
				return handle((NewNodeLockNotification::ACK *)notification);
			default:
				// ignore;
				return this;
		}
	}
	OperationState * handle(NodeFailureNotification * nodeFailure);
	OperationState * handle(NewNodeLockNotification::ACK * ack);

	string getOperationName() const ;
	string getOperationStatus() const ;

	bool doesExpect(NewNodeLockNotification::ACK * ack) const;
private:
	// NOTE : this vector does not include current node ID
	//        all elements are smaller than the current node ID.
	vector<NodeId> allNodesUpToCurrentNode;
//	vector<SingleResourceLockRequest *> lockRequests;
	ResourceLockRequest * lockRequests;
	LockHoldersRepository * lastShardLockRepository;

	NewNodeLockNotification * newNodeLockNotification;

	unsigned nodeIndex;

};


}
}

#endif // __SHARDING_SHARDING_NEW_NODE_LOCK_OPERATION_H__
