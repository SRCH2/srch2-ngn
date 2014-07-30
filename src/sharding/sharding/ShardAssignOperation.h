#ifndef __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__

#include "State.h"
#include "Notification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ShardAssignOperation : public LoadBalancingOperation {
public:

	class OUTPUT : public StateTransitionOutput{
	public:
		OUTPUT(ShardAssignOperation * shardAssignOperation, Notification * outputNotification);
		void execute();
		StateTransitionOutputType getType();
	private:
		Notification * outputNotification;
		ShardAssignOperation * shardAssignOperation;
	};

	ShardAssignOperation(unsigned operationId, ShardAssignChange * change);

	class PROPOSED : public ShardAssignOperation {
	public:
		PROPOSED(unsigned operationId, ShardAssignChange * change);
		OperationStateType getType();
		std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(ProposalNotification * inputNotification);
		std::pair<OperationState *, StateTransitionOutput *> handle(ProposalNotification::OK * inputNotification);
		std::pair<OperationState *, StateTransitionOutput *> handle(ProposalNotification::NO * inputNotification);
		bool doesExpect(ProposalNotification * inputNotification);
		bool doesExpect(ProposalNotification::OK * inputNotification);
		bool doesExpect(ProposalNotification::NO * inputNotification);
	private:
	};

	class COMMITTING : public ShardAssignOperation {
	public:
		COMMITTING(unsigned operationId, ShardAssignChange * change);
		OperationStateType getType();
		std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(CommitNotification::ACK * inputNotification);
		bool doesExpect(CommitNotification::ACK * inputNotification);
	private:
	};

private:
	ShardAssignChange * change;
};

}
}

#endif // __SHARDING_SHARDING_SHARD_ASSIGN_OPERATION_H__
