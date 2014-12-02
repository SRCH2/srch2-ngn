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
	if(this->consumer != NULL){
		this->setTransaction(this->consumer->getTransaction());
	}
	Logger::sharding(Logger::Detail,
			"NodeAggregator(opid=%s)| sending request(%s) to nodes %s and aggregating response(%s). Consumer is %s. ExpectResponse(%s)"
			, NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str()
			, getShardingMessageTypeStr(request->messageType()), this->participants.at(0).toString().c_str(),
			getShardingMessageTypeStr(resType), consumer == NULL ? "NULL" : consumer->getName().c_str(), expectResponse ? "YES" : "NO");
}

ConcurrentNotifOperation::ConcurrentNotifOperation(SP(ShardingNotification) request,
		ShardingMessageType resType,
		NodeIteratorListenerInterface * consumer, bool expectResponse):
		OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	stringstream ss;
	vector<NodeId> arrivedNodes;
	nodesWriteview->getArrivedNodes(arrivedNodes, true);
	for(unsigned p = 0 ; p < arrivedNodes.size(); p++){
		if(p != 0){
			ss << " | ";
		}
		this->participants.push_back(NodeOperationId(arrivedNodes.at(p)));
		this->requests.push_back(request);
		ss << NodeOperationId(participants.at(participants.size())).toString();
	}
	nodesWriteview.reset();
	this->consumer = consumer;
	if(this->consumer != NULL){
		this->setTransaction(this->consumer->getTransaction());
	}
	Logger::sharding(Logger::Detail, "NodeAggregator(opid=%s)| sending request(%s) to nodes %s and aggregating response(%s). Consumer is %s. ExpectResponse(%s)"
			, NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str()
			, getShardingMessageTypeStr(request->messageType()), ss.str().c_str(),
			getShardingMessageTypeStr(resType), consumer == NULL ? "NULL" : consumer->getName().c_str(), expectResponse ? "YES" : "NO");
}

ConcurrentNotifOperation::ConcurrentNotifOperation(SP(ShardingNotification) request,
		ShardingMessageType resType,
		vector<NodeId> participants,
		NodeIteratorListenerInterface * consumer , bool expectResponse):
			OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){
	if(participants.empty()){
		ASSERT(false);
	}
	stringstream ss;
	for(unsigned p = 0 ; p < participants.size(); p++){
		if(p != 0){
			ss << " | ";
		}
		this->participants.push_back(NodeOperationId(participants.at(p)));
		this->requests.push_back(request);
		ss << NodeOperationId(participants.at(p)).toString();
	}
	this->consumer = consumer;
	if(this->consumer != NULL){
		this->setTransaction(this->consumer->getTransaction());
	}
	Logger::sharding(Logger::Detail, "NodeAggregator(opid=%s)| Sending %s to %s . Consumer is %s. ExpectResponse(%s)"
			, NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str()
			, getShardingMessageTypeStr(request->messageType()), ss.str().c_str(),  consumer == NULL ? "NULL" : consumer->getName().c_str(), expectResponse ? "YES" : "NO");
}
ConcurrentNotifOperation::ConcurrentNotifOperation(ShardingMessageType resType,
		vector<std::pair<SP(ShardingNotification) , NodeId> > participants,
		NodeIteratorListenerInterface * consumer , bool expectResponse ):
			OperationState(this->getNextOperationId()),resType(resType), expectResponse(expectResponse){

	if(participants.empty()){
		ASSERT(false);
	}
	stringstream ss;
	for(unsigned p = 0 ; p < participants.size(); p++){
		if(p != 0){
			ss << " | ";
		}
		this->participants.push_back(NodeOperationId(participants.at(p).second));
		this->requests.push_back(participants.at(p).first);
		ss << NodeOperationId(participants.at(p).second).toString() << " to " << getShardingMessageTypeStr(participants.at(p).first->messageType()) ;
	}
	this->consumer = consumer;
	if(this->consumer != NULL){
		this->setTransaction(this->consumer->getTransaction());
	}
	Logger::sharding(Logger::Detail, "NodeAggregator(opid=%s)| Sending %s . Consumer is %s. ExpectResponse(%s)"
			, NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str()
			,ss.str().c_str(),  consumer == NULL ? "NULL" : consumer->getName().c_str(), expectResponse ? "YES" : "NO");
}

