#ifndef __SHARDING_SHARDING_STATE_H__
#define __SHARDING_SHARDING_STATE_H__

#include "Notification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

enum StateTransitionOutputType{
	LockOutputType_Lock_Granted,
	LockOutputType_Lock_Rejected,
	LockOutputType_Commit_ACK,
	LockOutputType_NodeInitialization,
	StateTransitionOutputType_Proposal_OK,
	StateTransitionOutputType_Proposal_NO,
	LockOutputType_Default
};

enum OperationStateType{
	OperationStateType_Lock,
	OperationStateType_Commit,
	OperationStateType_NodeInitializer_NewNode_Init,
	OperationStateType_NodeInitializer_NewNode_Ready,
	OperationStateType_NodeInitializer_NewNode_Done,
	OperationStateType_NodeInitializer_HostNode_Wait,
	OperationStateType_NodeInitializer_HostNode_Offered,
	OperationStateType_NodeInitializer_HostNode_Updating,
	OperationStateType_NodeInitializer_OtherNodes_Wait,
	OperationStateType_NodeInitializer_OtherNodes_Recovery,
	OperationStateType_NodeInitializer_OtherNodes_Updating,
	OperationStateType_ShardCopy_Src_Copying,
	OperationStateType_ShardCopy_Dest_Proposed,
	OperationStateType_ShardCopy_Dest_Locking,
	OperationStateType_ShardCopy_Dest_Copying,
	OperationStateType_ShardCopy_Dest_Commiting,
	OperationStateType_ShardAssign_Proposed,
	OperationStateType_ShardAssign_Commiting,
	OperationStateType_ShardMove_Dest_Proposed,
	OperationStateType_ShardMove_Dest_Locking,
	OperationStateType_ShardMove_Dest_Moving,
	OperationStateType_ShardMove_Dest_Committing,
	OperationStateType_ShardMove_Src_Moving,
	OperationStateType_ShardMove_Src_Cleanup,
	OperationStateType_ShardMove_Src_Recovery

};

class StateTransitionOutput{
public:
	virtual void execute() = 0;
	virtual StateTransitionOutputType getType() = 0;
	virtual ~StateTransitionOutput(){};
};


/*
 * OperationState {operationId}
 *  |
 *  |_____________LockManagerOperation {ResourceLockRequest * request}
 *  |                        |_____________ LocalLocking
 *  |                        |_____________ GlobalLocking
 *  |
 *  |_____________CommitOperation {MetadataChange * change}
 *  |
 *  |_____________NodeInitializerOperation {hostNodeId, newNodeId}
 *  |                        |_____________NewNodeOperation
 *  |                        |_____________________________::INIT
 *  |                        |_____________________________::READY
 *  |                        |_____________________________::DONE
 *  |                        |_____________HostNodeOperation
 *  |                        |______________________________::WAIT
 *  |                        |______________________________::OFFERED
 *  |                        |______________________________::UPDATING
 *  |                        |_____________OtherNodesOperation
 *  |                        |________________________________::WAIT
 *  |                        |________________________________::RECOVERY
 *  |                        |________________________________::UPDATING
 *  |
 *  |____________LoadBalancingOperation
 *                           |____________ShardCopyOperation {ShardCopyChange change}
 *                           |                        |     ::SRC
 *                           |                        |___________::COPYING
 *                           |                        |      ::DEST
 *                           |                        |____________::PROPOSED
 *                           |                        |____________::LOCKING
 *                           |                        |____________::COPYING
 *                           |                        |____________::COMMITTING
 *                           |
 *                           |____________ShardAssignOperation {ShardAssignChange change}
 *                           |                        |_______::PROPOSED
 *                           |                        |_______::COMMITTING
 *                           |
 *                           |____________ShardMoveOperation {ShardMoveOperation change}
 *                           |                        |     ::SRC
 *                           |                        |__________::MOVING
 *                           |                        |__________::CLEANUP
 *                           |                        |__________::RECOVERY
 *                           |                        |      ::DEST
 *                           |                        |____________::PROPOSED
 *                           |                        |____________::LOCKING
 *                           |                        |____________::COPYING
 *                           |                        |____________::COMMITTING
 *                           |
 */


/*
 * This class provides the interface of the state of one operation
 */
class OperationState{
public:

	OperationState(unsigned operationId):operationId(operationId){}

	virtual OperationStateType getType() = 0;
	virtual ~OperationState(){};

	unsigned getOperationId() const{
		return operationId;
	}
private:
	const unsigned operationId;
};

}
}

#endif // __SHARDING_SHARDING_STATE_H__
