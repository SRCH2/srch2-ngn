#ifndef __SHARDING_SHARDING_COPY_TO_ME_NOTIFICATION_H__
#define __SHARDING_SHARDING_COPY_TO_ME_NOTIFICATION_H__

#include "Notification.h"
#include "../metadata_manager/Shard.h"
#include "sharding/transport/Message.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class CopyToMeNotification : public ShardingNotification {
public:
	CopyToMeNotification(const ClusterShardId & srcShardId, const ClusterShardId & destShardId){
		this->replicaShardId = srcShardId;
		this->unassignedShardId = destShardId;
	}
    CopyToMeNotification(const ClusterShardId & srcShardId){
        this->replicaShardId = srcShardId;
    }
	CopyToMeNotification(){};

	static bool resolveMessage(Message * msg, NodeId sendeNode);

	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = replicaShardId.serialize(buffer);
        buffer = unassignedShardId.serialize(buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += ShardingNotification::getNumberOfBytes();
		numberOfBytes += replicaShardId.getNumberOfBytes();
        numberOfBytes += unassignedShardId.getNumberOfBytes();
		return numberOfBytes;
	}
	void * deserialize(void * buffer) {
		buffer = ShardingNotification::deserialize(buffer);
		buffer = replicaShardId.deserialize(buffer);
        buffer = unassignedShardId.deserialize(buffer);
		return buffer;
	}
	ShardingMessageType messageType() const{
		return ShardingCopyToMeMessageType;
	}
    ClusterShardId getReplicaShardId() const{
    	return replicaShardId;
    }
    ClusterShardId getUnassignedShardId() const{
        return unassignedShardId;
    }
	bool operator==(const CopyToMeNotification & right){
		return replicaShardId == right.replicaShardId && unassignedShardId == right.unassignedShardId;
	}
private:
	ClusterShardId replicaShardId;
	ClusterShardId unassignedShardId;
};


}
}


#endif // __SHARDING_SHARDING_COPY_TO_ME_NOTIFICATION_H__
