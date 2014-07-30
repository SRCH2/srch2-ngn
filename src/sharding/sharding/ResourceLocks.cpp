
#include "ResourceLocks.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


LockChange::LockChange(const unsigned coreId, const unsigned pid, const bool acquireOrRelease ,
		const bool sharedOrExclusive, const vector<NodeOperationId> & lockHolders):coreId(coreId),
		pid(pid), acquireOrRelease(acquireOrRelease), sharedOrExclusive(sharedOrExclusive), lockRequesters(lockHolders){
}
LockChange::LockChange():LockChange(0,0,false,false,vector<NodeOperationId>()){
	// temp init for deserialization;
}
LockChange::LockChange(const LockChange & change):
	LockChange(change.coreId, change.pid, change.acquireOrRelease, change.sharedOrExclusive, change.lockRequesters){
}

bool LockChange::apply(ClusterResourceLocks * locks, map<ShardId, bool> & needToChangeMetadata){
	if(isAcquireOrRelease()){
		return acquire(locks, needToChangeMetadata);
	}else{
		return release(locks, needToChangeMetadata);
	}
}

bool LockChange::acquire(ClusterResourceLocks * locks, map<ShardId, bool> & needToChangeMetadata){

	if(this->sharedOrExclusive){ // S lock
		// Condition : Nobody must have any X lock on this PID
		if(locks->xLocks.find(pid) != locks->xLocks.end() && locks->xLocks[pid].size() != 0){
			return false;
		}
		if(locks->sLocks.find(pid) == locks->sLocks.end() || locks->sLocks[pid].size() == 0){
			needToChangeMetadata[ShardId(coreId, pid, 0)] = true;
		}
		if(locks->sLocks.find(pid) == locks->sLocks.end()){
			locks->sLocks[pid] = lockRequesters;
		}else{
			locks->sLocks[pid].insert(locks->sLocks[pid].begin(), lockRequesters.begin(), lockRequesters.end());
		}
		return true;

	}else{ // X lock
		// Condition : nobody must have any S or X locks on pid
		if(locks->xLocks.find(pid) != locks->xLocks.end() && locks->xLocks[pid].size() != 0){
			return false;
		}
		if(locks->sLocks.find(pid) != locks->sLocks.end() && locks->sLocks[pid].size() != 0){
			return false;
		}
		needToChangeMetadata[ShardId(coreId, pid, 0)] = true;
		if(locks->xLocks.find(pid) == locks->xLocks.end()){
			locks->xLocks[pid] = lockRequesters;
		}else{
			locks->xLocks[pid].insert(locks->xLocks[pid].begin(), lockRequesters.begin(), lockRequesters.end());
		}
		return true;
	}
}

void LockChange::release(ClusterResourceLocks * locks,map<ShardId, bool> & needToChangeMetadata){
	// lock holders that are not there will be ignored
	if(this->sharedOrExclusive){ // S lock
		if(locks->sLocks.find(pid) == locks->sLocks.end()){
			return;
		}
		vector< NodeOperationId > & sLocks = locks->sLocks.find(pid)->second;
		vector< NodeOperationId > newSLocks;
		// move on existing locks and only keep those that are not present in release list
		for(vector<NodeOperationId>::iterator lockHolderPtr = sLocks.begin();
				lockHolderPtr != sLocks.end(); ++lockHolderPtr){
			bool isInReleaseList = false;
			for(vector<NodeOperationId>::iterator lockHolderPtrToRelease = lockRequesters.begin();
					lockHolderPtrToRelease != lockRequesters.end(); ++lockHolderPtrToRelease){
				if(*lockHolderPtr == *lockHolderPtrToRelease){
					isInReleaseList = true;
					needToChangeMetadata[ShardId(coreId, pid, 0)] = false;
					break;
				}
			}
			if(! isInReleaseList){
				newSLocks.push_back(*lockHolderPtr);
			}
		}
		locks->sLocks.find(pid)->second = newSLocks;
	}else{ // X lock
		if(locks->xLocks.find(pid) == locks->xLocks.end()){
			return;
		}
		vector< NodeOperationId > & xLocks = locks->xLocks.find(pid)->second;
		vector< NodeOperationId > newXLocks;
		// move on existing locks and only keep those that are not present in release list
		for(vector<NodeOperationId>::iterator lockHolderPtr = xLocks.begin();
				lockHolderPtr != xLocks.end(); ++lockHolderPtr){
			bool isInReleaseList = false;
			for(vector<NodeOperationId>::iterator lockHolderPtrToRelease = lockRequesters.begin();
					lockHolderPtrToRelease != lockRequesters.end(); ++lockHolderPtrToRelease){
				if(*lockHolderPtr == *lockHolderPtrToRelease){
					isInReleaseList = true;
					needToChangeMetadata[ShardId(coreId, pid, 0)] = false;
					break;
				}
			}
			if(! isInReleaseList){
				newXLocks.push_back(*lockHolderPtr);
			}
		}
		locks->xLocks.find(pid)->second = newXLocks;
	}
}

unsigned LockChange::getPID() const{
	return pid;
}
bool LockChange::isSharedOrExclusive() const{
	return sharedOrExclusive;
}
bool LockChange::isAcquireOrRelease() const{
	return acquireOrRelease;
}
void LockChange::setAcquireOrRelease(bool acOrRel){
	this->acquireOrRelease = acOrRel;
}

vector<NodeOperationId> * LockChange::getLockRequesters() const{
	return &(this->lockRequesters);
}

void * LockChange::serialize(void * buffer) const{
	buffer = ShardingChange::serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(coreId, buffer);
	buffer = srch2::util::serializeFixedTypes(pid, buffer);
	buffer = srch2::util::serializeFixedTypes(acquireOrRelease, buffer);
	buffer = srch2::util::serializeFixedTypes(sharedOrExclusive, buffer);
	buffer = srch2::util::serializeFixedTypes((unsigned)lockRequesters.size(), buffer);
	for(unsigned i = 0; i < lockRequesters.size(); ++i){
		buffer = lockRequesters.at(i).serialize(buffer);
	}
	return buffer;
}
unsigned LockChange::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += ShardingChange::getNumberOfBytes();
	numberOfBytes += sizeof(unsigned);//coreId
	numberOfBytes += sizeof(unsigned);//pid
	numberOfBytes += sizeof(bool);//acquireorRelease
	numberOfBytes += sizeof(bool);//S or X
	numberOfBytes += sizeof(unsigned);// size of lockRequesters
	for(unsigned i = 0; i < lockRequesters.size(); ++i){
		numberOfBytes += lockRequesters.at(i).getNumberOfBytes();
	}
	return numberOfBytes;
}
void * LockChange::deserialize(void * buffer){
	buffer = ShardingChange::deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
	buffer = srch2::util::deserializeFixedTypes(buffer, pid);
	buffer = srch2::util::deserializeFixedTypes(buffer, acquireOrRelease);
	buffer = srch2::util::deserializeFixedTypes(buffer, sharedOrExclusive);
	unsigned sizeOfLockRequesters;
	buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfLockRequesters);
	for(unsigned i = 0; i < sizeOfLockRequesters; ++i){
		NodeOperationId requester;
		buffer = requester.deserialize(buffer);
		lockRequesters.push_back(requester);
	}
	return buffer;
}


}
}
