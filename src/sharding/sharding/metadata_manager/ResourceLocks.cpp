
#include "ResourceLocks.h"
#include "core/util/Assert.h"
#include "core/util/SerializationHelper.h"
#include "../ShardManager.h"
#include "../notifications/NewNodeLockNotification.h"
#include "Cluster_Writeview.h"
#include "ResourceMetadataManager.h"
#include "sharding/util/FramedPrinter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

SingleResourceLockRequest::SingleResourceLockRequest(const ClusterShardId & resource,
		const vector<NodeOperationId> & holders,
		ResourceLockType lockType){
	this->resource = resource;
	this->holders = holders;
	this->lockType = lockType;
	this->requestType = ResourceLockRequestTypeLock;
}

SingleResourceLockRequest::SingleResourceLockRequest(const ClusterShardId & resource,
		const vector<NodeOperationId> & holders){
	this->resource = resource;
	this->holders = holders;
	this->requestType = ResourceLockRequestTypeRelease;
}

// Lock
SingleResourceLockRequest::SingleResourceLockRequest(const ClusterShardId & resource,
		const NodeOperationId & holder,
		ResourceLockType lockType){
	vector<NodeOperationId> holders;
	holders.push_back(holder);
	this->resource = resource;
	this->holders = holders;
	this->lockType = lockType;
	this->requestType = ResourceLockRequestTypeLock;
}

// Release
SingleResourceLockRequest::SingleResourceLockRequest(const ClusterShardId & resource,
		const NodeOperationId & holder){
	vector<NodeOperationId> holders;
	holders.push_back(holder);
	this->resource = resource;
	this->holders = holders;
	this->requestType = ResourceLockRequestTypeRelease;
}

SingleResourceLockRequest::SingleResourceLockRequest(const ClusterShardId & resource,ResourceLockRequestType requestType){
	this->resource = resource;
	ASSERT(requestType == ResourceLockRequestTypeUpgrade ||
			requestType == ResourceLockRequestTypeDowngrade);
	this->requestType = requestType;
}

SingleResourceLockRequest::SingleResourceLockRequest(const SingleResourceLockRequest & copy){
	this->resource = copy.resource;
	this->lockType = copy.lockType;
	this->requestType = copy.requestType;
	this->holders = copy.holders;
}

bool SingleResourceLockRequest::operator==(const SingleResourceLockRequest & right){
	return (resource == right.resource) && (lockType == right.lockType)
			&& (requestType == right.requestType) && (holders == right.holders);
}

void * SingleResourceLockRequest::serialize(void * buffer) const{
	buffer = resource.serialize(buffer);
	buffer = srch2::util::serializeFixedTypes(lockType, buffer);
	buffer = srch2::util::serializeFixedTypes(requestType, buffer);
	buffer = srch2::util::serializeVectorOfDynamicTypes(holders, buffer);
	return buffer;
}
unsigned SingleResourceLockRequest::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += resource.getNumberOfBytes();
	numberOfBytes += sizeof(ResourceLockType);
	numberOfBytes += sizeof(ResourceLockRequestType);
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypes(holders);
	return numberOfBytes;
}
void * SingleResourceLockRequest::deserialize(void * buffer){
	buffer = resource.deserialize(buffer);
	buffer = srch2::util::deserializeFixedTypes(buffer, lockType);
	buffer = srch2::util::deserializeFixedTypes(buffer, requestType);
	buffer = srch2::util::deserializeVectorOfDynamicTypes(buffer, holders);
	return buffer;
}
bool SingleResourceLockRequest::applyNodeFailure(const unsigned failedNodeId){
	vector<NodeOperationId> holdersFixed;
	for(unsigned i = 0 ; i < holders.size() ; ++i){
		if(holders.at(i).nodeId != failedNodeId){
			holdersFixed.push_back(holders.at(i));
		}
	}
	if(holdersFixed.size() == 0){
		holders.clear();
		return false;
	}
	holders = holdersFixed;
	return true;
}

string SingleResourceLockRequest::toString() const{
	stringstream ss;
	switch(requestType){
	case ResourceLockRequestTypeLock:
		switch (lockType) {
			case ResourceLockType_S:
				ss << "S on " << resource.toString() ;
				break;
			case ResourceLockType_X:
				ss << "X on " << resource.toString() ;
				break;
			case ResourceLockType_U:
				ss << "U on " << resource.toString() ;
				break;
		}
		ss << " for ";
		for(unsigned i = 0; i < holders.size(); ++i){
			if(i != 0){
				ss << ",";
			}
			ss << holders.at(i).toString();
		}
		break;
	case ResourceLockRequestTypeUpgrade:
		ss << "Upgrd " << resource.toString();
		break;
	case ResourceLockRequestTypeRelease:
		ss << "Rls " << resource.toString();
		break;
	case ResourceLockRequestTypeDowngrade:
		ss << "Dwngrd " << resource.toString();
		break;
	}
	return ss.str();
}

ResourceLockRequest::ResourceLockRequest(){
	this->isBlocking = false;
}
ResourceLockRequest::~ResourceLockRequest(){
	for(unsigned i = 0; i < requestBatch.size(); ++i){
		delete requestBatch.at(i);
		requestBatch.at(i) = NULL;
	}
	requestBatch.clear();
}

ResourceLockRequest::ResourceLockRequest(const ResourceLockRequest & copy){
	this->isBlocking = copy.isBlocking;
	for(unsigned i = 0 ; i < copy.requestBatch.size() ; ++i){
		this->requestBatch.push_back(new SingleResourceLockRequest(*(copy.requestBatch.at(i))));
	}
}

