#ifndef __SHARDING_SHARDING_NOTIFICATION_H__
#define __SHARDING_SHARDING_NOTIFICATION_H__

#include "core/util/Assert.h"

#include "sharding/configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

struct NodeOperationId{
	NodeId nodeId;
	unsigned operationId;
	NodeOperationId(){
		// temp init used for deserialization
	}
	NodeOperationId(const NodeOperationId & id){
		this->nodeId = id.nodeId;
		this->operationId = id.operationId;
	}
	NodeOperationId(NodeId nodeId, unsigned operationId){
		this->nodeId = nodeId;
		this->operationId = operationId;
	}
	void * serialize(void * buffer) const{
		buffer = srch2::util::serializeFixedTypes(nodeId, buffer);
		buffer = srch2::util::serializeFixedTypes(operationId, buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0 ;
		numberOfBytes += sizeof(nodeId);
		numberOfBytes += sizeof(operationId);
		return numberOfBytes;
	}
	void * deserialize(void * buffer)const{
		buffer = srch2::util::deserializeFixedTypes(buffer, nodeId);
		buffer = srch2::util::deserializeFixedTypes(buffer, operationId);
		return buffer;
	}
};


/*
 * Notification {getType()}
 *  |____________SMNotification
 *  |                  |__________SMNodeArrivalNotification {Node newNode}
 *  |                  |__________SMNodeFailureNotification {failedNodeId}
 *  |
 *  |____________MMNotification {sessionId}
 *  |                  |__________MMFailedNotification {failureCode}
 *  |                  |__________MMFinishedNotification
 *  |
 *  |____________DPNotification
 *  |                  |__________DPLoadUpdateNotification {shardId -> additionalLoad}
 *  |
 *  |____________ShardingNotification {src and dest NodeOperationId}
 *  |                   |____________BroadcaseNotification {broadcastId}
 *  |                   |                      |______________CommitNotification {MetadataChange}
 *  |                   |                      |_________________________________::ACK
 *  |                   |                      |______________LockingNotification {ResourceLockRequest, acquireOrRelease}
 *  |                   |                      |_________________________________::GRANTED
 *  |                   |                      |_________________________________::REJECTED
 *  |____________________________________________________________________________::READVIEW_RELEASED {metadataVersionId}
 *  |                   |                      |______________ProposalNotification {MetadataChange}
 *  |                   |                      |__________________________________::OK
 *  |                   |                      |__________________________________::NO
 *  |                   |
 *  |                   |____________NodeInitializationNotification
 *  |                   |                      |___________________::WELCOME
 *  |                   |                      |___________________::BUSY
 *  |                   |                      |___________________::NEW_HOST
 *  |                   |                      |___________________::SHARD_REQUEST
 *  |                   |                      |___________________::SHARD_OFFER
 *  |                   |                      |___________________::SHARDS_READY
 *  |                   |                      |___________________::JOIN_PERMIT
 *  |                   |____________CopyToMeNotification {srcShardId, destShardId}
 *  |                   |____________MoveToMeNotification {shardId}
 *  |
 */


class Notification{
public:
	virtual NotificationType getType() const = 0;
    virtual ShardingMessageType messageType() const = 0;
	virtual ~Notification(){};
};
class SMNotification : public Notification{
};
class SMNodeArrivalNotification : public SMNotification{
public:
	SMNodeArrivalNotification(const Node & newNode){
		this->newNode = newNode;
	}
	NotificationType getType() const{
		return 	NotificationType_SM_NodeArrival;
	}
	Node getNewNode() const{
		return newNode;
	}
private:
	Node newNode;
};
class SMNodeFailureNotification : public SMNotification{
public:
	SMNodeFailureNotification(NodeId failedNode){
		this->failedNode = failedNode;
	}
	NotificationType getType() const{
		return 	NotificationType_SM_NodeFailure;
	}
	NodeId getFailedNodeId() const{
		return failedNode;
	}
private:
	NodeId failedNode;
};

class MMNotification : public Notification{
public:
	MMNotification(unsigned sessionId){
		this->sessionId = sessionId;
	}
	unsigned getSessionId() const{
		return this->sessionId;
	}
private:
	unsigned sessionId;
};

class MMFailedNotification : public MMNotification{
public:
	MMFailedNotification(unsigned sessionId, unsigned failureCode):MMNotification(sessionId){
		this->failureCode = failureCode;
	}
	NotificationType getType() const{
		return 	NotificationType_MM_Failed;
	}
	unsigned getFailureCode() const{
		return this->failureCode;
	}
private:
	unsigned failureCode;
};

class MMFinishedNotification : public MMNotification{
public:
	MMFinishedNotification(unsigned sessionId):MMNotification(sessionId){};
	NotificationType getType() const{
		return 	NotificationType_MM_Finished;
	}
};


class DPNotification : public Notification{
};


class DPLoadUpdateNotification : public DPNotification{
public:
	DPLoadUpdateNotification(map<ShardId, double> additionalLoads){
		this->additionalLoads = additionalLoads;
	};
	NotificationType getType() const{
		return 	NotificationType_DP_UpdateLoads;
	}
private:
	map<ShardId, double> additionalLoads;
};


class ShardingNotification : public Notification{
public:
	ShardingNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId);
	NodeOperationId getSrcOperationId() const;
	NodeOperationId getDestOperationId() const;

