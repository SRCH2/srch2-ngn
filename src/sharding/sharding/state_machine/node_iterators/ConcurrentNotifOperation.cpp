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


ConcurrentNotifOperation::ConcurrentNotifOperation(SP(ShardingNotification) request,
		ShardingMessageType resType,
		NodeId participant,
		NodeIteratorListenerInterface * consumer, bool expectResponse):
		OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	this->participants.push_back(NodeOperationId(participant));
	this->requests.push_back(request);
	this->consumer = consumer;
}

ConcurrentNotifOperation::ConcurrentNotifOperation(SP(ShardingNotification) request,
		ShardingMessageType resType,
		vector<NodeId> participants,
		NodeIteratorListenerInterface * consumer , bool expectResponse):
			OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	for(unsigned p = 0 ; p < participants.size(); p++){
		this->participants.push_back(NodeOperationId(participants.at(p)));
		this->requests.push_back(request);
	}
	this->consumer = consumer;
}
ConcurrentNotifOperation::ConcurrentNotifOperation(ShardingMessageType resType,
		vector<std::pair<SP(ShardingNotification) , NodeId> > participants,
		NodeIteratorListenerInterface * consumer , bool expectResponse ):
			OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	for(unsigned p = 0 ; p < participants.size(); p++){
		this->participants.push_back(NodeOperationId(participants.at(p).second));
		this->requests.push_back(participants.at(p).first);
	}
	this->consumer = consumer;
};

ConcurrentNotifOperation::~ConcurrentNotifOperation(){
	if(consumer == NULL){
		if(this->expectResponse){
			targetResponsesMap.clear();
		}
		return;
	}
};

Transaction * ConcurrentNotifOperation::getTransaction(){
	if(this->consumer != NULL){
		this->consumer->getTransaction();
	}
	return OperationState::getTransaction();
}

OperationState * ConcurrentNotifOperation::entry(){
	if(this->participants.size() == 0){
		return finalize();
	}
	for(unsigned p = 0 ; p < this->participants.size(); ++p){
		// 1. send the request to the target
		send(this->requests.at(p), this->participants.at(p));
		// 2. handle targetResponsesMap
		if(! this->expectResponse){
			this->targetResponsesMap[this->participants.at(p)] = SP(ShardingNotification)();
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
OperationState * ConcurrentNotifOperation::handle(SP(Notification) n){
	if(n == NULL){
		ASSERT(false);
		return NULL;
	}

	if(resType){
		return handle(boost::dynamic_pointer_cast<ShardingNotification>(n));
	}

	switch(n->messageType()){
	case ShardingNodeFailureNotificationMessageType:
		return handle(boost::dynamic_pointer_cast<NodeFailureNotification>(n));
	default :
		ASSERT(false);
		return this;
	}
	return this;
}

OperationState * ConcurrentNotifOperation::handle(SP(NodeFailureNotification)  notif){
	NodeId failedNode = notif->getFailedNodeID();

	if(consumer != NULL){
		if(consumer->shouldAbort(failedNode)){
			return NULL;
		}
	}

	if(targetResponsesMap.find(failedNode) != targetResponsesMap.end()){
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

OperationState * ConcurrentNotifOperation::handle(SP(ShardingNotification) response){
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

}
}