bool ResourceLockRequest::applyNodeFailure(const unsigned failedNodeId){
	// any lock request that all its single requests get deleted will return false.
	vector<SingleResourceLockRequest *> requestBatchFixed;
	for(unsigned i = 0 ; i < requestBatch.size() ; ++i){
		if(requestBatch.at(i)->applyNodeFailure(failedNodeId)){
			requestBatchFixed.push_back(requestBatch.at(i));
		}else{
			delete requestBatch.at(i);
		}
	}
	if(requestBatchFixed.size() > 0){
		requestBatch = requestBatchFixed;
		return true;
	}
	requestBatch.clear();
	return false;
}
bool ResourceLockRequest::operator==(const ResourceLockRequest & right){
	if(isBlocking != right.isBlocking){
		return false;
	}
	if(requestBatch.size() != right.requestBatch.size()){
		return false;
	}
	for(unsigned i = 0 ; i < requestBatch.size(); ++i){
		if(! ( *(requestBatch.at(i)) == *(right.requestBatch.at(i)))){
			return false;
		}
	}
	return true;
}

void * ResourceLockRequest::serialize(void * buffer) const{
	buffer = srch2::util::serializeVectorOfDynamicTypePointers(requestBatch, buffer);
	buffer = srch2::util::serializeFixedTypes(isBlocking, buffer);
	return buffer;
}
unsigned ResourceLockRequest::getNumberOfBytes() const{
	unsigned numberOfBytes = 0 ;
	numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypePointers(requestBatch);
	numberOfBytes += sizeof(bool);
	return numberOfBytes;
}
void * ResourceLockRequest::deserialize(void * buffer){
	buffer = srch2::util::deserializeVectorOfDynamicTypePointers(buffer, requestBatch);
	buffer = srch2::util::deserializeFixedTypes(buffer, isBlocking);
	return buffer;
}

string ResourceLockRequest::toString() const{
	stringstream ss;
	if(isBlocking){
		ss << "Blocking" << "%";
	}else{
		ss << "Non-Blocking" << "%";
	}
	for(unsigned i = 0; i < requestBatch.size(); ++i){
		ss << requestBatch.at(i)->toString() << "%";
	}
	return ss.str();
}

PendingLockRequest::PendingLockRequest(const NodeOperationId & requesterAddress,
		ShardingMessageType ackType,
		const unsigned priority,
		ResourceLockRequest * request){
	this->request = request;
	this->ackType = ackType;
	this->requesterAddress = requesterAddress;
	this->priority = priority;
	this->metadataVersionId = 0;
}

PendingLockRequest::PendingLockRequest(const PendingLockRequest & copy){
	this->request = copy.request;
	this->ackType = copy.ackType;
	this->requesterAddress = copy.requesterAddress;
	this->priority = copy.priority;
	this->metadataVersionId = copy.metadataVersionId;
}

bool PendingLockRequest::operator<(const PendingLockRequest & right) const{
	return (this->priority < right.priority) ||
			((this->priority == right.priority)  && (this->requesterAddress > right.requesterAddress));
}
bool PendingLockRequest::operator==(const PendingLockRequest & right) const{
	return (this->priority == right.priority) && (this->requesterAddress == right.requesterAddress);
}
bool PendingLockRequest::operator>(const PendingLockRequest & right) const{
	return !((*this == right) || (*this < right));
}

bool PendingLockRequest::applyNodeFailure(const unsigned failedNodeId){
	if(requesterAddress.nodeId == failedNodeId){
		return false;
	}
	return request->applyNodeFailure(failedNodeId);
}

string PendingLockRequest::toString() const{
	stringstream ss;
	ss << "Requester : " << requesterAddress.toString() << "%";
	ss << "Priority : " << priority << "%" ;
	ss << "MetadataVid : " << metadataVersionId << "%";
	if(ShardingNewNodeLockACKMessageType == ackType){
		ss << "New node lock request.%";
	}else{
		ss << "Normal lock request.%";
	}
	if(request == NULL){
		ss << "Reserved lock request.";
	}else{
		ss << "Request : ----%";
		ss << request->toString();
		ss << "--------%";
	}
	return ss.str();
}

PendingLockRequest::~PendingLockRequest(){
}

void PendingLockRequestBuffer::push(const PendingLockRequest & pendingRequest){

	// no request that already exists;
	for(unsigned i = 0 ; i < pendingRequests.size(); ++i){
		if(pendingRequests.at(i) == pendingRequest){
			return;
		}
	}

	pendingRequests.push_back(pendingRequest);
	std::push_heap(pendingRequests.begin(), pendingRequests.end());
}
// doesn't remove the request from the buffer, just to see what it is ...
PendingLockRequest PendingLockRequestBuffer::top(bool & hasMore){
	if(pendingRequests.size() == 0){
		hasMore = false;
		return PendingLockRequest();
	}
	hasMore = true;
	return pendingRequests.front();
}
bool PendingLockRequestBuffer::pop(){
	if(pendingRequests.size() == 0){
		return false;
	}
	std::pop_heap(pendingRequests.begin(), pendingRequests.end());
	pendingRequests.pop_back();
	return true;
}

void PendingLockRequestBuffer::update(const PendingLockRequest & pendingRequest){
	// look for a pending request that has the same nodeId and priority, and update it.
	// if it's not found, add it.
	for(unsigned i = 0 ; i < pendingRequests.size(); ++i){
		if(pendingRequest.ackType == pendingRequests.at(i).ackType &&
				pendingRequest.requesterAddress.nodeId == pendingRequest.requesterAddress.nodeId &&
				pendingRequest.priority == pendingRequest.priority){
			if(pendingRequests.at(i).request == NULL){ // if it's not null, we don't update it.
				pendingRequests.at(i) = pendingRequest;
				std::make_heap(pendingRequests.begin(), pendingRequests.end());
			}
			return;
		}
	}
	// we couldn't find anything to update;
	// so just insert.
	push(pendingRequest);
}

