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
	ShardingMessageType messageType() const{
		return ShardingSaveDataMessageType;
	}

public:

	class ACK : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingSaveDataACKMessageType;
		}
	};

};



class SaveMetadataNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const{
		return ShardingSaveMetadataMessageType;
	}

public:

	class ACK : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingSaveMetadataACKMessageType;
		}
	};

};

class MergeNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const{
		return ShardingMergeMessageType;
	}
public:
	class ACK : public ShardingNotification{
	public:
		ShardingMessageType messageType() const{
			return ShardingMergeACKMessageType;
		}
	};
};


class ShutdownNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const {
		return ShardingShutdownMessageType;
	}
};

}
}


#endif // __SHARDING_SHARDING_NOTIFICATION_H__
