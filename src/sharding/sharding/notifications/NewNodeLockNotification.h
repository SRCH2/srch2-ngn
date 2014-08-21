#ifndef __SHARDING_SHARDING_NEW_NODE_LOCK_NOTIFICATION_H__
#define __SHARDING_SHARDING_NEW_NODE_LOCK_NOTIFICATION_H__

#include "../notifications/Notification.h"
#include "../metadata_manager/ResourceLocks.h"
#include "core/util/SerializationHelper.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class NewNodeLockNotification: public ShardingNotification{
public:
	NewNodeLockNotification(const vector<NodeId> & allNodesUpToCurrentNode,
			ResourceLockRequest * lockRequest){
		this->allNodesUpToNewNode = allNodesUpToCurrentNode;
		this->lockRequest = lockRequest;
	};
	NewNodeLockNotification(){};
	~NewNodeLockNotification(){
	};

	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = srch2::util::serializeVectorOfFixedTypes(allNodesUpToNewNode, buffer);
		buffer = lockRequest->serialize(buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += ShardingNotification::getNumberOfBytes();
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(allNodesUpToNewNode);
		numberOfBytes += lockRequest->getNumberOfBytes();
		return numberOfBytes;
	}
	void * deserialize(void * buffer){
		buffer = ShardingNotification::deserialize(buffer);
		buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, allNodesUpToNewNode);
		lockRequest = new ResourceLockRequest();
		buffer = lockRequest->deserialize(buffer);
		return buffer;
	}
    ShardingMessageType messageType() const{
    	return ShardingNewNodeLockMessageType;
    }
	vector<NodeId> getNewNodeClusterView() const{
		return allNodesUpToNewNode;
	}

	ResourceLockRequest * getLockRequest() const{
		return this->lockRequest;
	}
private:
	vector<NodeId> allNodesUpToNewNode; // without the new one itself
	ResourceLockRequest * lockRequest;

public:
	/// Sub classes ...
	class ACK : public ShardingNotification{
	public:
		ACK(LockHoldersRepository * shardLockRepository){
			ASSERT(shardLockRepository != NULL);
			// These objects must not get deleted in destructor of here ...
			this->shardLockRepository = shardLockRepository;
		};
		ACK(){};
		void * serialize(void * buffer) const{
			buffer = ShardingNotification::serialize(buffer);
			buffer = shardLockRepository->serialize(buffer);
			return buffer;
		}
		unsigned getNumberOfBytes() const{
			unsigned numberOfBytes = 0;
			numberOfBytes += ShardingNotification::getNumberOfBytes();
			numberOfBytes += shardLockRepository->getNumberOfBytes();
			return numberOfBytes;
		}
		void * deserialize(void * buffer){
			buffer = ShardingNotification::deserialize(buffer);
			shardLockRepository = new LockHoldersRepository();
			buffer = shardLockRepository->deserialize(buffer);
			return buffer;
		}
	    ShardingMessageType messageType() const{
	    	return ShardingNewNodeLockACKMessageType;
	    }
		LockHoldersRepository * getShardLockRepository() const{
			return shardLockRepository;
		}
	private:
	    LockHoldersRepository * shardLockRepository;
	};

};

}
}

#endif // __SHARDING_SHARDING_NEW_NODE_LOCK_NOTIFICATION_H__
