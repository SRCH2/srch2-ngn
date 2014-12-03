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
	if(this->validatorObj != NULL){
		this->setTransaction(this->validatorObj->getTransaction());
	}
	this->participantsIndex = 0;
	Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| Request(%s) Response(%s) - Consumer is %s",
			NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(),
			getShardingMessageTypeStr(request->messageType()) , getShardingMessageTypeStr(resType), validatorObj == NULL? "NULL" : validatorObj->getName().c_str());
}

OperationState * OrderedNodeIteratorOperation::entry(){
	__FUNC_LINE__
	if(this->validatorObj != NULL){
		this->setTransaction(this->validatorObj->getTransaction());
	}

	// Consistency check : all participants must be alive
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	vector<NodeOperationId> participantsCopy;
	for(unsigned i = 0 ; i < this->participants.size(); ++i){
		if(nodesWriteview->isNodeAlive(this->participants.at(i).nodeId)){
			participantsCopy.push_back(this->participants.at(i));
		}
	}
	this->participants = participantsCopy;
	nodesWriteview.reset();
	// Consistency check : participants must not be empty
	if(this->participants.empty()){
		return finalize();
	}
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
	case ShardingTimeoutNotificationMessageType:
		return handle(boost::dynamic_pointer_cast<TimeoutNotification>(n));
	default :
		ASSERT(false);
		return this;
	}
	return this;

}


OperationState * OrderedNodeIteratorOperation::handle(SP(ShardingNotification) notif){

	if(find(this->participants.begin(), this->participants.end(), notif->getSrc()) == this->participants.end()){
		return this;
	}
	if(targetResponsesMap.find(notif->getSrc()) != targetResponsesMap.end()){
		return this;
	}

	targetResponsesMap[notif->getSrc()] = notif;

	if(this->validatorObj != NULL){
		vector<NodeId> newParticipants;
		if(this->getTransaction()){
			this->getTransaction()->threadBegin(this->getTransaction());
		}

		bool conditionResult = this->validatorObj->condition(request, notif, newParticipants);

		if(this->getTransaction()){
			this->getTransaction()->threadEnd();
		}
		if(! conditionResult){
			this->setTransaction(SP(Transaction)());
			Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| consumer abort upon receiving response from node %s.",
					NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(),
					this->participants.at(this->participantsIndex).toString().c_str());
			/************* Thread Exit Point **********/
			return NULL;
		}
		appendParticipants(newParticipants);
	}
	// we must ask the next node
	this->participantsIndex ++;
	return askNode(this->participantsIndex);
}

OperationState * OrderedNodeIteratorOperation::handle(SP(NodeFailureNotification) notif){
	NodeId failedNode = notif->getFailedNodeID();

	map<NodeOperationId , SP(ShardingNotification)>::iterator targetItr = targetResponsesMap.end();
	for(targetItr = targetResponsesMap.begin(); targetItr != targetResponsesMap.end(); ++targetItr){
		if(targetItr->first.nodeId == failedNode){
			break;
		}
	}
	if(targetItr != targetResponsesMap.end()){
		targetResponsesMap.erase(targetItr);
	}

	unsigned failedTargetIndex = this->participants.size();
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
		Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| has passed node(%d) before its failure.",
				NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(),
				failedNode);
	}else if(failedTargetIndex == this->participantsIndex){ // we are waiting for the response of this target
        Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| was waiting for node(%d) when it failed.",
        		NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(),
        		failedNode);
		//2. fix the iteration index
		if(this->participants.size() == 0){
			// we are done, we shoud just call finalize
	        Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| list of participants is empty due to failure of node(%d): aborting.",
	        		NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(),
	        		failedNode);
			if(this->validatorObj != NULL){
				if(this->getTransaction()){
					this->getTransaction()->threadBegin(this->getTransaction());
				}
				map<NodeOperationId, SP(ShardingNotification) > _replies;
				this->validatorObj->end_(_replies, this->getOperationId());

				if(this->getTransaction()){
					this->getTransaction()->threadEnd();
				}
				this->setTransaction(SP(Transaction)());
			}
			return NULL;
		}
		if(this->participantsIndex > 0){
			this->participantsIndex --;
		}
		//3. now participantsIndex points to a new target, send a new request
		return askNode(this->participantsIndex);
	}//else{ // we have not reached to this target
		// nothing to do, we should just wait for the current response
	//}
	return this;
}


