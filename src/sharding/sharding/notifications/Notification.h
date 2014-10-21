#ifndef __SHARDING_SHARDING_NOTIFICATION_H__
#define __SHARDING_SHARDING_NOTIFICATION_H__


#include "sharding/configuration/ShardingConstants.h"
#include "sharding/transport/Message.h"
#include "sharding/transport/MessageAllocator.h"
#include "migration/MigrationManager.h"
#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class Notification{
public:
    virtual ShardingMessageType messageType() const = 0;
    virtual ~Notification(){};
};


//// TEMP
//enum MIGRATION_STATUS{
//	MIGRATION_STATUS_FAIL,
//	MIGRATION_STATUS_FINISH
//};
//struct ShardMigrationStatus{
//	ShardMigrationStatus(){};
//	ShardMigrationStatus(const ShardMigrationStatus & status);
//	ShardMigrationStatus & operator=(const ShardMigrationStatus & status);
//
//    void * serialize(void * buffer) const;
//    unsigned getNumberOfBytes() const;
//    void * deserialize(void * buffer) ;
//
//	unsigned srcOperationId;    // #1
//    unsigned dstOperationId;   // #7
//	NodeId sourceNodeId;   // NodeA
//	NodeId destinationNodeId;   // Current Node
//	boost::shared_ptr<Srch2Server> shard;
//	MIGRATION_STATUS status;
//};

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

	string getDescription(){
		stringstream ss;
		ss << "(" << srcOperationId.toString() << " => " << destOperationId.toString();
		if(bounced){
			ss << ", bounced)";
		}else{
			ss << ")";
		}
		return ss.str();
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

// NOTE : the reason MigrationManager status report is
//        a child of ShardingNotification is that we can
//        use the API of this class to keep src and dest of
//        this "data transfer"
class MMNotification : public ShardingNotification{
public:

	MMNotification(const ShardMigrationStatus & status);
	MMNotification();
	ShardMigrationStatus getStatus() const;
	void setStatus(const ShardMigrationStatus & status);
    ShardingMessageType messageType() const ;

    void * serialize(void * buffer) const;
    unsigned getNumberOfBytes() const;
    void * deserialize(void * buffer) ;

private:
	ShardMigrationStatus status;

};



class SaveDataNotification : public ShardingNotification {
public:
	SaveDataNotification(const NodeTargetShardInfo & info){
		targets = info;
	}
	ShardingMessageType messageType() const{
		return ShardingSaveDataMessageType;
	}
	static bool resolveMessage(Message * msg, NodeId sendeNode);
    void * serialize(void * buffer) const;
    unsigned getNumberOfBytes() const;
    void * deserialize(void * buffer) ;
    NodeTargetShardInfo & getTargets(){return targets;};
private:
	NodeTargetShardInfo targets;
}; // ACK is StatusMessageType



class SaveMetadataNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const{
		return ShardingSaveMetadataMessageType;
	}
	static bool resolveMessage(Message * msg, NodeId sendeNode);
}; // ACK is StatusMessageType

class MergeNotification : public ShardingNotification {
public:
	MergeNotification(const MergeOperationType & opType = MergeOperationType_Merge,
			const vector<ClusterShardId> & clusterShards = vector<ClusterShardId>(),
			const vector<NodeShardId> & nodeShards = vector<NodeShardId>()):
					operationType(opType){
		this->clusterShardIds = clusterShards;
		this->nodeShardIds = nodeShards;
	}
	ShardingMessageType messageType() const{
		return ShardingMergeMessageType;
	}
	static bool resolveMessage(Message * msg, NodeId sendeNode);
	MergeOperationType getMergeOperationType() const;
    void * serialize(void * buffer) const;
    unsigned getNumberOfBytes() const;
    void * deserialize(void * buffer) ;
private:
	const MergeOperationType operationType;

	// if merge flag is going to be set ON/OFF
	// we need a list of shards for the node that this notification goes to
	vector<ClusterShardId> clusterShardIds;
	vector<NodeShardId> nodeShardIds;
public:
	class ACK : public ShardingNotification{
	public:
		ShardingMessageType messageType() const{
			return ShardingMergeACKMessageType;
		}
		static bool resolveMessage(Message * msg, NodeId sendeNode);
	};
};


class ShutdownNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const {
		return ShardingShutdownMessageType;
	}
	static bool resolveMessage(Message * msg, NodeId sendeNode);
};

}
}


#endif // __SHARDING_SHARDING_NOTIFICATION_H__
