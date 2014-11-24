
#include "Write2PCNotification.h"
#include "../ShardManager.h"
#include "../state_machine/StateMachine.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "sharding/processor/DistributedProcessorInternal.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

RecordWriteOpHandle::RecordWriteOpHandle(Record * record){
	this->record = record;
	this->primaryKey = "";
	this->success = false;
	this->workDone = false;
	this->recordAclCommandFlag = false;
}
RecordWriteOpHandle::RecordWriteOpHandle(const string & primaryKey){
	this->record = NULL;
	this->primaryKey = primaryKey;
	this->success = false;
	this->workDone = false;
	this->recordAclCommandFlag = false;
}
RecordWriteOpHandle::RecordWriteOpHandle(const string & primaryKey, const vector<string> & roleIds){
	this->record = NULL;
	this->primaryKey = primaryKey;
	this->roleIds = roleIds;
	this->success = false;
	this->workDone = false;
	this->recordAclCommandFlag = true;
}
RecordWriteOpHandle::RecordWriteOpHandle(){
	this->record = NULL;
	this->primaryKey = "";
	this->success = false;
	this->workDone = false;
	this->recordAclCommandFlag = false;
}
string RecordWriteOpHandle::getPrimaryKey() const{
	if(record == NULL){
		return primaryKey;
	}else{
		return record->getPrimaryKey();
	}
}
Record * RecordWriteOpHandle::getRecordObj(){
	ASSERT(record != NULL);
	return record;
}
vector<string> & RecordWriteOpHandle::getRoleIds(){
	return roleIds;
}
void RecordWriteOpHandle::finalize(bool success){
	ASSERT(!this->workDone);
	this->workDone = true;
	this->success = success;
}
bool RecordWriteOpHandle::isSuccessful() const{
	return this->success;
}
bool RecordWriteOpHandle::isWorkDone() const{
	return this->workDone;
}
void RecordWriteOpHandle::addMessage(ShardId * _shardId, const JsonMessageCode messageCode){
	ShardId * shardId;
	if(_shardId == NULL){
		shardId = _shardId;
	}else{
		if(_shardId->isClusterShard()){
			shardId = new ClusterShardId((const ClusterShardId &)(*_shardId));
		}else{
			shardId = new NodeShardId((const NodeShardId &)(*_shardId));
		}
	}
	if(messageCodes.find(shardId) == messageCodes.end()){
		messageCodes[shardId] = vector<JsonMessageCode>();
	}
	messageCodes[shardId].push_back(messageCode);
}


map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > RecordWriteOpHandle::getMessageCodes(){
	return this->messageCodes;
}


void * RecordWriteOpHandle::serialize(void * buffer, const WriteNotificationMode mode) const{
	switch (mode) {
		case WriteNotificationModeAsk2PC:
		{
			// only transferring primary keys
			buffer = srch2::util::serializeString(this->getPrimaryKey(), buffer);
			break;
		}
		case WriteNotificationModePerformWrite:
		{
			buffer = srch2::util::serializeFixedTypes((bool)(this->record == NULL), buffer );
			if(record == NULL){
				// it's now either record delete or record acl command
				buffer = srch2::util::serializeFixedTypes(recordAclCommandFlag,  buffer );
				if(recordAclCommandFlag){
					// acl record
					// serialize the roleIds
					buffer = srch2::util::serializeVectorOfString(this->roleIds, buffer);
				}else{
					// record delete command
					buffer = srch2::util::serializeString(this->primaryKey, buffer);
				}
			}else{
				// record exists
				buffer = this->record->serializeForNetwork(buffer);
			}
			break;
		}
	}
	return buffer;
}
void * RecordWriteOpHandle::deserialize(void * buffer, const WriteNotificationMode mode, const Schema *schema){
	switch (mode) {
		case WriteNotificationModeAsk2PC:
		{
			// only transferring primary keys
			buffer = srch2::util::deserializeString(buffer, this->primaryKey);
			this->record = NULL;
			break;
		}
		case WriteNotificationModePerformWrite:
		{
			bool recordIsNull = false;
			buffer = srch2::util::deserializeFixedTypes(buffer, recordIsNull);
			if(recordIsNull){
				// it's now either record delete or record acl command
				buffer = srch2::util::deserializeFixedTypes(buffer, recordAclCommandFlag);
				if(recordAclCommandFlag){
					// acl record
					// serialize the roleIds
					buffer = srch2::util::deserializeVectorOfString(buffer, this->roleIds);
				}else{
					buffer = srch2::util::deserializeString(buffer, this->primaryKey);
				}
			}else{
				this->record = new Record(schema);
				buffer = Record::deserializeForNetwork(buffer, *(this->record));
			}
			break;
		}
	}
	return buffer;
}
unsigned RecordWriteOpHandle::getNumberOfBytes(const WriteNotificationMode mode) const{
	unsigned numberOfBytes = 0;
	switch (mode) {
		case WriteNotificationModeAsk2PC:
		{
			// only transferring primary keys
			numberOfBytes += sizeof(unsigned) + this->primaryKey.size();
			break;
		}
		case WriteNotificationModePerformWrite:
		{
			numberOfBytes += sizeof(bool);
			if(this->record == NULL){
				numberOfBytes += sizeof(bool);
				if(recordAclCommandFlag){
					// acl record
					// serialize the roleIds
					numberOfBytes += srch2::util::getNumberOfBytesVectorOfString(roleIds);
				}else{
					numberOfBytes += sizeof(unsigned) + this->primaryKey.size();
				}
			}else{
				numberOfBytes += this->record->getNumberOfBytesSize();
			}
			break;
		}
	}
	return numberOfBytes;
}