void PendingLockRequestBuffer::applyNodeFailure(const unsigned failedNodeId){
	vector<PendingLockRequest> pendingRequestsFixed;
	for(unsigned i = 0 ; i < pendingRequests.size(); ++i){
		if(pendingRequests.at(i).applyNodeFailure(failedNodeId)){
			pendingRequestsFixed.push_back(pendingRequests.at(i));
		}else{
			delete pendingRequests.at(i).request;
		}
	}
	if(pendingRequestsFixed.size() > 0){
		pendingRequests = pendingRequestsFixed;
		std::make_heap(pendingRequests.begin(), pendingRequests.end());
	}else{
		pendingRequests.clear();
	}
}

void PendingLockRequestBuffer::print() const{
	if(pendingRequests.size() == 0){
		cout << "Pending lock requests : empty." << endl;
		return;
	}
	vector<string> pendingHeaders;
	pendingHeaders.push_back("Info");
	vector<string> pendingLables;
	for(unsigned i = 0 ; i < pendingRequests.size(); ++i){
		pendingLables.push_back("");
	}
	srch2::util::TableFormatPrinter pendingTable("Pending lock requests", 80, pendingHeaders, pendingLables );
	pendingTable.printColumnHeaders();
	pendingTable.startFilling();
	for(unsigned i = 0 ; i < pendingRequests.size(); ++i){
		pendingTable.printNextCell(pendingRequests.at(i).toString());
	}
}

LockHoldersRepository::LockHoldersRepository(){
	//TODO
}
LockHoldersRepository::LockHoldersRepository(const LockHoldersRepository & repos){
	this->S_Holders = repos.S_Holders;
	this->U_Holders = repos.U_Holders;
	this->X_Holders = repos.X_Holders;
}

void * LockHoldersRepository::serialize(void * buffer) const{
	buffer = serializeHolderList(buffer, S_Holders);
	buffer = serializeHolderList(buffer, U_Holders);
	buffer = serializeHolderList(buffer, X_Holders);
	return buffer;
}
unsigned LockHoldersRepository::getNumberOfBytes() const{
	unsigned numberOfBytes = 0;
	numberOfBytes += getNumberOfBytesHolderList(S_Holders);
	numberOfBytes += getNumberOfBytesHolderList(U_Holders);
	numberOfBytes += getNumberOfBytesHolderList(X_Holders);
	return numberOfBytes;
}
void * LockHoldersRepository::deserialize(void * buffer){
	buffer = deserializeHolderList(S_Holders, buffer);
	buffer = deserializeHolderList(U_Holders, buffer);
	buffer = deserializeHolderList(X_Holders, buffer);
	return buffer;
}
bool LockHoldersRepository::isFree(const ClusterShardId & resource) const{
	if(S_Holders.find(resource) != S_Holders.end()){
		return false;
	}
	if(U_Holders.find(resource) != U_Holders.end()){
		return false;
	}
	if(X_Holders.find(resource) != X_Holders.end()){
		return false;
	}
	return true;
}

bool LockHoldersRepository::isPartitionLocked(const ClusterPID & pid) const{
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = S_Holders.begin();
			clusterShardIdItr != S_Holders.end(); ++clusterShardIdItr){
		ClusterPID rsrcPid(clusterShardIdItr->first.coreId, clusterShardIdItr->first.partitionId);
		if(rsrcPid == pid){
			return true;
		}
	}
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = U_Holders.begin();
			clusterShardIdItr != U_Holders.end(); ++clusterShardIdItr){
		ClusterPID rsrcPid(clusterShardIdItr->first.coreId, clusterShardIdItr->first.partitionId);
		if(rsrcPid == pid){
			return true;
		}
	}
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = X_Holders.begin();
			clusterShardIdItr != X_Holders.end(); ++clusterShardIdItr){
		ClusterPID rsrcPid(clusterShardIdItr->first.coreId, clusterShardIdItr->first.partitionId);
		if(rsrcPid == pid){
			return true;
		}
	}
	return false;
}

void LockHoldersRepository::getAllLockedResources(vector<ClusterShardId> & resources) const{
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = S_Holders.begin();
			clusterShardIdItr != S_Holders.end(); ++clusterShardIdItr){
		resources.push_back(clusterShardIdItr->first);
	}
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = U_Holders.begin();
			clusterShardIdItr != U_Holders.end(); ++clusterShardIdItr){
		resources.push_back(clusterShardIdItr->first);
	}
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = X_Holders.begin();
			clusterShardIdItr != X_Holders.end(); ++clusterShardIdItr){
		resources.push_back(clusterShardIdItr->first);
	}
}

