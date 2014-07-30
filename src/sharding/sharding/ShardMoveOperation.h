#ifndef __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__

#include "State.h"
#include "Notification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {



class ShardMoveOperation : public LoadBalancingOperation{
public:
	class OUTPUT : public StateTransitionOutput{
	public:
		OUTPUT(ShardMoveOperation * shardMoveOperation, Notification * outputNotification);
		void execute();
		StateTransitionOutputType getType();
	private:
		Notification * outputNotification;
		ShardMoveOperation * shardMoveOperation;
	};


	ShardMoveOperation(unsigned operationId, ShardMoveChange * change):OperationState(operationId){
		this->change = change;
	};

	class DEST{
	public:
		class PROPOSED : public ShardMoveOperation {
		public:
			PROPOSED(unsigned operationId, ShardMoveChange * change);
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

		class LOCKING : public ShardMoveOperation {
		public:
			LOCKING(unsigned operationId, ShardMoveChange * change);
			OperationStateType getType();
			std::pair<OperationState *, StateTransitionOutput *> entry();
			virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
				return {this, NULL};
			}
			bool doesExpect(Notification * inputNotification){
				return false;
			}
			std::pair<OperationState *, StateTransitionOutput *> handle(LockingNotification::GRANTED * inputNotification);
			bool doesExpect(LockingNotification::GRANTED * inputNotification);
		private:

		};

		class MOVING : public ShardMoveOperation {
		public:
			MOVING(unsigned operationId, ShardMoveChange * change);
			OperationStateType getType();
			std::pair<OperationState *, StateTransitionOutput *> entry();
			virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
				return {this, NULL};
			}
			bool doesExpect(Notification * inputNotification){
				return false;
			}
			std::pair<OperationState *, StateTransitionOutput *> handle(MMFinishedNotification * inputNotification);
			std::pair<OperationState *, StateTransitionOutput *> handle(MMFailedNotification * inputNotification);
			bool doesExpect(MMFinishedNotification * inputNotification);
			bool doesExpect(MMFailedNotification * inputNotification);
		private:

		};

		class COMMITTING : public ShardMoveOperation {
		public:
			COMMITTING(unsigned operationId, ShardMoveChange * change);
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
	};


	class SRC{
	public:
		class MOVING : public ShardMoveOperation {
		public:
			MOVING(unsigned operationId, ShardMoveChange * change);
			OperationStateType getType();
			std::pair<OperationState *, StateTransitionOutput *> entry();
			virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
				return {this, NULL};
			}
			bool doesExpect(Notification * inputNotification){
				return false;
			}
			std::pair<OperationState *, StateTransitionOutput *> handle(CommitNotification * inputNotification);
			std::pair<OperationState *, StateTransitionOutput *> handle(MMFinishedNotification * inputNotification);
			std::pair<OperationState *, StateTransitionOutput *> handle(MMFailedNotification * inputNotification);
			bool doesExpect(CommitNotification * inputNotification);
			bool doesExpect(MMFinishedNotification * inputNotification);
			bool doesExpect(MMFailedNotification * inputNotification);
		private:

		};

		class CLEANUP : public ShardMoveOperation {
		public:
			CLEANUP(unsigned operationId, ShardMoveChange * change);
			OperationStateType getType();
			std::pair<OperationState *, StateTransitionOutput *> entry();
			virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
				return {this, NULL};
			}
			bool doesExpect(Notification * inputNotification){
				return false;
			}
			std::pair<OperationState *, StateTransitionOutput *> handle(LockingNotification * inputNotification);
			std::pair<OperationState *, StateTransitionOutput *> handle(SMNodeFailureNotification * inputNotification);
			bool doesExpect(LockingNotification * inputNotification);
			bool doesExpect(SMNodeFailureNotification * inputNotification);
		private:

		};

		class RECOVERY : public ShardMoveOperation {
		public:
			RECOVERY(unsigned operationId, ShardMoveChange * change);
			OperationStateType getType();
			std::pair<OperationState *, StateTransitionOutput *> entry();
			virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
				return {this, NULL};
			}
			bool doesExpect(Notification * inputNotification){
				return false;
			}
			std::pair<OperationState *, StateTransitionOutput *> handle(CommitNotification::ACK * recoveryNotification);
			bool doesExpect(CommitNotification::ACK * inputNotification);
		private:

		};
	private:

	};
private:
	ShardMoveChange * change;

};


}
}


#endif // __SHARDING_SHARDING_SHARD_MOVE_OPERATION_H__
