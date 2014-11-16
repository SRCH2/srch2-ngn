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
				resItr != grantedLocks[resource].end();){
			if(resItr->first == opid){
				resItr = grantedLocks[resource].erase(resItr);
				releaseHappened = true;
			}else{
				++resItr;
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
			for(vector<pair<NodeOperationId, LockLevel> >::iterator nodeItr = resItr->second.begin(); nodeItr != resItr->second.end(); ){
				if(nodeItr->first.nodeId == failedNodeId){
					nodeItr = resItr->second.erase(nodeItr);
				}else{
					++nodeItr;
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
			if(resItr->second.size() > 0){
				allResources.push_back(resItr->first);
			}
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

	void print(const string & tableName){

		vector<string> colomnHeaders;
		colomnHeaders.push_back("Lock holders");
		vector<string> labels;
		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::const_iterator
				resItr = grantedLocks.begin(); resItr != grantedLocks.end(); ++resItr){
			if(resItr->second.size() > 0){
				labels.push_back(_toString(resItr->first));
			}
		}
		if(labels.size() == 0){
			cout << tableName << " is empty." << endl;
			return;
		}
		srch2::util::TableFormatPrinter lockTable(tableName, 120, colomnHeaders, labels );
		lockTable.printColumnHeaders();
		lockTable.startFilling();
		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::const_iterator
				resItr = grantedLocks.begin(); resItr != grantedLocks.end(); ++resItr){
			if(resItr->second.size() == 0){
				continue;
			}
			stringstream ss;
			for(unsigned i = 0; i < resItr->second.size(); ++i){
				if(i != 0){
					ss << " ==|== ";
				}
				ss << resItr->second.at(i).first.toString() << "=" << resItr->second.at(i).second ;
			}
			lockTable.printNextCell(ss.str());
		}
	}

	bool isClean() const{
		for(typename map<Resource, vector<pair<NodeOperationId, LockLevel> > >::const_iterator
				resItr = grantedLocks.begin(); resItr != grantedLocks.end(); ++resItr){
			if(! resItr->second.empty()){
				return false;
			}
		}
		return true;
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
	string _toString(const ClusterShardId & id){
		return id.toString();
	}
	string _toString(const string & resource){
		return resource;
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

	string getBatchTypeStr() const {
		stringstream ss;
	    switch(batchType){
	    case LockRequestType_Copy:
	        ss << "copy";
	        break;
	    case LockRequestType_Move:
            ss << "move";
            break;
	    case LockRequestType_Metadata:
            ss << "metadata";
            break;
	    case LockRequestType_PrimaryKey:
            ss << "pk";
            break;
	    case LockRequestType_GeneralPurpose:
            ss << "gen-purpose";
            break;
	    case LockRequestType_ShardIdList:
            ss << "shard-list";
            break;
	    }
	    return ss.str();
	}

	string getLockHoldersStr() const{
		stringstream ss;
		for(unsigned i = 0 ; i < opIds.size(); ++i){
			if(i != 0){
				ss << " - ";
			}
			ss << opIds.at(i).toString();
		}
		return ss.str();
	}
	string getResourceStr() const{
	    stringstream ss;
        switch(batchType){
        case LockRequestType_Copy:
        case LockRequestType_Move:
        case LockRequestType_GeneralPurpose:
        case LockRequestType_ShardIdList:
            {
                for(unsigned i = 0 ; i < tokens.size(); ++i){
                    ss << tokens.at(i).first.toString() << "/";
                    if(tokens.at(i).second == LockLevel_S){
                        ss << "S%";
                    }else{
                        ss << "X%";
                    }
                }
                break;
            }
        case LockRequestType_Metadata:
            ss << "metadata";
            if(metadataLockLevel == LockLevel_S){
                ss << "S%";
            }else{
                ss << "X%";
            }
            break;
        case LockRequestType_PrimaryKey:
        	ss << "PK%";
            break;
        }
        return ss.str();
	}

	string getExtraInfoStr() const{
		stringstream ss;
		ss << "b:" << blocking << "|";
		ss << "r:" << release << "|";
		ss << "i:" << incremental << "|";
		ss << "v:" << versionId << "|";
		return ss.str();
	}

	string toString(){
	    stringstream ss;
	    ss << "B(" << blocking << "), R(" << release << "), I(" << incremental << "), batchType(" << getBatchTypeStr() << "), ";
        switch(batchType){
        case LockRequestType_Copy:
        case LockRequestType_Move:
        case LockRequestType_GeneralPurpose:
        case LockRequestType_ShardIdList:
            {
                ss << "for : ";
                for(unsigned i = 0 ; i < tokens.size(); ++i){
                    if(tokens.at(i).second == LockLevel_S){
                        ss << "S on ";
                    }else{
                        ss << "X on ";
                    }
                    ss << tokens.at(i).first.toString() << " - ";
                }
                break;
            }
        case LockRequestType_Metadata:
            ss << "for ";
            if(metadataLockLevel == LockLevel_S){
                ss << "S on ";
            }else{
                ss << "X on ";
            }
            ss << "metadata";
            break;
        case LockRequestType_PrimaryKey:
            break;
        }
        return ss.str();
	}

	static LockBatch * generateLockBatch(SP(LockingNotification) notif);
	static LockBatch * generateLockBatch(const ClusterShardId & shardId, const LockLevel & lockLevel);

private:
	// so that only our static class makes these objects
	LockBatch();

};


}
}

#endif // __SHARDING_LOCK_MANAGER_LOCK_BATCH_H__