void LockHoldersRepository::applyNodeFailure(const unsigned failedNodeId){
	vector<ClusterShardId> holdersToRemove;

	// S
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = S_Holders.begin();
			clusterShardIdItr != S_Holders.end(); ++clusterShardIdItr){
		vector<NodeOperationId> holdersFixed ;
		for(unsigned i = 0 ; i < clusterShardIdItr->second.size(); ++i){
			if(clusterShardIdItr->second.at(i).nodeId != failedNodeId){
				holdersFixed.push_back(clusterShardIdItr->second.at(i));
			}
		}
		if(holdersFixed.size() == 0){
			holdersToRemove.push_back(clusterShardIdItr->first);
		}else{
			S_Holders[clusterShardIdItr->first] = holdersFixed;
		}
	}
	for(unsigned i = 0 ; i < holdersToRemove.size(); ++i){
		S_Holders.erase(holdersToRemove.at(i));
	}

	// U
	holdersToRemove.clear();
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = U_Holders.begin();
			clusterShardIdItr != U_Holders.end(); ++clusterShardIdItr){
		vector<NodeOperationId> holdersFixed ;
		for(unsigned i = 0 ; i < clusterShardIdItr->second.size(); ++i){
			if(clusterShardIdItr->second.at(i).nodeId != failedNodeId){
				holdersFixed.push_back(clusterShardIdItr->second.at(i));
			}
		}
		if(holdersFixed.size() == 0){
			holdersToRemove.push_back(clusterShardIdItr->first);
		}else{
			U_Holders[clusterShardIdItr->first]  = holdersFixed;
		}
	}
	for(unsigned i = 0 ; i < holdersToRemove.size(); ++i){
		U_Holders.erase(holdersToRemove.at(i));
	}

	// X
	holdersToRemove.clear();
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator clusterShardIdItr = X_Holders.begin();
			clusterShardIdItr != X_Holders.end(); ++clusterShardIdItr){
		vector<NodeOperationId> holdersFixed ;
		for(unsigned i = 0 ; i < clusterShardIdItr->second.size(); ++i){
			if(clusterShardIdItr->second.at(i).nodeId != failedNodeId){
				holdersFixed.push_back(clusterShardIdItr->second.at(i));
			}
		}
		if(holdersFixed.size() == 0){
			holdersToRemove.push_back(clusterShardIdItr->first);
		}else{
			X_Holders[clusterShardIdItr->first]  = holdersFixed;
		}
	}
	for(unsigned i = 0 ; i < holdersToRemove.size(); ++i){
		X_Holders.erase(holdersToRemove.at(i));
	}
}

void LockHoldersRepository::printLockHolders(const map<ClusterShardId, vector<NodeOperationId> > & holders, const string & tableName) const{
	if(holders.size() == 0){
		cout << tableName << " : " << "empty." << endl;
		return;
	}
	vector<string> holderHeaders;
	holderHeaders.push_back("Holders list");
	vector<string> holderLabels;
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator holderItr = holders.begin(); holderItr != holders.end(); ++holderItr){
		holderLabels.push_back(holderItr->first.toString());
	}
	srch2::util::TableFormatPrinter lockTable(tableName, 120, holderHeaders, holderLabels);
	lockTable.printColumnHeaders();
	lockTable.startFilling();
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator holderItr = holders.begin(); holderItr != holders.end(); ++holderItr){
		stringstream ss;
		for(unsigned i = 0 ; i < holderItr->second.size(); ++i){
			if(i != 0){
				ss << ",";
			}
			ss << holderItr->second.at(i).toString() ;
		}
		lockTable.printNextCell(ss.str());
	}
}

void LockHoldersRepository::print() const{
	printLockHolders(S_Holders, "Resource S Lock% Holders");
	printLockHolders(U_Holders, "Resource U Lock% Holders");
	printLockHolders(X_Holders, "Resource X Lock% Holders");
}

bool LockHoldersRepository::operator==(const LockHoldersRepository & right) const{
	return (this->S_Holders == right.S_Holders)
			&& (this->U_Holders == right.U_Holders)
			&& (this->X_Holders == right.X_Holders);
}

void LockHoldersRepository::clear(){
	S_Holders.clear();
	U_Holders.clear();
	X_Holders.clear();
}

void * LockHoldersRepository::serializeHolderList(void * buffer,
		const map<ClusterShardId, vector<NodeOperationId> > & holders) const{
	buffer = srch2::util::serializeFixedTypes((unsigned)(holders.size()),buffer);
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator holderItr = holders.begin();
			holderItr != holders.end(); ++holderItr){
		buffer = holderItr->first.serialize(buffer);
		buffer = srch2::util::serializeVectorOfDynamicTypes(holderItr->second, buffer);
	}
	return buffer;
}
unsigned LockHoldersRepository::getNumberOfBytesHolderList(const map<ClusterShardId, vector<NodeOperationId> > & holders) const{
	unsigned numberOfBytes = 0;
	numberOfBytes += sizeof(unsigned);
	for(map<ClusterShardId, vector<NodeOperationId> >::const_iterator holderItr = holders.begin();
			holderItr != holders.end(); ++holderItr){
		numberOfBytes += holderItr->first.getNumberOfBytes();
		numberOfBytes += srch2::util::getNumberOfBytesVectorOfDynamicTypes(holderItr->second);
	}
	return numberOfBytes;
}
void * LockHoldersRepository::deserializeHolderList(map<ClusterShardId, vector<NodeOperationId> > & holders,
		void * buffer){
	unsigned sizeTemp;
	buffer = srch2::util::deserializeFixedTypes(buffer, sizeTemp);
	for(unsigned i = 0 ; i < sizeTemp ; ++i){
		ClusterShardId key;
		vector<NodeOperationId> value;
		buffer = key.deserialize(buffer);
		holders[key] = value;
		buffer = srch2::util::deserializeVectorOfDynamicTypes(buffer, holders[key]);
	}
	return buffer;
}


ResourceLockManager::ResourceLockManager(){
	this->lockHolders = new LockHoldersRepository();
	// use for deserialization
}
ResourceLockManager::ResourceLockManager(const ResourceLockManager & copy){
	this->lockHolders = new LockHoldersRepository(*(copy.lockHolders));
}


