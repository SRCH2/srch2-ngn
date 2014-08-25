#ifndef __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__
#define __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__

#include "../metadata_manager/Shard.h"
#include "Notification.h"
#include "core/util/SerializationHelper.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class MoveToMeNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const{
		return ShardingMoveToMeMessageType;
	}
	MoveToMeNotification(const ClusterShardId & shardId){
        this->shardId = shardId;
    }
	MoveToMeNotification(){};

	void * serialize(void * buffer) const{
        buffer = ShardingNotification::serialize(buffer);
        buffer = shardId.serialize(buffer);
        return buffer;
    }
    unsigned getNumberOfBytes() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += ShardingNotification::getNumberOfBytes();
        numberOfBytes += shardId.getNumberOfBytes();
        return numberOfBytes;
    }
    void * deserialize(void * buffer){
        buffer = ShardingNotification::deserialize(buffer);
        buffer = shardId.deserialize(buffer);
        return buffer;
    }

    bool operator==(const MoveToMeNotification & right){
        return shardId == right.shardId;
    }

    ClusterShardId getShardId() const{
        return this->shardId;
    }
private:
    ClusterShardId shardId;

public:
	class START : public ShardingNotification {
	public:
		START(const ClusterShardId & shardId){
			this->shardId = shardId;
		}
		START(){};
		ShardingMessageType messageType() const{
			return ShardingMoveToMeStartMessageType;
		}
		void * serialize(void * buffer) const{
			buffer = ShardingNotification::serialize(buffer);
			buffer = shardId.serialize(buffer);
			return buffer;
		}
		unsigned getNumberOfBytes() const{
			unsigned numberOfBytes = 0;
			numberOfBytes += ShardingNotification::getNumberOfBytes();
			numberOfBytes += shardId.getNumberOfBytes();
			return numberOfBytes;
		}
		void * deserialize(void * buffer){
			buffer = ShardingNotification::deserialize(buffer);
			buffer = shardId.deserialize(buffer);
			return buffer;
		}

		bool operator==(const MoveToMeNotification::START & right){
			return shardId == right.shardId;
		}

		ClusterShardId getShardId() const{
			return this->shardId;
		}
	private:
		ClusterShardId shardId;
	};
	class ACK : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingMoveToMeACKMessageType;
		}
	};
	class FINISH : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingMoveToMeFinishMessageType;
		}
	};
	class ABORT : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingMoveToMeAbortMessageType;
		}
	};
};


}
}


#endif // __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__
