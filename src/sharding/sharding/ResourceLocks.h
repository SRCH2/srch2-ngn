#ifndef __SHARDING_SHARDING_RESOURCE_LOCKS_H__
#define __SHARDING_SHARDING_RESOURCE_LOCKS_H__

#include "Resources.h"
#include "Notification.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {



class ClusterResourceLocks{
public:
	// pid => list of (nodeId, operationId)
	map<unsigned, vector< NodeOperationId > > sLocks;
	map<unsigned, vector< NodeOperationId > > xLocks;
};

class LockChange : public ShardingChange{
public:
	LockChange(const unsigned coreId, const unsigned pid, const bool acquireOrRelease ,
			const bool sharedOrExclusive, const vector<NodeOperationId> & lockHolders);
	LockChange();
	LockChange(const LockChange & change);


	bool apply(ClusterResourceLocks * locks, map<ShardId, bool> & needToChangeMetadata);

	unsigned getPID() const;
	bool isSharedOrExclusive() const;
	bool isAcquireOrRelease() const;
	void setAcquireOrRelease(bool acOrRel);
	vector<NodeOperationId> * getLockRequesters() const;

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	ShardingChange * clone(){
		return new LockChange(*this);
	}

private:
	unsigned coreId;
	unsigned pid;
	bool acquireOrRelease ; // true means acquire request, false is release
	bool sharedOrExclusive; // true means S, false means X
	vector<NodeOperationId> lockRequesters;
	bool acquire(ClusterResourceLocks * locks, map<ShardId, bool> & needToChangeMetadata);
	void release(ClusterResourceLocks * locks, map<ShardId, bool> & needToChangeMetadata);
};


}
}

#endif // __SHARDING_SHARDING_RESOURCE_LOCKS_H__
