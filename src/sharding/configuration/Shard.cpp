
#include "Shard.h"

#include "src/core/util/SerializationHelper.h"
#include <sstream>

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

bool ShardId::isPrimaryShard() const{
	return (replicaId == 0); // replica #0 is always the primary shard
}
std::string ShardId::toString() const {
	// A primary shard starts with a "P" followed by an integer id.
	// E.g., a cluster with 4 shards of core 8 will have shards named "C8_P0", "C8_R0_1", "C8_R0_2", "C8_P3".
	//
	// A replica shard starts with an "R" followed by a replica count and then its primary's id.
	// E.g., for the above cluster, replicas of "P0" will be named "8_R1_0" and "8_R2_0".
	// Similarly, replicas of "P3" will be named "8_R3_1" and "8_R3_2".
	if(coreId != unsigned(-1) || partitionId != unsigned(-1) || replicaId != unsigned(-1)){
		std::stringstream sstm;
		sstm << "C" << coreId << "_";
		if (isPrimaryShard()){
			sstm << "P" << partitionId;
		}
		else{
			sstm << "R" << partitionId << "_" << replicaId;
		}
		return sstm.str();
	}
	else{
		return "";
	}
}

ShardId::ShardId() {
	coreId = unsigned(-1);
	partitionId = unsigned(-1);
	replicaId = unsigned(-1);
}
ShardId::ShardId(unsigned coreId, unsigned partitionId, unsigned replicaId) :
	coreId(coreId), partitionId(partitionId), replicaId(replicaId) {}

bool ShardId::operator==(const ShardId& rhs) const {
	return coreId == rhs.coreId && partitionId == rhs.partitionId
			&& replicaId == replicaId;
}
bool ShardId::operator!=(const ShardId& rhs) const {
	return coreId != rhs.coreId || partitionId != rhs.partitionId
			|| replicaId != replicaId;
}
bool ShardId::operator>(const ShardId& rhs) const {
	return  coreId > rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId > rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId > replicaId)));
}
bool ShardId::operator<(const ShardId& rhs) const {
	return  coreId < rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId < rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId < replicaId)));
}
bool ShardId::operator>=(const ShardId& rhs) const {
	return  coreId > rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId > rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId >= replicaId)));
}
bool ShardId::operator<=(const ShardId& rhs) const {
	return  coreId < rhs.coreId ||
			(coreId == rhs.coreId &&
					(partitionId < rhs.partitionId ||
							(partitionId == rhs.partitionId && replicaId <= replicaId)));
}


//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* ShardId::serializeForNetwork(void * buffer){
	buffer = srch2::util::serializeFixedTypes(partitionId, buffer);
	buffer = srch2::util::serializeFixedTypes(replicaId, buffer);
	buffer = srch2::util::serializeFixedTypes(coreId, buffer);
	return buffer;
}

//given a byte stream recreate the original object
void * ShardId::deserializeForNetwork(void* buffer){
	buffer = srch2::util::deserializeFixedTypes(buffer, partitionId);
	buffer = srch2::util::deserializeFixedTypes(buffer, replicaId);
	buffer = srch2::util::deserializeFixedTypes(buffer, coreId);
	return buffer;
}

unsigned ShardId::getNumberOfBytesForNetwork(){
	return 3 * sizeof(unsigned);
}

/////////////////////////////// ShardIdComparator
bool ShardIdComparator::operator() (const ShardId s1, const ShardId s2) {
	if (s1.coreId > s2.coreId)
		return true;

	if (s1.coreId < s2.coreId)
		return false;

	// they have equal coreId; we look at their partitionId
	if (s1.partitionId > s2.partitionId)
		return true;

	if (s1.partitionId < s2.partitionId)
		return false;

	// they have equal partitionId; we look at their replicaId
	if (s1.replicaId > s2.replicaId)
		return true;

	return false;
}

/////////////////////////////// Shard

Shard::Shard(){
	this->nodeId = 0;
	this->shardState = SHARDSTATE_UNALLOCATED;
	this->shardId.coreId = 0;
	this->shardId.partitionId = 0;
	this->shardId.replicaId = 0;
}

