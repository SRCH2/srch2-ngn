#ifndef __SHARDING_SHARDING_COMMIT_NOTIFICATION_H__
#define __SHARDING_SHARDING_COMMIT_NOTIFICATION_H__


#include "Notification.h"
#include "core/util/SerializationHelper.h"
#include "../metadata_manager/ResourceMetadataChange.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class CommitNotification : public ShardingNotification{
public:

	CommitNotification(MetadataChange * metadataChange);

	CommitNotification(){
		metadataChange = NULL;
	};
	~CommitNotification(){
	}
	MetadataChange * getMetadataChange() const;

	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		ASSERT(metadataChange != NULL);
		buffer = srch2::util::serializeFixedTypes(metadataChange->getType(), buffer);
		buffer = metadataChange->serialize(buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += ShardingNotification::getNumberOfBytes();
		numberOfBytes += sizeof(MetadataChangeType);
		numberOfBytes += metadataChange->getNumberOfBytes();
		return numberOfBytes;
	}
	void * deserialize(void * buffer){
		buffer = ShardingNotification::deserialize(buffer);
		MetadataChangeType type;
		buffer = srch2::util::deserializeFixedTypes(buffer, type);
		switch (type) {
			case ShardingChangeTypeNodeAdd:
				metadataChange = new NodeAddChange();
				break;
			case ShardingChangeTypeShardAssign:
				metadataChange = new ShardAssignChange();
				break;
			case ShardingChangeTypeShardMove:
				metadataChange = new ShardMoveChange();
				break;
			case ShardingChangeTypeLoadChange:
				metadataChange = new ShardLoadChange();
				break;
			default:
				ASSERT(false);
				break;
		}
		buffer = metadataChange->deserialize(metadataChange);
		return buffer;
	}
    ShardingMessageType messageType() const;
private:
	MetadataChange * metadataChange;

public:
	class ACK : public ShardingNotification{
	public:
	    ShardingMessageType messageType() const{
	    	return ShardingCommitACKMessageType;
	    }
	};
};


}
}
#endif // __SHARDING_SHARDING_COMMIT_NOTIFICATION_H__
