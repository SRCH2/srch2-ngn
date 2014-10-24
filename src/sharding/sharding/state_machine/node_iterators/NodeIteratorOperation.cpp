#include "NodeIteratorOperation.h"

#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "../../metadata_manager/Cluster_Writeview.h"
#include "../../ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


OrderedNodeIteratorOperation::OrderedNodeIteratorOperation(SP(ShardingNotification) request, ShardingMessageType resType,
		vector<NodeId> & participants, OrderedNodeIteratorListenerInterface * validatorObj):
	OperationState(this->getNextOperationId()), resType(resType){
	this->setParticipants(participants);
	this->request = request;
	ASSERT(! ! (this->request));
	this->validatorObj = validatorObj;
	this->participantsIndex = 0;
}
OrderedNodeIteratorOperation::~OrderedNodeIteratorOperation(){
};

Transaction * OrderedNodeIteratorOperation::getTransaction(){
	if(this->validatorObj != NULL){
		return this->validatorObj->getTransaction();
	}
	return OperationState::getTransaction();
}

OperationState * OrderedNodeIteratorOperation::entry(){
	ASSERT(this->participants.size() > 0);
	// ask the first node.
	this->participantsIndex = 0;
	return askNode(	this->participantsIndex);
}
// it returns this, or next state or NULL.
// if it returns NULL, we delete the object.
OperationState * OrderedNodeIteratorOperation::handle(SP(Notification) n){
	if(n == NULL){
		ASSERT(false);
		return NULL;
	}

	if(resType == n->messageType()){
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


OperationState * OrderedNodeIteratorOperation::handle(SP(ShardingNotification) notif){
	if(! validateResponse(notif)){
		return NULL;
	}
	// we must ask the next node
	this->participantsIndex ++;
	return askNode(this->participantsIndex);
}

OperationState * OrderedNodeIteratorOperation::handle(SP(NodeFailureNotification) notif){
	NodeId failedNode = notif->getFailedNodeID();
	if(this->validatorObj != NULL){
		if(this->validatorObj->shouldAbort(failedNode)){
			return NULL;
		}
	}
	bool failedTargetIndex = this->participants.size();
	for(unsigned p = 0 ; p < this->participants.size(); ++p){
		if(this->participants.at(p).nodeId == failedNode){
			failedTargetIndex = p;
			break;
		}
	}
	if(failedTargetIndex >= this->participants.size()){
		return this; // this node failure doesn't affect this operation
	}
	//1. remove this target
	this->participants.erase(this->participants.begin() + failedTargetIndex);
	if(failedTargetIndex < this->participantsIndex){ // we have passed this target
		//2. fix the iteration index
		this->participantsIndex -- ;
	}else if(failedTargetIndex == this->participantsIndex){ // we are waiting for the response of this target
		//2. fix the iteration index
		if(this->participants.size() == 0){
			// we are done, we shoud just call finalize
			if(this->validatorObj != NULL){
				this->validatorObj->end();
			}
			return NULL;
		}
		this->participantsIndex --;
		//3. now participantsIndex points to a new target, send a new request
		return askNode(this->participantsIndex);
	}//else{ // we have not reached to this target
		// nothing to do, we should just wait for the current response
	//}
	return this;
}

void OrderedNodeIteratorOperation::setParticipants(vector<NodeId> & participants){
	ASSERT(participants.size() > 0);
	this->participants.clear();
	std::sort(participants.begin(), participants.end());
	for(unsigned i = 0 ; i < participants.size() ; ++i){
		this->participants.push_back(NodeOperationId(participants.at(i)));
	}
}

bool OrderedNodeIteratorOperation::validateResponse(SP(ShardingNotification) response){
	if(this->validatorObj == NULL){
		return true;
	}
	vector<NodeId> newParticipants;
	bool conditionResult = this->validatorObj->condition(request, response, newParticipants);
	updateParticipantsList(newParticipants);
	return conditionResult;
}

string OrderedNodeIteratorOperation::getOperationName() const {
	return "NodeIteratorOperation, request " + string(getShardingMessageTypeStr(request->messageType()));
};

string OrderedNodeIteratorOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Total " << this->participants.size()
			<< " targets, now " <<
			this->participants.at(this->participantsIndex).toString().c_str() << " .";

	return ss.str();
};

OperationState * OrderedNodeIteratorOperation::askNode(const unsigned nodeIndex){
	// if all nodes are already iterated : call finalize from validator
	if(nodeIndex >= this->participants.size()){
		if(this->validatorObj != NULL){
			this->validatorObj->end();
		}
		return NULL;
	}
	const NodeOperationId & target = this->participants.at(nodeIndex);
	send(request, target);
	return this;
}

void OrderedNodeIteratorOperation::updateParticipantsList(vector<NodeId> newParticipants){
	std::sort(newParticipants.begin(), newParticipants.end());
	// 1. check to see if there are any new node
	//    new nodes must have larger ids
	NodeId largestParticipant = this->participants.at(this->participants.size() - 1).nodeId;

	// append new participants
	for(unsigned i = 0 ; i < newParticipants.size() ; ++i){
		if(newParticipants.at(i) > largestParticipant){
			// we should append this new participant to the end of list
			this->participants.push_back(NodeOperationId(newParticipants.at(i)));
		}
	}
}

}
}
