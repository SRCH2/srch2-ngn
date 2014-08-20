#ifndef __SHARDING_SHARDING_COPY_TO_ME_NOTIFICATION_H__
#define __SHARDING_SHARDING_COPY_TO_ME_NOTIFICATION_H__

#include "Notification.h"
#include "../metadata_manager/Shard.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class CopyToMeNotification : public ShardingNotification {
public:
	CopyToMeNotification(const ClusterShardId & srcShardId){
		this->srcShardId = srcShardId;
	}
	CopyToMeNotification(){};


	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = srcShardId.serialize(buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += ShardingNotification::getNumberOfBytes();
		numberOfBytes += srcShardId.getNumberOfBytes();
		return numberOfBytes;
	}
	void * deserialize(void * buffer) {
		buffer = ShardingNotification::deserialize(buffer);
		buffer = srcShardId.deserialize(buffer);
		return buffer;
	}
	ShardingMessageType messageType() const{
		return ShardingCopyToMeMessageType;
	}
    ClusterShardId getSrcShardId() const{
    	return srcShardId;
    }
private:
	ClusterShardId srcShardId;
};


}
}


#endif // __SHARDING_SHARDING_COPY_TO_ME_NOTIFICATION_H__
