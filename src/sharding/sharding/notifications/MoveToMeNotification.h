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


	bool resolveNotification(SP(ShardingNotification) _notif);

	bool hasResponse() const {
		return true;
	}

	void * serializeBody(void * buffer) const{
        buffer = shardId.serialize(buffer);
        return buffer;
    }
    unsigned getNumberOfBytesBody() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += shardId.getNumberOfBytes();
        return numberOfBytes;
    }
    void * deserializeBody(void * buffer){
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

		bool resolveNotification(SP(ShardingNotification) _notif);

		ShardingMessageType messageType() const{
			return ShardingMoveToMeCleanupMessageType;
		}
		void * serializeBody(void * buffer) const{
			buffer = shardId.serialize(buffer);
			return buffer;
		}
		unsigned getNumberOfBytesBody() const{
			unsigned numberOfBytes = 0;
			numberOfBytes += shardId.getNumberOfBytes();
			return numberOfBytes;
		}
		void * deserializeBody(void * buffer){
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

		bool resolveNotification(SP(ShardingNotification) _notif);
	};
};


}
}


#endif // __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__
