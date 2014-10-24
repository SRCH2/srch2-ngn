#ifndef __SHARDING_LOCK_MANAGER_LOCK_BATCH_H__
#define __SHARDING_LOCK_MANAGER_LOCK_BATCH_H__

#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../../configuration/ShardingConstants.h"

#include <sstream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

template <class Resource>
class ItemLockHolder{
public:
	bool lock(const Resource & resource, const vector<NodeOperationId> & opids , const LockLevel & lockLevel){
		if(opids.empty()){
			return false;
		}
		if(grantedLocks.find(resource) == grantedLocks.end()){
			grantedLocks[resource] = vector<pair<NodeOperationId, LockLevel> >();
		}
		if(grantedLocks[resource].size() == 0){
			for(unsigned i = 0; i < opids.size(); ++i){
				grantedLocks[resource].push_back(std::make_pair(opids.at(i), lockLevel));
			}
			return true;
		}
		if(! conflict(grantedLocks[resource].at(0).second, lockLevel) ){
			for(unsigned i = 0; i < opids.size(); ++i){
				grantedLocks[resource].push_back(std::make_pair(opids.at(i), lockLevel));
			}
			return true;
		}
		return false;
	}

	bool canLock(const Resource & resource, const LockLevel & lockLevel){
		if(grantedLocks.find(resource) == grantedLocks.end()){
			grantedLocks[resource] = vector<pair<NodeOperationId, LockLevel> >();
		}
		if(grantedLocks[resource].size() == 0){
			return true;
		}
		if(! conflict(grantedLocks[resource].at(0).second, lockLevel) ){
			return true;
		}
		return false;
	}

	bool release(const Resource & resource, const NodeOperationId & opid ){
		if(grantedLocks.find(resource) == grantedLocks.end()){
			return false;
		}
		bool releaseHappened = false;
		for(vector<pair<NodeOperationId, LockLevel> >::iterator resItr = grantedLocks[resource].begin();
				resItr != grantedLocks[resource].end(); ++resItr){
			if(resItr->first == opid){
				resItr = grantedLocks[resource].erase(resItr);
				releaseHappened = true;
			}
		}
		if(grantedLocks[resource].size() == 0){
			grantedLocks.erase(resource);
		}
		return releaseHappened;
	}

	void clear(){
		grantedLocks.clear();
	}