	virtual void * serialize(void * buffer) const;
	virtual unsigned getNumberOfBytes() const;
	virtual void * deserialize(void * buffer) const;

	Message * serialize(MessageAllocator * allocator) const{
		unsigned numberOfBytes = getNumberOfBytes();
		Message * msg = allocator->allocateMessage(numberOfBytes);
		void * bufferWritePointer = Message::getBodyPointerFromMessagePointer(msg);
		bufferWritePointer = serialize(bufferWritePointer);
		return msg;
	}

private:
    NodeOperationId srcOperationId;
    NodeOperationId destOperationId;
};

class BroadcastNotification : public ShardingNotification{
public:
	BroadcastNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId);
	virtual void * serialize(void * buffer) const;
	virtual unsigned getNumberOfBytes() const;
	virtual void * deserialize(void * buffer) const;


private:
};


class ProposalNotification : public ShardingNotification{
public:

	class OK : public ShardingNotification{
	public:
		OK(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId):ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){};

		NotificationType getType() const;
	    ShardingMessageType messageType() const;
	    static ProposalNotification::OK * deserializeAndConstruct(void * buffer) const{
	    	ProposalNotification::OK * newOK = new ProposalNotification::OK(0,0,0,0);
	    	newOK->deserialize(buffer);
	    	return newOK;
	    }
	};

	class NO : public ShardingNotification{
	public:
		NO(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId):ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){};

		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static ProposalNotification::NO * deserializeAndConstruct(void * buffer) const{
	    	ProposalNotification::NO * newNO = new ProposalNotification::NO(0,0,0,0);
	    	newNO->deserialize(buffer);
	    	return newNO;
	    }
	};

	ProposalNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId,
			MetadataChange * metadataChange);

	NotificationType getType() const;
	MetadataChange * getMetadataChange() const;

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer) const;
	ShardingMessageType messageType() const;
    static ProposalNotification * deserializeAndConstruct(void * buffer) const{
    	ProposalNotification * newProposal = new ProposalNotification(0,0,0,0,NULL);
    	newProposal->deserialize(buffer);
    	return newProposal;
    };


private:
	MetadataChange * metadataChange;
};

class LockingNotification : public BroadcastNotification{
public:


	class GRANTED : public ShardingNotification{
	public:
		GRANTED(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId):ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){};
		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static LockingNotification::GRANTED * deserializeAndConstruct(void * buffer) const{
	    	LockingNotification::GRANTED * newGranted = new LockingNotification::GRANTED(0,0,0,0);
	    	newGranted->deserialize(buffer);
	    	return newGranted;
	    }
	};
	class REJECTED : public ShardingNotification{
	public:
		REJECTED(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId):ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){};
		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static LockingNotification::REJECTED * deserializeAndConstruct(void * buffer) const{
	    	LockingNotification::REJECTED * newRejected = new LockingNotification::REJECTED(0,0,0,0);
	    	newRejected->deserialize(buffer);
	    	return newRejected;
	    }
	};

	class RELEASED : public ShardingNotification{
	public:
		RELEASED(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId):ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){};
		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static LockingNotification::RELEASED * deserializeAndConstruct(void * buffer) const{
	    	LockingNotification::RELEASED * newReleased = new LockingNotification::RELEASED(0,0,0,0);
	    	newReleased->deserialize(buffer);
	    	return newReleased;
	    }
	};

	class RV_RELEASED : public Notification{
	public:
		RV_RELEASED(unsigned metadataVersionId);
		NotificationType getType() const;
		ShardingMessageType messageType() const;
		unsigned getMetadataVersionId() const;
	private:
		unsigned metadataVersionId;
	};

	LockingNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId,
			LockChange * lockRequest);

	NotificationType getType() const;
	LockChange * getLockRequest() const;

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer) const;
	ShardingMessageType messageType() const;
    static LockingNotification * deserializeAndConstruct(void * buffer) const{
    	LockingNotification * newLock = new LockingNotification(0,0,0,0,NULL);
    	newLock->deserialize(buffer);
    	return newLock;
    }

private:
	LockChange * lockRequest;
};

class NodeInitNotification : public ShardingNotification {
public:

