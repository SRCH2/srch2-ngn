#ifndef __SHARDING_SHARDING_LOCKING_NOTIFICATION_H__
#define __SHARDING_SHARDING_LOCKING_NOTIFICATION_H__

#include "Notification.h"
#include "sharding/configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {
class ResourceLockRequest;
class LockingNotification : public ShardingNotification{
public:

	LockingNotification(ResourceLockRequest * lq);

	LockingNotification();


	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	ShardingMessageType messageType() const;
    ResourceLockRequest * getLockRequest() const ;
    bool isReleaseOfNodeInitialization() const;
private:
	// list of lock requests that we want to be executed in one batch.
	// the order of these lock requests must be based on a general ordering on
	// resource identifier.
	ResourceLockRequest * lockRequest;

	//////////////////////// Sub Classes ///////////////////////////
public:


	class ACK : public ShardingNotification{
	public:
		ACK(){};
		ACK(bool grantedFlag);
		ShardingMessageType messageType() const;
		void * serialize(void * buffer) const;
		unsigned getNumberOfBytes() const;
		void * deserialize(void * buffer);
		bool isGranted() const;

	private:
	    bool grantedFlag;
	};

	class RV_RELEASED : public Notification{
	public:
		RV_RELEASED(unsigned metadataVersionId); // this notification is not going to be sent to the network
		ShardingMessageType messageType() const;
		unsigned getMetadataVersionId() const;
	private:
		unsigned metadataVersionId;
	};
};




}
}

#endif // __SHARDING_SHARDING_LOCKING_NOTIFICATION_H__