OperationState * OrderedNodeIteratorOperation::handle(SP(TimeoutNotification) notif){


	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();

	vector<NodeOperationId> participantsCopy = this->participants;
	for(unsigned p = 0 ; p < participantsCopy.size(); ++p){
		if(! nodesWriteview->isNodeAlive(participantsCopy.at(p).nodeId)){
			SP(NodeFailureNotification) nodeFailureNotif =
					SP(NodeFailureNotification)( new NodeFailureNotification(participantsCopy.at(p).nodeId));
			OperationState * result = handle(nodeFailureNotif);
			if( result == NULL){
				return NULL;
			}else if(result == this){
				continue;
			}else{
				ASSERT(false);
			}
		}
	}

	return this;
}


void OrderedNodeIteratorOperation::setParticipants(vector<NodeId> & participants){

	// Consistency check : there must be at least one participant
	if(participants.empty()){
		ASSERT(false);
		return;
	}

	// Consistency check : all participants must be alive
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	vector<NodeId> participantsCopy;
	for(int nodeIdx = 0; nodeIdx < participants.size(); ++nodeIdx){
		if(nodesWriteview->isNodeAlive(participants.at(nodeIdx))){
			participantsCopy.push_back(participants.at(nodeIdx));
		}
	}
	participants = participantsCopy;
	nodesWriteview.reset();

	// we can continue with this set of participants
	std::sort(participants.begin(), participants.end());

	this->participants.clear();
	for(unsigned i = 0 ; i < participants.size() ; ++i){
		this->participants.push_back(NodeOperationId(participants.at(i)));
	}
	stringstream ss;
	for(unsigned i =0 ; i < participants.size(); ++i){
		if(i != 0){
			ss << "|";
		}
		ss << participants.at(i);
	}
    Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| Participants : %s",
    		NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(), ss.str().c_str());
}

string OrderedNodeIteratorOperation::getOperationName() const {
	return "NodeIteratorOperation";
}

string OrderedNodeIteratorOperation::getOperationStatus() const {
	stringstream ss;
	ss << "Request " << string(getShardingMessageTypeStr(request->messageType()));
	ss << "Targets (" << this->participants.size() << "): " ;
	for(unsigned i = 0 ; i < this->participants.size() ; ++i){
		if(i > 0){
			ss << " | ";
		}
		if(this->participantsIndex == i){
			ss << "*";
		}
	    ss << this->participants.at(i).toString() ;
	}

	return ss.str();
}

OperationState * OrderedNodeIteratorOperation::finalize(){
	if(this->validatorObj != NULL){
		if(this->getTransaction()){
			this->getTransaction()->threadBegin(this->getTransaction());
		}
		this->validatorObj->end_(targetResponsesMap, this->getOperationId());
		if(this->getTransaction()){
			this->getTransaction()->threadEnd();
		}
		this->setTransaction(SP(Transaction)());
	}
	return NULL;
}

OperationState * OrderedNodeIteratorOperation::askNode(const unsigned nodeIndex){
	// if all nodes are already iterated : call finalize from validator
	if(nodeIndex >= this->participants.size()){
	    Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| Done. ",NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str());
	    return finalize();
	}

	Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| : asking the next node : %s",
			NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(),
			this->participants.at(nodeIndex).toString().c_str());
	const NodeOperationId & target = this->participants.at(nodeIndex);
	send(request, target);
	return this;
}

void OrderedNodeIteratorOperation::appendParticipants(vector<NodeId> newParticipants){
	std::sort(newParticipants.begin(), newParticipants.end());
	// 1. check to see if there are any new node
	//    new nodes must have larger ids
	NodeId largestParticipant = this->participants.at(this->participants.size() - 1).nodeId;

	// append new participants
	for(unsigned i = 0 ; i < newParticipants.size() ; ++i){
		if(newParticipants.at(i) > largestParticipant){
			// we should append this new participant to the end of list
			this->participants.push_back(NodeOperationId(newParticipants.at(i)));
		    Logger::sharding(Logger::Detail, "NodeIterator(opid=%s)| New participant (Node ID %d) added.",
		    		NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str(),
		    		newParticipants.at(i));
		}
	}
}

}
}