void ResourceLockManager::resolve(NewNodeLockNotification * notification){
	if(notification == NULL){
		ASSERT(false);
		return ;
	}
	vector<NodeId> allNodesUpToNewNode = notification->getNewNodeClusterView();

	if(allNodesUpToNewNode.size() < 1){ // at least this node must be in the list
		ASSERT(false);
		return;
	}
	Cluster_Writeview * writeview = ShardManager::getWriteview();

//	vector<const Node *> allNodes;
//	writeview->getAllNodes(allNodes);
//	for(vector<const Node *>::iterator nodeItr = allNodes.begin(); nodeItr != allNodes.end(); ++nodeItr){
//
//	}//TODO
	for(vector<NodeId>::iterator nodeItr = allNodesUpToNewNode.begin();
			nodeItr != allNodesUpToNewNode.end(); ++nodeItr){
		if(*nodeItr == ShardManager::getCurrentNodeId()){
			continue;
		}
		// 1. is this node in the writeview of this node ?
		if(writeview->nodes.find(*nodeItr) == writeview->nodes.end()){
			//add it to the list of nodes in writeview as NotArrived node
			writeview->setNodeState(*nodeItr, ShardingNodeStateNotArrived);
		}
		writeview->printNodes();
		// reserve place in waitingList
		PendingLockRequest tempReq(NodeOperationId(*nodeItr),
						ShardingNewNodeLockACKMessageType, LOCK_REQUEST_PRIORITY_NODE_ARRIVAL, NULL);
		pendingLockRequestBuffer.update(tempReq);
	}
	// put the lock requests of this new node in it's entry
	// first check if this node is already saved in the writeview and waiting list
	PendingLockRequest tempReq(notification->getSrc(),
				ShardingNewNodeLockACKMessageType, LOCK_REQUEST_PRIORITY_NODE_ARRIVAL, notification->getLockRequest());
	pendingLockRequestBuffer.update(tempReq);

	pendingLockRequestBuffer.print();
	// try to get the lock for the minimum nodeId of this waiting
	// list if it's prepared (it has lock requests and it's not already on the lock)
	tryPendingRequest();
}


void ResourceLockManager::resolve(LockingNotification * notification){
	if(notification == NULL){
		ASSERT(false);
		return;
	}
	ResourceLockRequest * lockRequest = notification->getLockRequest();

	bool needCommit ;
	resolveBatch(notification->getSrc(), LOCK_REQUEST_PRIORITY_LOAD_BALANCING, lockRequest, ShardingLockACKMessageType);
	return;
}

void ResourceLockManager::resolve(LockingNotification::RV_RELEASED * inputNotification){
	if(inputNotification == NULL){
		ASSERT(false);
		return;
	}
	boost::unique_lock<boost::mutex> bouncedNotificationsLock(readviewReleaseMutex);
	NodeId currentNodeId = ShardManager::getCurrentNodeId(); // TODO : if current node id changes, then this line is NOT thread safe
	vector<PendingLockRequest > newPendingNotifications;
	for(vector<PendingLockRequest >::iterator pendingNotifItr = pendingRVReleaseRequests.begin();
			pendingNotifItr != pendingRVReleaseRequests.end(); ++pendingNotifItr){
		if(inputNotification->getMetadataVersionId() >= pendingNotifItr->metadataVersionId){
			// we can send GRANTED to the requester
			sendAck(*pendingNotifItr , true);
			delete pendingNotifItr->request;
		}else{
			newPendingNotifications.push_back(*pendingNotifItr);
		}
	}
	pendingRVReleaseRequests = newPendingNotifications;
}


void ResourceLockManager::resolve(NodeFailureNotification * nodeFailureNotif){

	//1. first apply on lock repository
	this->lockHolders->applyNodeFailure(nodeFailureNotif->getFailedNodeID());

	//2. apply on waiting list
	this->pendingLockRequestBuffer.applyNodeFailure(nodeFailureNotif->getFailedNodeID());

	//3. move on RV_RELEASE pending requests and remove them if not valid anynmore
	readviewReleaseMutex.lock();
	vector<PendingLockRequest > rvReleaseFixed;
	for(unsigned i = 0 ; i < pendingRVReleaseRequests.size() ; ++i){
		if(! pendingRVReleaseRequests.at(i).applyNodeFailure(nodeFailureNotif->getFailedNodeID())){
			delete pendingRVReleaseRequests.at(i).request;
		}else{
			rvReleaseFixed.push_back(pendingRVReleaseRequests.at(i));
		}
	}
	pendingRVReleaseRequests = rvReleaseFixed;
	readviewReleaseMutex.unlock();
	// 4. commit the metadata
	ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();

	// 5. check if we can let any other request in ...
	tryPendingRequest();
}

void ResourceLockManager::setLockHolderRepository(LockHoldersRepository * shardLockHolders){
	if(shardLockHolders == NULL){
		ASSERT(false);
		return ;
	}
	if(this->lockHolders != NULL){
		delete this->lockHolders;
	}
	this->lockHolders = shardLockHolders;
}