Shard::Shard(const Shard & shard){
	this->nodeId = shard.nodeId;
	this->shardState = shard.shardState;
	this->shardId.coreId = shard.shardId.coreId;
	this->shardId.partitionId = shard.shardId.partitionId;
	this->shardId.replicaId = shard.shardId.replicaId;
	this->srch2Server = shard.srch2Server;
}

Shard::Shard(unsigned nodeId, unsigned coreId, unsigned partitionId, unsigned replicaId) {
	this->nodeId = nodeId;
	this->shardState = SHARDSTATE_UNALLOCATED;
	this->shardId.coreId = coreId;
	this->shardId.partitionId = partitionId;
	this->shardId.replicaId = replicaId;
}

//Can be used in Migration
void Shard::setPartitionId(int partitionId) {
	this->shardId.partitionId = partitionId;
}

//Can be used in Migration
void Shard::setReplicaId(int replicaId) {
	this->shardId.replicaId = replicaId;
}

ShardId Shard::getShardId() const{
	return this->shardId;
}

void Shard::setShardState(ShardState newState){
	this->shardState = newState;
}

void Shard::setNodeId(unsigned id){
	this->nodeId = id;
}

ShardState Shard::getShardState() const{
	return this->shardState;
}

unsigned Shard::getNodeId() const{
	return this->nodeId;
}

void Shard::setSrch2Server(boost::shared_ptr<Srch2Server> srch2Server){
	this->srch2Server = srch2Server;
}

boost::shared_ptr<Srch2Server> Shard::getSrch2Server() const{
	return this->srch2Server;
}

std::string Shard::toString() const{
	std::stringstream sstm;
	sstm << "ShardId : " << shardId.toString();
	sstm << ", ShardState";
	switch (shardState) {
	case SHARDSTATE_ALLOCATED: // must have a valid node
		sstm << "SHARDSTATE_ALLOCATED" ;
		break;
	case SHARDSTATE_UNALLOCATED:
		sstm << "SHARDSTATE_UNALLOCATED" ;
		break;
	case SHARDSTATE_MIGRATING:
		sstm << "SHARDSTATE_MIGRATING" ;
		break;
	case SHARDSTATE_INDEXING:
		sstm << "SHARDSTATE_INDEXING" ;
		break;
	// these are the constants that DPEx, DPInt, RM and MM use
	case SHARDSTATE_REGISTERED:
		sstm << "SHARDSTATE_REGISTERED" ;
		break;
	case SHARDSTATE_NOT_COMMITTED:
		sstm << "SHARDSTATE_NOT_COMMITTED" ;
		break;
	case SHARDSTATE_COMMITTED:
		sstm << "SHARDSTATE_COMMITTED" ;
		break;
	}
	sstm << ", nodeId" << nodeId;

	return sstm.str();

}



//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* Shard::serializeForNetwork(void * buffer){
	buffer = shardId.serializeForNetwork(buffer);
	buffer = srch2::util::serializeFixedTypes(shardState, buffer);
	buffer = srch2::util::serializeFixedTypes(nodeId, buffer );
	return buffer;
}

//given a byte stream recreate the original object
Shard * Shard::deserializeForNetwork(void* buffer){
	Shard * newShard = new Shard();
	buffer = newShard->shardId.deserializeForNetwork(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, newShard->shardState);
	buffer = srch2::util::deserializeFixedTypes(buffer, newShard->nodeId);
	return newShard;
}

unsigned Shard::getNumberOfBytesForNetwork(){
	unsigned numberOfBytes = 0 ;
	numberOfBytes += shardId.getNumberOfBytesForNetwork();
	numberOfBytes += sizeof(ShardState);
	numberOfBytes += sizeof(unsigned);
	return numberOfBytes;
}


/////////////////////////////// CoreShardContainer

