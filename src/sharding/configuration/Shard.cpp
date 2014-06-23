
#include "Shard.h"

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
CoreInfo_t * CoreShardContainer::getCore(){
	return core;
}
const CoreInfo_t * CoreShardContainer::getCore() const{
	return core;
}
void CoreShardContainer::setCore(CoreInfo_t * core){
	this->core = core;
}

vector<Shard *> * CoreShardContainer::getPrimaryShards(){
	return &(this->primaryShards);
}
vector<Shard *> * CoreShardContainer::getReplicaShards(){
	return &(this->replicaShards);
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

}
}