LockHoldersRepository * ResourceLockManager::getLockHolderRepository() const{
	return this->lockHolders;
}
// this functions either executes all requests in this batch or non of them
// if the request is not blocking, the returns type tells us whether the lock was granted or not.
bool ResourceLockManager::resolveBatch(const NodeOperationId & requesterAddress, const unsigned priority,
		ResourceLockRequest * lockRequest, const ShardingMessageType & ackType){

	// 1. check to see whether we should GRANT or REJECT the lock
	//    or push it to the waiting list.
	if(! canGrantRequest(lockRequest)){
		// if it's blocking, we must push it to pending lock requests
		if(! lockRequest->isBlocking){
			// reject
			sendAck(PendingLockRequest(requesterAddress, ackType, priority, lockRequest), false);
			delete lockRequest;
			return false;
		}
		// blocking
		pendingLockRequestBuffer.push(PendingLockRequest(requesterAddress, ackType, priority, lockRequest));
		return true;// blocking requests will be granted eventually.
	}

    // 4. check if any of the lock requests where RELEASE, in that case we must check to see if we can
    //    apply a pending request.
    bool hasRelease = false;
    for(unsigned i = 0 ; i < lockRequest->requestBatch.size(); ++i){
        if(lockRequest->requestBatch.at(i)->requestType == ResourceLockRequestTypeRelease ||
                lockRequest->requestBatch.at(i)->requestType == ResourceLockRequestTypeDowngrade    ){
            hasRelease = true;
            break;
        }
    }

	bool needCommit = false;

	// 2. we can grant, apply the locks.
	executeBatch(lockRequest->requestBatch, needCommit);
	lockHolders->print();
	// 3. send back the ack or save it in RV release pending requests .
	if(needCommit){
		PendingLockRequest rvRelease(requesterAddress, ackType, priority, lockRequest);
		rvRelease.metadataVersionId = ShardManager::getWriteview()->versionId;
		readviewReleaseMutex.lock();
		pendingRVReleaseRequests.push_back(rvRelease);
		readviewReleaseMutex.unlock();
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
	}else{
		sendAck(PendingLockRequest(requesterAddress, ackType, priority, lockRequest), true);
		delete lockRequest;
	}


	if(hasRelease){
		tryPendingRequest();
	}
	return true;
}


LockHoldersRepository * ResourceLockManager::getShardLockHolders(){
	return lockHolders;
}

bool ResourceLockManager::isPartitionLocked(const ClusterPID & pid){
	return this->getShardLockHolders()->isPartitionLocked(pid);
}

void ResourceLockManager::getLockedPartitions(vector<ClusterPID> & lockedPartitions){
	vector<ClusterShardId> lockedResources;
	this->lockHolders->getAllLockedResources(lockedResources);
	for(unsigned i = 0 ; i < lockedResources.size(); ++i){
		if(std::find(lockedPartitions.begin(), lockedPartitions.end(),
				lockedResources.at(i).getPartitionId()) == lockedPartitions.end()){
			lockedPartitions.push_back(lockedResources.at(i).getPartitionId());
		}
	}
}


void ResourceLockManager::print() {
	lockHolders->print();

	pendingLockRequestBuffer.print();

	printRVReleasePendingRequests();
}

void ResourceLockManager::printRVReleasePendingRequests(){

	boost::unique_lock<boost::mutex> rvReleaseLock(readviewReleaseMutex);
	if(pendingRVReleaseRequests.size() == 0){
		cout << "Pending RV release lock requests : empty." << endl;
		return;
	}

	vector<string> pendingHeaders;
	pendingHeaders.push_back("Info");
	vector<string> pendingLables;
	for(unsigned i = 0 ; i < pendingRVReleaseRequests.size(); ++i){
		pendingLables.push_back("");
	}
	srch2::util::TableFormatPrinter pendingTable("RV Release %pending requests", 80, pendingHeaders, pendingLables );
	pendingTable.printColumnHeaders();
	pendingTable.startFilling();
	for(unsigned i = 0 ; i < pendingRVReleaseRequests.size(); ++i){
		pendingTable.printNextCell(pendingRVReleaseRequests.at(i).toString());
	}
}

// we know that we can apply this batch when we call this function.
void ResourceLockManager::executeBatch(const vector<SingleResourceLockRequest *> & requestBatch, bool & needCommit){

	// first go over all partitions affected by the batch and save their before value
	// partition id => partition state before execution
	map<ClusterPID, bool> partitionBeforeState;
	for(unsigned i = 0 ; i < requestBatch.size(); ++i){
		if(requestBatch.at(i)->requestType == ResourceLockRequestTypeLock ||
				requestBatch.at(i)->requestType == ResourceLockRequestTypeRelease){ // if partition's lock
			ClusterPID resourcePartitionId(requestBatch.at(i)->resource.coreId,
					requestBatch.at(i)->resource.partitionId);
			bool beforeExecuteState = isPartitionLocked(resourcePartitionId);
			partitionBeforeState[resourcePartitionId] = beforeExecuteState;
		}
	}

	// all requests can be granted.
	for(unsigned i = 0 ; i < requestBatch.size(); ++i){
		executeRequest(*(requestBatch.at(i)));
//		lockHolders->print();
	}

	needCommit = false;
	// check all affected partitions
	for(unsigned i = 0 ; i < requestBatch.size(); ++i){
		if(requestBatch.at(i)->requestType == ResourceLockRequestTypeLock ||
				requestBatch.at(i)->requestType == ResourceLockRequestTypeRelease){ // if partition's lock
			ClusterPID resourcePartitionId(requestBatch.at(i)->resource.coreId,
					requestBatch.at(i)->resource.partitionId);
			bool afterExecuteState = isPartitionLocked(resourcePartitionId);
			if(partitionBeforeState[resourcePartitionId] != afterExecuteState){
				needCommit = true;
				break;
			}
		}
	}
	return;
}


bool ResourceLockManager::canAcquireLock(const ClusterShardId & resource, ResourceLockType lockType){
	return canLock(lockType, resource, *(this->lockHolders));
}

