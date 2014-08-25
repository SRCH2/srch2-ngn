#ifndef __SHARDING_SHARDING_NOTIFICATION_H__
#define __SHARDING_SHARDING_NOTIFICATION_H__

#include "core/util/Assert.h"

#include "sharding/configuration/ShardingConstants.h"
#include "sharding/transport/Message.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

struct NodeOperationId{
	NodeId nodeId;
	unsigned operationId;
	NodeOperationId();
	NodeOperationId(const NodeOperationId & id);
	NodeOperationId(NodeId nodeId, unsigned operationId = 0);
	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	bool operator==(const NodeOperationId & right) const;
	bool operator>(const NodeOperationId & right) const;
	bool operator<(const NodeOperationId & right) const;
	string toString() const;
};


class Notification{
public:
    virtual ShardingMessageType messageType() const = 0;
    virtual ~Notification(){};
};


// TEMP
enum MIGRATION_STATUS{
	MIGRATION_STATUS_FAIL,
	MIGRATION_STATUS_FINISH
};
struct ShardMigrationStatus{
	ShardMigrationStatus(){};
	ShardMigrationStatus(const ShardMigrationStatus & status){
		this->srcOperationId = status.srcOperationId;
		this->dstOperationId = status.dstOperationId;
		this->sourceNodeId = status.sourceNodeId;
		this->destinationNodeId = status.destinationNodeId;
		this->shard = status.shard;
		this->status = status.status;

	};
	ShardMigrationStatus operator=(const ShardMigrationStatus & status){
		ShardMigrationStatus left;
		left.srcOperationId = status.srcOperationId;
		left.dstOperationId = status.dstOperationId;
		left.sourceNodeId = status.sourceNodeId;
		left.destinationNodeId = status.destinationNodeId;
		left.shard = status.shard;
		left.status = status.status;
		return left;
	}
	unsigned srcOperationId;    // #1
    unsigned dstOperationId;   // #7
	NodeId sourceNodeId;   // NodeA
	NodeId destinationNodeId;   // Current Node
	boost::shared_ptr<Srch2Server> shard;
	MIGRATION_STATUS status;
};

class ShardingNotification : public Notification{
public:
	ShardingNotification();
	virtual ~ShardingNotification(){};
	NodeOperationId getSrc() const;
	NodeOperationId getDest() const;
	void setSrc(const NodeOperationId & src) ;
	void setDest(const NodeOperationId & dest) ;
	void setBounced();
	void resetBounced();
	bool isBounced() const;

	virtual void * serialize(void * buffer) const;
	virtual unsigned getNumberOfBytes() const;
	virtual void * deserialize(void * buffer) ;
	Message * serialize(MessageAllocator * allocator) const{
		unsigned numberOfBytes = getNumberOfBytes();
		Message * msg = allocator->allocateMessage(numberOfBytes);
		void * bufferWritePointer = Message::getBodyPointerFromMessagePointer(msg);
		bufferWritePointer = serialize(bufferWritePointer);
		return msg;
	}
	template<class TYPE>
    static TYPE * deserializeAndConstruct(void * buffer){
    	TYPE * notification = new TYPE();
    	notification->deserialize(buffer);
    	return notification;
    }
	void swapSrcDest(){
		NodeOperationId temp = srcOperationId;
		srcOperationId = destOperationId;
		destOperationId = temp;
	}

private:
    NodeOperationId srcOperationId;
    NodeOperationId destOperationId;
    bool bounced;
};


class NodeFailureNotification : public Notification{
public:
	NodeFailureNotification(const NodeId & failedNodeId):failedNodeId(failedNodeId){
	}
	NodeId getFailedNodeID() const{
		return this->failedNodeId;
	}
    ShardingMessageType messageType() const {
    	return ShardingNodeFailureNotificationMessageType;
    }
private:
	const NodeId failedNodeId;
};


class MMNotification : public ShardingNotification{
public:

    // TODO : second argument must be removed when migration manager is merged with shard manager codes ...
	MMNotification(const ShardMigrationStatus & status, const ClusterShardId & destShardId):status(status){
		this->setSrc(NodeOperationId(this->status.sourceNodeId, this->status.srcOperationId));
		this->setDest(NodeOperationId(this->status.destinationNodeId, this->status.dstOperationId));
		this->destShardId = destShardId;
	}
	MMNotification(){};
	ShardMigrationStatus getStatus() const{
		return this->status;
	}
	void setStatus(const ShardMigrationStatus & status){
		this->status = status;
	}
    ShardingMessageType messageType() const {
    	return ShardingMMNotificationMessageType;
    }
    ClusterShardId getDestShardId() const{
        return this->destShardId;
    }
private:
	ShardMigrationStatus status;
	ClusterShardId destShardId;

};

}
}


#endif // __SHARDING_SHARDING_NOTIFICATION_H__
