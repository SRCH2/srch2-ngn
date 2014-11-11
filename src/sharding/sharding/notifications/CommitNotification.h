#ifndef __SHARDING_SHARDING_COMMIT_NOTIFICATION_H__
#define __SHARDING_SHARDING_COMMIT_NOTIFICATION_H__


#include "Notification.h"
#include "../metadata_manager/ResourceMetadataChange.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class CommitNotification : public ShardingNotification{
public:

	CommitNotification(MetadataChange * metadataChange);

	CommitNotification();
	~CommitNotification();

	bool resolveNotification(SP(ShardingNotification) notif);
    bool hasResponse() const {
			return true;
	}
	MetadataChange * getMetadataChange() const;

	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer);
    ShardingMessageType messageType() const;

    bool operator==(const CommitNotification & right);
private:
	MetadataChange * metadataChange;

public:
	class ACK : public ShardingNotification{
	public:
	    ShardingMessageType messageType() const{
	    	return ShardingCommitACKMessageType;
	    }
	    bool operator==(const CommitNotification::ACK & right);

		bool resolveNotification(SP(ShardingNotification) _notif);
	};
};


}
}
#endif // __SHARDING_SHARDING_COMMIT_NOTIFICATION_H__