CoreShardContainer::CoreShardContainer(CoreInfo_t * core){
	this->core = core;
}
CoreShardContainer::CoreShardContainer(const CoreShardContainer & coreShardContainer){
	this->setCore(coreShardContainer.core);
	for(vector<Shard *>::const_iterator shardItr = coreShardContainer.primaryShards.begin(); shardItr != coreShardContainer.primaryShards.end(); ++ shardItr){
		this->primaryShards.push_back(new Shard(**shardItr));
	}
	for(vector<Shard *>::const_iterator shardItr = coreShardContainer.replicaShards.begin(); shardItr != coreShardContainer.replicaShards.end(); ++ shardItr){
		this->replicaShards.push_back(new Shard(**shardItr));
	}
}
CoreShardContainer::~CoreShardContainer(){
	// TODO : we don't delete core objects here, because
	// since currently we don't change cores, the pointer to CoreInfo_t is
	// always passed ....
	// delete shards
	for(vector<Shard *>::iterator shardItr = primaryShards.begin(); shardItr != primaryShards.end(); ++ shardItr){
		delete *shardItr;
	}
	// delete shards
	for(vector<Shard *>::iterator shardItr = replicaShards.begin(); shardItr != replicaShards.end(); ++ shardItr){
		delete *shardItr;
	}
}

CoreInfo_t * CoreShardContainer::getCore(){
	return core;
}
const CoreInfo_t * CoreShardContainer::getCore() const{
	return core;
}

string CoreShardContainer::getCoreName() const{
	return this->coreName;
}

void CoreShardContainer::setCore(CoreInfo_t * core){
	this->core = core;
	ASSERT(core != NULL);
	this->coreName = core->getName();
}

vector<Shard *> * CoreShardContainer::getPrimaryShards(){
	return &(this->primaryShards);
}
vector<Shard *> * CoreShardContainer::getReplicaShards(){
	return &(this->replicaShards);
}

void CoreShardContainer::setPrimaryShards(vector<Shard *> & srcPrimaryShards){
	for(unsigned pIndex = 0 ; pIndex < srcPrimaryShards.size(); ++pIndex){
		this->primaryShards.push_back(new Shard(*(srcPrimaryShards.at(pIndex))));
	}
}
void CoreShardContainer::setReplicaShards(vector<Shard *> & srcReplicaShards){
	for(unsigned rIndex = 0 ; rIndex < srcReplicaShards.size(); ++rIndex){
		this->replicaShards.push_back(new Shard(*(srcReplicaShards.at(rIndex))));
	}
}

void CoreShardContainer::setSrch2ServerPointers(CoreShardContainer * src){
	// TODO : for now, src and local primary and replica shards must be the same
	ASSERT(primaryShards.size() == src->getPrimaryShards()->size());
	ASSERT(replicaShards.size() == src->getReplicaShards()->size());

	// first create a map from ShardIds to Shard * of src
	std::map<ShardId, Shard * > shardIdToShardMap;
	// go over primary and replica shards and fill the map
	for(unsigned p = 0 ; p < src->getPrimaryShards()->size() ; ++p){
		// nothing must be repeated
		ASSERT(shardIdToShardMap.find(src->getPrimaryShards()->at(p)->getShardId()) == shardIdToShardMap.end());
		shardIdToShardMap.insert(std::make_pair(src->getPrimaryShards()->at(p)->getShardId(), src->getPrimaryShards()->at(p)));
	}
	for(unsigned r = 0 ; r < src->getReplicaShards()->size() ; ++r){
		// nothing must be repeated
		ASSERT(shardIdToShardMap.find(src->getReplicaShards()->at(r)->getShardId()) == shardIdToShardMap.end());
		shardIdToShardMap.insert(std::make_pair(src->getReplicaShards()->at(r)->getShardId(), src->getReplicaShards()->at(r)));
	}
	// go over local primary shards and replica shards and set the srch2server shared pointer
	for(unsigned p = 0 ; p < primaryShards.size() ; ++p){
		// nothing must be missing // TODO : for now , this assumption is true
		ASSERT(shardIdToShardMap.find(primaryShards.at(p)->getShardId()) != shardIdToShardMap.end());
		Shard * srcShard = shardIdToShardMap.find(primaryShards.at(p)->getShardId())->second;
		primaryShards.at(p)->setSrch2Server(srcShard->getSrch2Server());
	}
	for(unsigned r = 0 ; r < replicaShards.size() ; ++r){
		ASSERT(shardIdToShardMap.find(replicaShards.at(r)->getShardId()) != shardIdToShardMap.end());
		Shard * srcShard = shardIdToShardMap.find(replicaShards.at(r)->getShardId())->second;
		replicaShards.at(r)->setSrch2Server(srcShard->getSrch2Server());
	}


}

