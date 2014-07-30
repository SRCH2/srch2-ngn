#ifndef __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__
#define __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__

#include "State.h"
#include "Notification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class ShardCopyOperation : public LoadBalancingOperation {
public:


	class OUTPUT : public StateTransitionOutput{
	public:
		OUTPUT(ShardCopyOperation * shardCopyOperation, Notification * outputNotification);
		void execute();
		StateTransitionOutputType getType();
	private:
		Notification * outputNotification;
		ShardCopyOperation * shardCopyOperation;
	};


	ShardCopyOperation(unsigned operationId, ShardCopyChange * change);

	class SRC{
	public:

		class COPYING : public ShardCopyOperation{
		public:
			COPYING(unsigned operationId, ShardCopyChange * change);
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

	private:
	};
	class DEST{
	public:

		class PROPOSED : public ShardCopyOperation{
		public:
			PROPOSED(unsigned operationId, ShardCopyChange * change);
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


		class LOCKING : public ShardCopyOperation{
		public:
			LOCKING(unsigned operationId, ShardCopyChange * change);
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


		class COPYING : public ShardCopyOperation{
		public:
			COPYING(unsigned operationId, ShardCopyChange * change);
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


		class COMMITTING : public ShardCopyOperation{
		public:
			COMMITTING(unsigned operationId, ShardCopyChange * change);
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
private:
	ShardCopyChange * change;
};


}
}

#endif // __SHARDING_SHARDING_SHARD_COPY_OPERATION_H__
