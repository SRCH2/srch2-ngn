#ifndef __SHARDING_SHARDING_SERIAL_LOCK_OPERATION_H__
#define __SHARDING_SHARDING_SERIAL_LOCK_OPERATION_H__

#include "State.h"
#include "notifications/Notification.h"
#include "metadata_manager/ResourceLocks.h"
#include "sharding/configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

// Must be used for Lock and Upgrade
// it does the work in the order of nodes.

struct SerialLockResultStatus{
	SerialLockResultStatus(){
		grantedFlag = false;
	}
	bool grantedFlag;
};

class SerialLockOperation : public OperationState{
public:
	SerialLockOperation(const unsigned & operationId, ResourceLockRequest * lockRequests, SerialLockResultStatus * resultStatus = NULL);
	SerialLockOperation(const unsigned & operationId, const vector<NodeId> & participants,
			ResourceLockRequest * lockRequests, SerialLockResultStatus * resultStatus = NULL);

	~SerialLockOperation(){
		delete lockRequests;
	}
	OperationState * entry();
	OperationState * handle(Notification * notification){
		if(notification == NULL){
			ASSERT(false);
			return this;
		}
		switch (notification->messageType()) {
			case ShardingNodeFailureNotificationMessageType:
				return handle((NodeFailureNotification *)notification);
			case ShardingLockACKMessageType:
				return handle((LockingNotification::ACK * )notification);
			default:
				// ignore;
				return this;
		}
	}

	OperationState * handle(LockingNotification::ACK * ack);
	OperationState * handle(NodeFailureNotification * nodeFailure);

	bool doesExpect(const LockingNotification::ACK * ack) const;

	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	vector<NodeId> participantNodes;
	ResourceLockRequest * lockRequests;
	unsigned nodeIndex;
	SerialLockResultStatus * resultStatus;

	OperationState * askNextNode(NodeId targetNodeId);

	OperationState * handleRejectNonBlocking();

	struct LockRequestArguments{
		ResourceLockRequest * lockRequest;
		unsigned priority;
		NodeOperationId requester;
		ShardingMessageType ackType;
	};

	static void * localLockRequest(void *);

};

}
}


#endif // __SHARDING_SHARDING_SERIAL_LOCK_OPERATION_H__