void CoreShardContainer::addPrimaryShards(vector<const Shard *> & primaryShards) const{
	for(vector<Shard *>::const_iterator shardItr = this->primaryShards.begin(); shardItr != this->primaryShards.end(); ++shardItr){
		primaryShards.push_back(*shardItr);
	}
}
void CoreShardContainer::addReplicaShards(vector<const Shard *> & replicaShards) const{
	for(vector<Shard *>::const_iterator shardItr = this->replicaShards.begin(); shardItr != this->replicaShards.end(); ++shardItr){
		replicaShards.push_back(*shardItr);
	}
}
void CoreShardContainer::addPrimaryShardReplicas(const ShardId & primaryShardId, vector<const Shard *> & replicaShards) const{
	for(vector<Shard *>::const_iterator shardItr = this->replicaShards.begin(); shardItr != this->replicaShards.end(); ++shardItr){
		ASSERT((*shardItr)->getShardId().coreId == primaryShardId.coreId);
		ASSERT((*shardItr)->getShardId().coreId == this->getCore()->getCoreId());
		if((*shardItr)->getShardId().partitionId == primaryShardId.partitionId){
			replicaShards.push_back(*shardItr);
		}
	}
}
unsigned CoreShardContainer::getTotalNumberOfPrimaryShards() const{
	return this->primaryShards.size();
}

const Shard * CoreShardContainer::getShard(const ShardId & shardId) const{
	for(vector<Shard *>::const_iterator shardItr = this->primaryShards.begin(); shardItr != this->primaryShards.end(); ++shardItr){
		if((*shardItr)->getShardId() == shardId){
			return *shardItr;
		}
	}
	for(vector<Shard *>::const_iterator shardItr = this->replicaShards.begin(); shardItr != this->replicaShards.end(); ++shardItr){
		if((*shardItr)->getShardId() == shardId){
			return *shardItr;
		}
	}
	return NULL;
}


//serializes the object to a byte array and places array into the region
//allocated by given allocator
void* CoreShardContainer::serializeForNetwork(void * buffer){
	buffer = srch2::util::serializeString(coreName, buffer);

	buffer = srch2::util::serializeFixedTypes((unsigned)(primaryShards.size()), buffer);
	for(unsigned p = 0 ; p < primaryShards.size(); ++p){
		buffer = primaryShards.at(p)->serializeForNetwork(buffer);
	}
	buffer = srch2::util::serializeFixedTypes((unsigned)(replicaShards.size()), buffer);
	for(unsigned p = 0 ; p < replicaShards.size(); ++p){
		buffer = replicaShards.at(p)->serializeForNetwork(buffer);
	}
	return buffer;
}

//given a byte stream recreate the original object
CoreShardContainer * CoreShardContainer::deserializeForNetwork(void* buffer){
	CoreShardContainer * newObj = new CoreShardContainer(NULL);
	buffer = srch2::util::deserializeString(buffer, newObj->coreName);

	unsigned size = 0;
	buffer = srch2::util::deserializeFixedTypes(buffer, size);
	for(unsigned p = 0 ; p < size; ++p){
		Shard * newShard = Shard::deserializeForNetwork(buffer);
		newObj->primaryShards.push_back(newShard);
		buffer = (char *)buffer + newShard->getNumberOfBytesForNetwork();
	}
	buffer = srch2::util::deserializeFixedTypes(buffer, size);
	for(unsigned p = 0 ; p < size; ++p){
		Shard * newShard = Shard::deserializeForNetwork(buffer);
		newObj->replicaShards.push_back(newShard);
		buffer = (char *)buffer + newShard->getNumberOfBytesForNetwork();
	}
	return newObj;
}

unsigned CoreShardContainer::getNumberOfBytesForNetwork(){
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned) + coreName.size();
	numberOfBytes += sizeof(unsigned); // number of primary shards
	for(unsigned p = 0 ; p < primaryShards.size(); ++p){
		numberOfBytes += primaryShards.at(p)->getNumberOfBytesForNetwork();
	}
	numberOfBytes += sizeof(unsigned); // number of replica shards
	for(unsigned p = 0 ; p < replicaShards.size(); ++p){
		numberOfBytes += replicaShards.at(p)->getNumberOfBytesForNetwork();
	}
	return numberOfBytes;
}

}
}