void ResourceLockManager::tryPendingRequest(){
	bool hasMore;
	PendingLockRequest pendingRequest(pendingLockRequestBuffer.top(hasMore));
	if(! hasMore){
		return;// we have no pending request
	}

	if(pendingRequest.request == NULL){
		return; // this is just a reservation
	}
	if(! canGrantRequest(pendingRequest.request)){
		return;
	}

	pendingLockRequestBuffer.pop();
	bool needCommit;
	executeBatch(pendingRequest.request->requestBatch, needCommit);
	if(needCommit){
		pendingRequest.metadataVersionId = ShardManager::getWriteview()->versionId;
		readviewReleaseMutex.lock();
		pendingRVReleaseRequests.push_back(pendingRequest);
		readviewReleaseMutex.unlock();
		ShardManager::getShardManager()->getMetadataManager()->commitClusterMetadata();
		tryPendingRequest();
		return;
	}
	// we must send back the ack
	// use ackType to send back the ack
	sendAck(pendingRequest, true);
	delete pendingRequest.request;
}

void ResourceLockManager::sendAck(const PendingLockRequest & pendingRequest, const bool isGranted){
	switch (pendingRequest.ackType) {
		case ShardingNewNodeLockACKMessageType:
		{
			ASSERT(pendingRequest.requesterAddress.nodeId != ShardManager::getCurrentNodeId());
			NewNodeLockNotification::ACK * newNodeAck = new NewNodeLockNotification::ACK(this->getShardLockHolders());
			newNodeAck->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
			newNodeAck->setDest(pendingRequest.requesterAddress);
			ShardManager::getShardManager()->send(newNodeAck);
			delete newNodeAck;
			return;
		}
		case ShardingLockACKMessageType:
		{
			LockingNotification::ACK * lockingAck = new LockingNotification::ACK(isGranted);
			lockingAck->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
			lockingAck->setDest(pendingRequest.requesterAddress);
			if(lockingAck->getDest().nodeId == ShardManager::getCurrentNodeId()){
				ShardManager::getShardManager()->resolve(lockingAck);
			}else{
				ShardManager::getShardManager()->send(lockingAck);
			}
			delete lockingAck;
			return;
		}
		default:
		{
			ASSERT(false);
			return;
		}
	}
}

bool ResourceLockManager::canGrantRequest(const ResourceLockRequest * lockRequest){
	for(unsigned i = 0 ; i < lockRequest->requestBatch.size(); ++i){
		if(! canGrantRequest(*(lockRequest->requestBatch.at(i)))){
			return false;
		}
	}
	return true;
}

bool ResourceLockManager::canGrantRequest(const SingleResourceLockRequest & request){
	switch (request.requestType) {
		case ResourceLockRequestTypeLock:
			return canLock(request.lockType, request.resource, *lockHolders);
		case ResourceLockRequestTypeRelease:
			return canRelease(request.resource, *lockHolders);
		case ResourceLockRequestTypeUpgrade:
			return canUpgrade(request.resource, *lockHolders);
		case ResourceLockRequestTypeDowngrade:
			return canDowngrade(request.resource, *lockHolders);
		default:
			ASSERT(false);
			return false;
	}
	return false;
}

void ResourceLockManager::executeRequest(const SingleResourceLockRequest & request){
	ASSERT(canGrantRequest(request));
	switch (request.requestType) {
		case ResourceLockRequestTypeLock:
			lock(request.lockType, request.resource, request.holders, *lockHolders);
			return;
		case ResourceLockRequestTypeRelease:
			release(request.resource, request.holders, *lockHolders);
			return;
		case ResourceLockRequestTypeUpgrade:
			upgrade(request.resource, *lockHolders);
			return;
		case ResourceLockRequestTypeDowngrade:
			downgrade(request.resource, *lockHolders);
			return;
		default:
			ASSERT(false);
			return;
	}
}



// returns false if not found
void ResourceLockManager::release(const ClusterShardId & shardId, const vector<NodeOperationId> & holders, LockHoldersRepository & lockRepository){
	ASSERT(canRelease(shardId, lockRepository));
	// look up opId in all the three maps and remove it from them.
	// if not found, ignore.
	// S Locks
	for(unsigned holderIdx = 0 ; holderIdx < holders.size(); holderIdx++){
		if(lockRepository.S_Holders.find(shardId) != lockRepository.S_Holders.end()){
			vector<NodeOperationId> & sLockHolders = lockRepository.S_Holders[shardId];
			if(std::find(sLockHolders.begin(), sLockHolders.end(), holders.at(holderIdx)) != sLockHolders.end()){
				sLockHolders.erase(std::find(sLockHolders.begin(), sLockHolders.end(), holders.at(holderIdx)));
				if(sLockHolders.size() == 0){ // remove entry from map is resource is completely released
					lockRepository.S_Holders.erase(shardId);
				}
			}
		}
		if(lockRepository.U_Holders.find(shardId) != lockRepository.U_Holders.end()){
			vector<NodeOperationId> & uLockHolders = lockRepository.U_Holders[shardId];
			if(std::find(uLockHolders.begin(), uLockHolders.end(), holders.at(holderIdx)) != uLockHolders.end()){
				uLockHolders.erase(std::find(uLockHolders.begin(), uLockHolders.end(), holders.at(holderIdx)));
				if(uLockHolders.size() == 0){ // remove entry from map is resource is completely released
					lockRepository.U_Holders.erase(shardId);
				}
			}
		}

		if(lockRepository.X_Holders.find(shardId) != lockRepository.X_Holders.end()){
			vector<NodeOperationId> & xLockHolders = lockRepository.X_Holders[shardId];
			if(std::find(xLockHolders.begin(), xLockHolders.end(), holders.at(holderIdx)) != xLockHolders.end()){
				xLockHolders.erase(std::find(xLockHolders.begin(), xLockHolders.end(), holders.at(holderIdx)));
				if(xLockHolders.size() == 0){ // remove entry from map is resource is completely released
					lockRepository.X_Holders.erase(shardId);
				}
			}
		}
	}

	return;
}