ConcurrentNotifOperation::~ConcurrentNotifOperation(){
	if(consumer == NULL){
		if(this->expectResponse){
			targetResponsesMap.clear();
		}
		return;
	}
}

OperationState * ConcurrentNotifOperation::entry(){
	__FUNC_LINE__
	if(this->consumer != NULL){
		this->setTransaction(this->consumer->getTransaction());
	}
	Logger::sharding(Logger::Detail, "NodeAggregator| entry");

	if(this->participants.empty()){
		Logger::sharding(Logger::Warning, "NodeAggregator(opid=%s)| Finalizing because !! NO NODES !! . Consumer is %s. ExpectResponse(%s)"
				, NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str()
				, consumer == NULL ? "NULL" : consumer->getName().c_str(), expectResponse ? "YES" : "NO");
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

OperationState * ConcurrentNotifOperation::handle(SP(NodeFailureNotification)  notif){
	NodeId failedNode = notif->getFailedNodeID();
	map<NodeOperationId , SP(ShardingNotification)>::iterator responseItr = targetResponsesMap.end();
	for(responseItr = targetResponsesMap.begin(); responseItr != targetResponsesMap.end(); ++responseItr){
		if(responseItr->first.nodeId == failedNode){
			break;
		}
	}
	if(responseItr != targetResponsesMap.end()){
		targetResponsesMap.erase(responseItr);
	}
	for(vector<NodeOperationId>::iterator participantItr = this->participants.begin();
			participantItr != this->participants.end();){
		if(participantItr->nodeId == failedNode){
			participantItr = this->participants.erase(participantItr);
		}else{
			++participantItr;
		}
	}
	if(checkFinished()){
		Logger::sharding(Logger::Detail, "NodeAggregator(opid=%s)| terminated due to node failure."
				, NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str());
		return finalize();
	}
	return this;
}

OperationState * ConcurrentNotifOperation::handle(SP(TimeoutNotification)  notif){

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

OperationState * ConcurrentNotifOperation::handle(SP(ShardingNotification) response){
	// 1. save the response in the map
	if(targetResponsesMap.find(response->getSrc()) != targetResponsesMap.end()){
		ASSERT(false);
	}else{
		targetResponsesMap[response->getSrc()] = response;
	}
	// 2. check if we are done
	if(checkFinished()){
		return finalize();
	}
	return this;
}

string ConcurrentNotifOperation::getOperationName() const {
	return "ConcurrentNotifOperation";
}
string ConcurrentNotifOperation::getOperationStatus() const {
    stringstream ss;
    ss << "Request " << string(getShardingMessageTypeStr(requests.at(0)->messageType()));
    ss << ", Replies status :";
    for(unsigned p = 0 ; p < this->participants.size(); ++p){
    	if(p != 0){
    		ss << " | ";
    	}
        if(this->targetResponsesMap.find(this->participants.at(p)) == this->targetResponsesMap.end()){
            ss << "W";
        }
        ss << this->participants.at(p).toString() ;
    }
	return ss.str();
}

OperationState * ConcurrentNotifOperation::finalize(){

	Logger::sharding(Logger::Detail, "NodeAggregator(opid=%s)| Done." ,
			NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()).toString().c_str());
    if(consumer == NULL){
		return NULL;
	}
	if(this->getTransaction()){
		this->getTransaction()->threadBegin(this->getTransaction());
	}
	this->consumer->end_(this->targetResponsesMap, this->getOperationId());
	if(this->getTransaction()){
		this->getTransaction()->threadEnd();
	}
	this->setTransaction(SP(Transaction)());
	/***************** Thread Exit Point *************/
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
