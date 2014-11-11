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

	bool resolveNotification(SP(ShardingNotification) _notif);

	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer) ;
	ShardingMessageType messageType() const;
    ClusterShardId getReplicaShardId() const;
    ClusterShardId getUnassignedShardId() const;
	bool operator==(const CopyToMeNotification & right);
    bool hasResponse() const {
			return true;
	}
private:
	ClusterShardId replicaShardId;
	ClusterShardId unassignedShardId;


public:
	class ACK : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingCopyToMeACKMessageType;
		}

		bool resolveNotification(SP(ShardingNotification) _notif);
	};
};


}
}


#endif // __SHARDING_SHARDING_COPY_TO_ME_NOTIFICATION_H__