ShardingMessageType Write2PCNotification::messageType() const{
	return ShardingWriteCommand2PCMessageType;
}
Write2PCNotification::Write2PCNotification(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		ClusterRecordOperation_Type commandType,
		NodeTargetShardInfo targets,
		WriteNotificationMode mode,
		vector<RecordWriteOpHandle *> recordHandles){
	this->clusterReadview = clusterReadview;
	this->commandType = commandType;
	this->targets = targets;
	this->recordHandles = recordHandles;
	this->mode = mode;
}
Write2PCNotification::Write2PCNotification(){
	ShardManager::getReadview(clusterReadview);
}


bool Write2PCNotification::resolveNotification(SP(ShardingNotification) _notif){
	SP(Write2PCNotification::ACK) response =
			ShardManager::getShardManager()->getDPInternal()->
			resolveWrite2PC(boost::dynamic_pointer_cast<Write2PCNotification>(_notif));
	if(! response){
		response = create<Write2PCNotification::ACK>();
	}
    response->setSrc(_notif->getDest());
    response->setDest(_notif->getSrc());
	send(response);
	return true;
}

void * Write2PCNotification::serializeBody(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes(commandType, buffer);
	buffer = targets.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(mode, buffer);
	buffer = srch2::util::serializeFixedTypes((unsigned)(recordHandles.size()), buffer);
	for(unsigned i = 0 ; i < recordHandles.size(); ++i){
		buffer = recordHandles.at(i)->serialize(buffer, mode);
	}
	return buffer;
}
unsigned Write2PCNotification::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(commandType);
	numberOfBytes += targets.getNumberOfBytes();
	numberOfBytes += sizeof(mode);
	numberOfBytes += sizeof(unsigned);
	for(unsigned i = 0 ; i < recordHandles.size(); ++i){
		numberOfBytes += recordHandles.at(i)->getNumberOfBytes(mode);
	}
	return numberOfBytes;
}
void * Write2PCNotification::deserializeBody(void * buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, commandType);
	buffer = targets.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, mode);

	const Schema * schema = clusterReadview->getCore(targets.getCoreId())->getSchema();
	unsigned recordHandlesSize = 0;
	buffer = srch2::util::deserializeFixedTypes(buffer, recordHandlesSize);
	for(unsigned i = 0 ; i < recordHandlesSize; ++i){
		RecordWriteOpHandle * newHandle = new RecordWriteOpHandle();
		buffer = newHandle->deserialize(buffer, mode, schema);
		this->recordHandles.push_back(newHandle);
	}
	return buffer;
}

vector<RecordWriteOpHandle *> & Write2PCNotification::getRecordHandles(){
	return recordHandles;
}

ClusterRecordOperation_Type Write2PCNotification::getCommandType() const{
	return commandType;
}

NodeTargetShardInfo Write2PCNotification::getTargets() const{
	return targets;
}

boost::shared_ptr<const ClusterResourceMetadata_Readview> Write2PCNotification::getClusterReadview(){
	return clusterReadview;
}


