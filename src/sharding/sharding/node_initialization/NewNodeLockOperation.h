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
