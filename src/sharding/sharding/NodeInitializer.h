#ifndef __SHARDING_SHARDING_NODE_INITIALIZER_H__
#define __SHARDING_SHARDING_NODE_INITIALIZER_H__

#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {



class NodeInitializerOperation : public OperationState{
public:

	class OUTPUT : public StateTransitionOutput{
	public:
		void execute();
		StateTransitionOutputType getType();
	private:
		Notification * outputNotification;
	};

	NodeInitializerOperation(unsigned operationId, NodeId newNodeId, NodeId hostNodeId);
	NodeId getNewNodeId() const;
	NodeId getHostNodeId() const;
private:
	NodeId newNodeId;
	NodeId hostNodeId;
};

class NewNodeOperation : public NodeInitializerOperation{
public:
	class INIT : public NodeInitializerOperation{
	public:
		OperationStateType getType();
		virtual std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::WELCOME * inputNotification);
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::NEW_HOST * inputNotification);
		bool doesExpect(NodeInitNotification::WELCOME * inputNotification);
		bool doesExpect(NodeInitNotification::NEW_HOST * inputNotification);
	private:
	};
	class READY : public NodeInitializerOperation{
	public:
		OperationStateType getType();
		std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::NEW_HOST * inputNotification);
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::SHARD_OFFER * inputNotification);
		bool doesExpect(NodeInitNotification::NEW_HOST * inputNotification);
		bool doesExpect(NodeInitNotification::SHARD_OFFER * inputNotification);
	private:
	};
	class DONE : public NodeInitializerOperation{
	public:
		OperationStateType getType();
		std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::NEW_HOST * inputNotification);
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::JOIN_PERMIT * inputNotification);
		bool doesExpect(NodeInitNotification::NEW_HOST * inputNotification);
		bool doesExpect(NodeInitNotification::JOIN_PERMIT * inputNotification);
	private:
	};
private:
};

class HostNodeOperation : public NodeInitializerOperation{
public:
	class WAIT : public NodeInitializerOperation{
	public:
		OperationStateType getType();
		std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::BUSY * inputNotification);
		bool doesExpect(NodeInitNotification::BUSY * inputNotification);
	private:
	};
	class OFFERED : public NodeInitializerOperation{
	public:
		OperationStateType getType();
		std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::SHARDS_READY * inputNotification);
		bool doesExpect(NodeInitNotification::SHARDS_READY * inputNotification);
	private:
	};
	class UPDATING : public NodeInitializerOperation{
	public:
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


class OtherNodesOperation : public NodeInitializerOperation{
public:
	class WAIT : public NodeInitializerOperation{
	public:
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
	class RECOVERY : public NodeInitializerOperation{
	public:
		OperationStateType getType();
		std::pair<OperationState *, StateTransitionOutput *> entry();
		virtual std::pair<OperationState *, StateTransitionOutput *> handle(Notification * inputNotification){
			return {this, NULL};
		}
		bool doesExpect(Notification * inputNotification){
			return false;
		}
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::BUSY * inputNotification);
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::SHARD_REQUEST * inputNotification);
		std::pair<OperationState *, StateTransitionOutput *> handle(NodeInitNotification::SHARDS_READY * inputNotification);
		bool doesExpect(NodeInitNotification::BUSY * inputNotification);
		bool doesExpect(NodeInitNotification::SHARD_REQUEST * inputNotification);
		bool doesExpect(NodeInitNotification::SHARDS_READY * inputNotification);
	private:
	};
	class UPDATING : public NodeInitializerOperation{
	public:
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


/*
 * This class is responsible of joining a new node to the cluster
 */
class NodeInitializer{
public:
	static NodeInitializer * createNodeInitializer();
	static NodeInitializer * getNodeInitializer();
	/*
	* the first method called when a notification comes for LockManager,
	* this method uses other functions and the buffer to take care of this notification.
	*/

	void resolveSMNodeArrival(const Node & newNode);

	void resolve(NodeInitNotification::WELCOME * welcome);
	void resolve(NodeInitNotification::BUSY * busy);
	void resolve(NodeInitNotification::NEW_HOST * newHost);
	void resolve(NodeInitNotification::SHARD_REQUEST * shardRequest);
	void resolve(NodeInitNotification::SHARD_OFFER * shardOffer);
	void resolve(NodeInitNotification::SHARDS_READY * shardsReady);
	void resolve(NodeInitNotification::JOIN_PERMIT * joinPermit);
	void resolve(CommitNotification::ACK * commitAckNotification);

	void resolve(CommitNotification * commitNotification);
	void doesExpect(CommitNotification * commitNotification);

private:
	static NodeInitializer * singleInstance;

	//operationId => NewNodeOperation
	map<unsigned, NodeInitializerOperation *> operations;
	// When it's not null, it means we are initializing this node
	NewNodeOperation * initOperation;
	std::queue<Notification *> buffer;

};

}
}


#endif // __SHARDING_SHARDING_NODE_INITIALIZER_H__