	class WELCOME : public NodeInitNotification{
	public:
		WELCOME(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId): NodeInitNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}

		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static NodeInitNotification::WELCOME * deserializeAndConstruct(void * buffer) const{
	    	NodeInitNotification::WELCOME * newNotif = new NodeInitNotification::WELCOME(0,0,0,0);
	    	newNotif->deserialize(buffer);
	    	return newNotif;
	    }
	private:

	};

	class BUSY : public NodeInitNotification{
	public:
		BUSY(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId): NodeInitNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}
		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static NodeInitNotification::BUSY * deserializeAndConstruct(void * buffer) const{
	    	NodeInitNotification::BUSY * newNotif = new NodeInitNotification::BUSY(0,0,0,0);
	    	newNotif->deserialize(buffer);
	    	return newNotif;
	    }
	private:

	};

	class NEW_HOST : public NodeInitNotification{
	public:
		NEW_HOST(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId): NodeInitNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}
		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static NodeInitNotification::NEW_HOST * deserializeAndConstruct(void * buffer) const{
	    	NodeInitNotification::NEW_HOST * newNotif = new NodeInitNotification::NEW_HOST(0,0,0,0);
	    	newNotif->deserialize(buffer);
	    	return newNotif;
	    }
	private:

	};

	class SHARD_REQUEST : public NodeInitNotification{
	public:
		SHARD_REQUEST(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId): NodeInitNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}

		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static NodeInitNotification::SHARD_REQUEST * deserializeAndConstruct(void * buffer) const{
	    	NodeInitNotification::SHARD_REQUEST * newRequest = new NodeInitNotification::SHARD_REQUEST(0,0,0,0);
	    	newRequest->deserialize(buffer);
	    	return newRequest;
	    }
	private:

	};

	class SHARD_OFFER : public NodeInitNotification{
	public:
		SHARD_OFFER(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId): NodeInitNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}

		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static NodeInitNotification::SHARD_OFFER * deserializeAndConstruct(void * buffer) const{
	    	NodeInitNotification::SHARD_OFFER * newOffer = new NodeInitNotification::SHARD_OFFER(0,0,0,0);
	    	newOffer->deserialize(buffer);
	    	return newOffer;
	    }
	private:

	};

	class SHARDS_READY : public NodeInitNotification{
	public:
		SHARDS_READY(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId): NodeInitNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}

		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static NodeInitNotification::SHARDS_READY * deserializeAndConstruct(void * buffer) const{
	    	NodeInitNotification::SHARDS_READY * newReadyNotif = new NodeInitNotification::SHARDS_READY(0,0,0,0);
	    	newReadyNotif->deserialize(buffer);
	    	return newReadyNotif;
	    }
	private:

	};


	class JOIN_PERMIT : public NodeInitNotification{
	public:
		JOIN_PERMIT(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId): NodeInitNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}

		NotificationType getType() const;
		ShardingMessageType messageType() const;
	    static NodeInitNotification::JOIN_PERMIT * deserializeAndConstruct(void * buffer) const{
	    	NodeInitNotification::JOIN_PERMIT * newReadyNotif = new NodeInitNotification::JOIN_PERMIT(0,0,0,0);
	    	newReadyNotif->deserialize(buffer);
	    	return newReadyNotif;
	    }
	private:

	};

	NodeInitNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId): ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){}

private:

};


class CopyToMeNotification : public ShardingNotification {
public:
	CopyToMeNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId,
			ShardId srcShardId,
			ShardId destShardId): ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){
		this->srcShardId = srcShardId;
		this->destShardId = destShardId;
	}

	NotificationType getType() const{
		return 	NotificationType_Sharding_CopyToMe;
	}


	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = srcShardId.serialize(buffer);
		buffer = destShardId.serialize(buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += srcShardId.getNumberOfBytes();
		numberOfBytes += destShardId.getNumberOfBytes();
		return numberOfBytes;
	}
	void * deserialize(void * buffer) const{
		buffer = ShardingNotification::deserialize(buffer);
		buffer = srcShardId.deserialize(buffer);
		buffer = destShardId.deserialize(buffer);
		return buffer;
	}
	ShardingMessageType messageType() const{
		return ShardingCopyToMeMessageType;
	}

    static CopyToMeNotification * deserializeAndConstruct(void * buffer) const{
    	CopyToMeNotification * copyToMeNotif = new CopyToMeNotification(0,0,0,0, ShardId(), ShardId());
    	copyToMeNotif->deserialize(buffer);
    	return copyToMeNotif;
    }
private:
	ShardId srcShardId;
	ShardId destShardId;
};


class MoveToMeNotification : public ShardingNotification {
public:
	MoveToMeNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId,
			ShardId srcShardId,
			ShardId destShardId): ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){
		this->srcShardId = srcShardId;
		this->destShardId = destShardId;
	}

	NotificationType getType() const{
		return 	NotificationType_Sharding_MoveToMe;
	}

	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = srcShardId.serialize(buffer);
		buffer = destShardId.serialize(buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += srcShardId.getNumberOfBytes();
		numberOfBytes += destShardId.getNumberOfBytes();
		return numberOfBytes;
	}
	void * deserialize(void * buffer) const{
		buffer = ShardingNotification::deserialize(buffer);
		buffer = srcShardId.deserialize(buffer);
		buffer = destShardId.deserialize(buffer);
		return buffer;
	}
	ShardingMessageType messageType() const{
		return ShardingMoveToMeMessageType;
	}

    static MoveToMeNotification * deserializeAndConstruct(void * buffer) const{
    	MoveToMeNotification * moveToMeNotif = new MoveToMeNotification(0,0,0,0, ShardId(), ShardId());
    	moveToMeNotif->deserialize(buffer);
    	return moveToMeNotif;
    }
private:
	ShardId srcShardId;
	ShardId destShardId;
};

}
}


#endif // __SHARDING_SHARDING_NOTIFICATION_H__