bool ResourceLockManager::canRelease(const ClusterShardId & shardId, LockHoldersRepository & lockRepository){
	return true;
}

// returns false if it could not acquire the lock
void ResourceLockManager::lock(ResourceLockType lockType, const ClusterShardId & resource,
		const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository){
	ASSERT(canLock(lockType, resource, lockRepository));
	switch (lockType) {
		case ResourceLockType_S:
			lock_S(resource, lockHolders, lockRepository);
			return;
		case ResourceLockType_U:
			lock_U(resource, lockHolders, lockRepository);
			return;
		case ResourceLockType_X:
			lock_X(resource, lockHolders, lockRepository);
			return;
		default:
			ASSERT(false);
			return;
	}
	return;
}

bool ResourceLockManager::canLock(ResourceLockType lockType, const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	switch (lockType) {
		case ResourceLockType_S:
			return canLock_S(resource, lockRepository);
		case ResourceLockType_U:
			return canLock_U(resource, lockRepository);
		case ResourceLockType_X:
			return canLock_X(resource, lockRepository);
		default:
			ASSERT(false);
			return false;
	}
	return false;
}

void ResourceLockManager::upgrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	ASSERT(canUpgrade(resource, lockRepository));
	// Lock upgrade can be granted.
	// 1. add lockHolders to the xLock map
	lockRepository.X_Holders[resource] = lockRepository.U_Holders[resource];
	// 2. remove lockHolders from uLock list and erase the map entry
	lockRepository.U_Holders.erase(resource);
	return;
}
bool ResourceLockManager::canUpgrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	// 1. first, check if there is any U lock for this resource at all
	if(lockRepository.U_Holders.find(resource) == lockRepository.U_Holders.end()){
		ASSERT(false);
		return false;
	}

	// 2. third, check whether resource has any S lock (because it cannot have U or X locks)
	if(lockRepository.S_Holders.find(resource) != lockRepository.S_Holders.end()){
		// we have to wait for these S holders to leave ...
		return false;
	}

	return true;
}

void ResourceLockManager::downgrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	// 1. resource should be locked in X mode
	ASSERT(canDowngrade(resource, lockRepository));
	// 2. Lock upgrade can be granted.
	// 2.1. add lockHolders to the uLock map
	lockRepository.U_Holders[resource] = lockRepository.X_Holders[resource];
	// 2.2. remove lockHolders from xLock list and erase the map entry
	lockRepository.X_Holders.erase(resource);
}

bool ResourceLockManager::canDowngrade(const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	if(lockRepository.X_Holders.find(resource) == lockRepository.X_Holders.end()){
		return false;
	}
	return true;
}

bool ResourceLockManager::canLock_S(const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	if(lockRepository.X_Holders.find(resource) != lockRepository.X_Holders.end()){
		// we have X lock on it, not possible.
		return false;
	}
	return true;
}

void ResourceLockManager::lock_S(const ClusterShardId & resource, const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository){
	// 1. First, check whether this lock conflicts with existing locks
	// no conflict with S
	// no conflic with U
	// possible conflic with X
	ASSERT(canLock_S(resource, lockRepository));

	// 2. add lock holders to s list of this resource
	if(lockRepository.S_Holders.find(resource) == lockRepository.S_Holders.end()){
		lockRepository.S_Holders.insert(std::make_pair(resource ,vector<NodeOperationId>()));
	}
	for(unsigned i = 0; i < lockHolders.size(); ++i){
		lockRepository.S_Holders.find(resource)->second.push_back(lockHolders.at(i));
	}

//	lockRepository.S_Holders.find(resource)->second.insert(lockRepository.S_Holders.find(resource)->second.begin(), lockHolders.begin(), lockHolders.end());
	return;
}

bool ResourceLockManager::canLock_U(const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	if(lockRepository.U_Holders.find(resource) != lockRepository.U_Holders.end()){
		return false;
	}
	// possible conflic with X
	if(lockRepository.X_Holders.find(resource) != lockRepository.X_Holders.end()){
		return false;
	}
	return true;
}

void ResourceLockManager::lock_U(const ClusterShardId & resource, const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository){
	// 1. First, check whether this lock conflicts with any existing lock
	// no conflict with S
	// possible conflic with U
	ASSERT(canLock_U(resource, lockRepository));

	// 2. add lock holders to u list of this resource
	lockRepository.U_Holders[resource] = lockHolders;
	return;
}

bool ResourceLockManager::canLock_X(const ClusterShardId & resource, LockHoldersRepository & lockRepository){
	if(lockRepository.S_Holders.find(resource) != lockRepository.S_Holders.end()){
		return false;
	}
	// possible conflic with U
	if(lockRepository.U_Holders.find(resource) != lockRepository.U_Holders.end()){
		return false;
	}
	// possible conflic with X
	if(lockRepository.X_Holders.find(resource) != lockRepository.X_Holders.end()){
		return false;
	}
	return true;
}

void ResourceLockManager::lock_X(const ClusterShardId & resource, const vector<NodeOperationId> & lockHolders, LockHoldersRepository & lockRepository){
	// 1. First, check whether this lock conflicts with any existing lock
	// possible conflic with S
	ASSERT(canLock_X(resource, lockRepository));

	// 2. add lock holders to u list of this resource
	lockRepository.X_Holders[resource] = lockHolders;
	return;
}

}
}
