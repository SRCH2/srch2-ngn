#ifndef __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__
#define __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__

#include "Notification.h"
#include "../metadata_manager/Shard.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"

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


	static bool resolveMessage(Message * msg, NodeId sendeNode);

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
	class CleanUp : public ShardingNotification {
	public:
		CleanUp(const ClusterShardId & shardId){
			this->shardId = shardId;
		}
		CleanUp(){};

		static bool resolveMessage(Message * msg, NodeId sendeNode);

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

		bool operator==(const MoveToMeNotification::CleanUp & right){
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

		static bool resolveMessage(Message * msg, NodeId sendeNode);
	};
	class FINISH : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingMoveToMeFinishMessageType;
		}
		static bool resolveMessage(Message * msg, NodeId sendeNode);
	};
	class ABORT : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingMoveToMeAbortMessageType;
		}
		static bool resolveMessage(Message * msg, NodeId sendeNode);
	};
};


}
}


#endif // __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__
