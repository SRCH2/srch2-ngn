#include "ConcurrentNotifOperation.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../../metadata_manager/Cluster_Writeview.h"
#include "../../ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


ConcurrentNotifOperation::ConcurrentNotifOperation(ShardingNotification * request,
		ShardingMessageType resType,
		NodeId participant,
		NodeIteratorListenerInterface * consumer = NULL, bool expectResponse = true):
		OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	this->participants.push_back(NodeOperationId(participant));
	this->requests.push_back(request);
	this->consumer = consumer;
	if(this->consumer != NULL){
		this->connectDeletePathToParent(this->consumer);
	}
}

ConcurrentNotifOperation::ConcurrentNotifOperation(ShardingNotification * request,
		ShardingMessageType resType,
		vector<NodeId> participants,
		NodeIteratorListenerInterface * consumer = NULL, bool expectResponse = true):
			OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	for(unsigned p = 0 ; p < participants.size(); p++){
		this->participants.push_back(NodeOperationId(participants.at(p)));
		this->requests.push_back(request);
	}
	this->consumer = consumer;
	if(this->consumer != NULL){
		this->connectDeletePathToParent(this->consumer);
	}
}
ConcurrentNotifOperation::ConcurrentNotifOperation(ShardingMessageType resType,
		vector<std::pair<ShardingNotification * , NodeId> > participants,
		NodeIteratorListenerInterface * consumer = NULL, bool expectResponse = true):
			OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	for(unsigned p = 0 ; p < participants.size(); p++){
		this->participants.push_back(NodeOperationId(participants.at(p).second));
		this->requests.push_back(participants.at(p).first);
	}
	this->consumer = consumer;
	if(this->consumer != NULL){
		this->connectDeletePathToParent(this->consumer);
	}
};

ConcurrentNotifOperation::~ConcurrentNotifOperation(){
	if(consumer == NULL){
		if(this->expectResponse){
			for(typename map<NodeOperationId , ShardingNotification *>::iterator targetResItr =
					targetResponsesMap.begin(); targetResItr != targetResponsesMap.end(); ++targetResItr){
				if(targetResItr->second != NULL){
					delete targetResItr->second;
				}
			}
			targetResponsesMap.clear();
		}
		return;
	}
};

OperationState * ConcurrentNotifOperation::entry(){
	if(this->participants.size() == 0){
		return finalize();
	}
	for(unsigned p = 0 ; p < this->participants.size(); ++p){
		// 1. send the request to the target
		sendRequest(this->requests.at(p), this->participants.at(p));
		// 2. handle targetResponsesMap
		if(! this->expectResponse){
			this->targetResponsesMap[this->participants.at(p)] = NULL;
		}
	}
	if(! this->expectResponse){
		ASSERT(checkFinished());
		return finalize();
	}
	return this;
}
// it returns this, or next state or NULL.
// if it returns NULL, we delete the object.
OperationState * ConcurrentNotifOperation::handle(Notification * n){
	if(n == NULL){
		ASSERT(false);
		return NULL;
	}
	switch(n->messageType()){
	case ShardingNodeFailureNotificationMessageType:
		return handle((NodeFailureNotification *) n);
	case resType:
		return handle((ShardingNotification *) n);
	default :
		ASSERT(false);
		return this;
	}
	return this;
}

OperationState * ConcurrentNotifOperation::handle(NodeFailureNotification *  notif){
	NodeId failedNode = notif->getFailedNodeID();

	if(consumer != NULL){
		if(consumer->shouldAbort(failedNode)){
			return NULL;
		}
	}

	if(targetResponsesMap.find(failedNode) != targetResponsesMap.end()){
		if(targetResponsesMap.find(failedNode)->second != NULL){
			delete targetResponsesMap.find(failedNode)->second;
		}
		targetResponsesMap.erase(failedNode);
	}
	for(unsigned p = 0 ; p < this->participants.size(); ++p){
		if(participants.at(p).nodeId == failedNode){
			this->participants.erase(this->participants.begin() + p);
			p --;
		}
	}
	if(checkFinished()){
		return finalize();
	}
	return this;
}

OperationState * ConcurrentNotifOperation::handle(ShardingNotification * response){
	// 1. save the response in the map
	targetResponsesMap[response->getSrc()] = response;
	// 2. check if we are done
	if(checkFinished()){
		return finalize();
	}
	return this;
}

string ConcurrentNotifOperation::getOperationName() const {
	return "ConcurrentNotifOperation, request " + string(getShardingMessageTypeStr(requests.at(0)->messageType()));
}
string ConcurrentNotifOperation::getOperationStatus() const {
	return "operation status";//TODO
}

OperationState * ConcurrentNotifOperation::finalize(){
	if(consumer == NULL){
		return NULL;
	}
	this->consumer->end_(this->targetResponsesMap);
	return NULL;
}

bool ConcurrentNotifOperation::checkFinished(){
	for(unsigned p = 0 ; p < this->participants.size(); ++p){
		if(this->targetResponsesMap.find(this->participants.at(p)) == this->targetResponsesMap.end()){
			return false;
		}
	}
	return true;
}

void ConcurrentNotifOperation::sendRequest(ShardingNotification * request, const NodeOperationId & target){
	const NodeId currentNodeId = ShardManager::getCurrentNodeId();
	if(target.nodeId == currentNodeId){
		request->setDest(target);
		request->setSrc(NodeOperationId(currentNodeId, this->getOperationId()));
		ShardManager::getShardManager()->resolveLocal(request);
	}else{
		send(request, target);
	}
}

}
}