	void release(const NodeId & failedNodeId){

		vector<Resource> resourceToDelete;

		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::iterator resItr = grantedLocks.begin();
				resItr != grantedLocks.end(); ++resItr){
			for(vector<pair<NodeOperationId, LockLevel> >::iterator nodeItr = resItr->second.begin();
					nodeItr != resItr->second.end(); ++nodeItr){
				if(nodeItr->first.nodeId == failedNodeId){
					nodeItr = resItr->second.erase(nodeItr);
				}
			}
			if(resItr->second.size() == 0){
				resourceToDelete.push_back(resItr->first);
			}
		}
		for(unsigned i = 0 ; i < resourceToDelete.size(); ++i){
			grantedLocks.erase(resourceToDelete.at(i));
		}
	}
	bool isLock(const Resource & resource){
		if(grantedLocks.find(resource) == grantedLocks.end()){
			return false;
		}
		if(grantedLocks[resource].size() == 0){
			return false;
		}
		return true;
	}

	void getAllLockedResource(vector<Resource> & allResources) const{
		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::const_iterator
				resItr = grantedLocks.begin(); resItr != grantedLocks.end(); ++resItr){
			allResources.push_back(resItr->first);
		}
	}

	void * serialize(void * buffer) const{
		// serialize the size of the main map
		buffer = srch2::util::serializeFixedTypes((unsigned)(grantedLocks.size()), buffer);
		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::const_iterator
				resItr = grantedLocks.begin(); resItr != grantedLocks.end(); ++resItr){
			buffer = serialize(resItr->first, buffer);
			// serialize size of vector
			buffer = srch2::util::serializeFixedTypes((unsigned)(resItr->second.size()), buffer);
			for(unsigned i = 0 ; i < resItr->second.size(); i++){
				buffer = resItr->second.at(i).first.serialize(buffer);
				buffer = srch2::util::serializeFixedTypes(resItr->second.at(i).second, buffer);
			}
		}
		return buffer;
	}
	void * deserialize(void * buffer){
		unsigned sizeOfMap = 0;
		buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfMap);
		for(unsigned r = 0 ; r < sizeOfMap ; ++r){
			Resource res;
			buffer = deserialize(buffer, res);
			vector<pair<NodeOperationId, LockLevel> > keyVector;
			grantedLocks[res] = keyVector;
			unsigned sizeOfVector = 0;
			buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfVector);
			for(unsigned i = 0 ; i < sizeOfVector; ++i){
				NodeOperationId id;
				buffer = id.deserialize(buffer);
				LockLevel ll;
				buffer = srch2::util::deserializeFixedTypes(buffer, ll);
				grantedLocks[res].push_back(std::make_pair(id,ll));
			}
		}
		return buffer;
	}

	unsigned getNumberOfBytes(){
		unsigned numberOfBytes = 0 ;
		numberOfBytes += sizeof(unsigned);
		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::const_iterator
				resItr = grantedLocks.begin(); resItr != grantedLocks.end(); ++resItr){
			numberOfBytes += getNumberOfBytes(resItr->first);
			// serialize size of vector
			numberOfBytes += sizeof(unsigned);
			for(unsigned i = 0 ; i < resItr->second.size(); i++){
				numberOfBytes += resItr->second.at(i).first.getNumberOfBytes();
				numberOfBytes += resItr->second.at(i).second.size() + sizeof(unsigned);
			}
		}
		return numberOfBytes;
	}

private:
	map<Resource, vector<pair<NodeOperationId, LockLevel> > > grantedLocks;
	bool conflict(const LockLevel & level1, const LockLevel & level2){
		if(level1 == LockLevel_X){
			return true;
		}
		if(level2 == LockLevel_X){
			return true;
		}
		return false;
	}

	void * serialize(const string & str, void * buffer) const{
		buffer = srch2::util::serializeString(str, buffer);
		return buffer;
	}
	void * serialize(const ClusterShardId & shardId, void * buffer) const{
		buffer = shardId.serialize(buffer);
		return buffer;
	}
	void * deserialize(void * buffer, string & str) {
		buffer = srch2::util::deserializeString(buffer, str);
		return buffer;
	}
	void * deserialize(void * buffer, ClusterShardId & shardId){
		buffer = shardId.deserialize(buffer);
		return buffer;
	}
	unsigned getNumberOfBytes(const string & str) const{
		return str.size() + sizeof(unsigned);
	}
	unsigned getNumberOfBytes(const ClusterShardId & shardId) const{
		return shardId.getNumberOfBytes();
	}
};


class LockBatch{
public:
	~LockBatch();
	bool blocking;
	bool release;
	bool incremental;
	bool granted;
	int lastGrantedItemIndex;
	LockRequestType batchType;
	vector<NodeOperationId> opIds;
	SP(LockingNotification::ACK) ack;

	unsigned versionId;


	// for all cluster shard related locks
	vector<pair<ClusterShardId, LockLevel> > tokens;

	// for primary key related locks
	vector<pair<string, LockLevel> > pkTokens;
	ClusterPID pid; // the partition id of these primary keys

	// for metadata locks
	LockLevel metadataLockLevel;
	vector<NodeId> olderNodes;

	bool isReadviewPending() const;

	bool update(const NodeId & failedNode);

	static LockBatch * generateLockBatch(SP(LockingNotification) notif);
	static LockBatch * generateLockBatch(const ClusterShardId & shardId, const LockLevel & lockLevel);

private:
	// so that only our static class makes these objects
	LockBatch();

};


}
}

#endif // __SHARDING_LOCK_MANAGER_LOCK_BATCH_H__