Write2PCNotification::ACK::ShardResult::ShardResult(ShardId * shardId, bool statusValue, const vector<JsonMessageCode> & messageCodes){
	ASSERT(shardId != NULL);
	this->shardId = shardId;
	this->statusValue = statusValue;
	this->messageCodes = messageCodes;
}
Write2PCNotification::ACK::ShardResult::~ShardResult(){
	if(shardId != NULL){
		delete shardId;
	}
}
Write2PCNotification::ACK::ShardResult::ShardResult(){
	shardId = NULL;
}

void * Write2PCNotification::ACK::ShardResult::serialize(void * buffer) const{
	ASSERT(shardId != NULL);
	buffer = srch2::util::serializeFixedTypes((bool)shardId->isClusterShard(), buffer);
	buffer = shardId->serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(statusValue, buffer);
    buffer = srch2::util::serializeVectorOfFixedTypes(messageCodes, buffer);
    return buffer;
}
unsigned Write2PCNotification::ACK::ShardResult::getNumberOfBytes() const{
	ASSERT(shardId != NULL);
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(bool);
	numberOfBytes += shardId->getNumberOfBytes();
	numberOfBytes += sizeof(statusValue);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfFixedTypes(messageCodes);
	return numberOfBytes;
}
void * Write2PCNotification::ACK::ShardResult::deserialize(void * buffer){
	bool isClusterShard = false;
	buffer = srch2::util::deserializeFixedTypes(buffer, isClusterShard);
	if(isClusterShard){
		shardId = new ClusterShardId();
	}else{
		shardId = new NodeShardId();
	}
	buffer = shardId->deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, statusValue);
    buffer = srch2::util::deserializeVectorOfFixedTypes(buffer, messageCodes);
    return buffer;
}


ShardingMessageType Write2PCNotification::ACK::messageType() const{
	return ShardingWriteCommand2PCACKMessageType;
}
Write2PCNotification::ACK::~ACK(){
	for(map<string, vector<ShardResult *> >::iterator pItr =
			primaryKeyShardResults.begin(); pItr != primaryKeyShardResults.end(); ++pItr){
		for(unsigned i =0 ; i < pItr->second.size(); ++i){
			if(pItr->second.at(i) != NULL){
				delete pItr->second.at(i);
			}
		}
	}
}

bool Write2PCNotification::ACK::resolveNotification(SP(ShardingNotification) _notif){
	ShardManager::getShardManager()->getStateMachine()->handle(_notif);
	return true;
}

void * Write2PCNotification::ACK::serializeBody(void * buffer) const{
	buffer = srch2::util::serializeFixedTypes((unsigned)primaryKeyShardResults.size() , buffer);
	for(map<string, vector<ShardResult *> >::const_iterator pItr = primaryKeyShardResults.begin();
			pItr != primaryKeyShardResults.end(); ++pItr){
		buffer = srch2::util::serializeString(pItr->first, buffer);
		buffer = srch2::util::serializeVectorOfDynamicTypePointers(pItr->second, buffer);
	}
	return buffer;
}
unsigned Write2PCNotification::ACK::getNumberOfBytesBody() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	for(map<string, vector<ShardResult *> >::const_iterator pItr = primaryKeyShardResults.begin();
			pItr != primaryKeyShardResults.end(); ++pItr){
		numberOfBytes += sizeof(unsigned ) + pItr->first.size();
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypePointers(pItr->second);
	}
	return numberOfBytes;
}
void * Write2PCNotification::ACK::deserializeBody(void * buffer){
	unsigned sizeOfMap;
	buffer = srch2::util::deserializeFixedTypes(buffer, sizeOfMap);
	for(unsigned i = 0 ; i < sizeOfMap ; i++){
		string key;
		buffer = srch2::util::deserializeString(buffer, key);
		vector<ShardResult *> value;
		buffer = srch2::util::deserializeVectorOfDynamicTypePointers(buffer, value);
		primaryKeyShardResults[key] = value;
	}
	return buffer;
}

void Write2PCNotification::ACK::addPrimaryKeyShardResult(const string & primaryKey, ShardResult * result){
	ASSERT(result != NULL);
	if(primaryKeyShardResults.find(primaryKey) == primaryKeyShardResults.end()){
		primaryKeyShardResults[primaryKey] = vector<ShardResult *>();
	}
	primaryKeyShardResults[primaryKey].push_back(result);
}

map<string, vector<Write2PCNotification::ACK::ShardResult *> > & Write2PCNotification::ACK::getResults(){
	return primaryKeyShardResults;
}

}
}
