/*
 * SMCallBackHandler.cpp
 *
 *  Created on: Mar 15, 2015
 */

#include "SMCallBackHandler.h"

namespace srch2 {
namespace httpwrapper {

///
///   SM Handler implementation start here.
///

/*
 *  Constructor
 */
SMCallBackHandler::SMCallBackHandler(bool isMaster) {
	this->isMaster = isMaster;
	heartbeatMessageTimeEntry = time(NULL);
	msgAllocator = MessageAllocator();
    heartbeatMessage = msgAllocator.allocateMessage(sizeof(NodeId));

}
/*
 *   The function gets Message form TM and process it based on the message type.
 *
 *   1. Master heart beat message is stored with it arrival timestamp.
 *   2. Client message is stored in a per message queue array.
 *
 */
bool SMCallBackHandler::resolveMessage(Message *message, NodeId node){
	switch(message->getType()){
	case HeartBeatMessageType:
	{
		if (!isMaster) {
			Logger::debug("SM-CB-getting heart-beat request from node %d", node);
			boost::mutex::scoped_lock lock(hbLock);
			heartbeatMessageTimeEntry = time(NULL);
			memcpy(heartbeatMessage, message, sizeof(Message) + sizeof(NodeId));
		} else {
			//cout << "Master should not receive heat beat request" << endl;
			ASSERT(false);
		}
		break;
	}
	case ClusterUpdateMessageType:
	case ClientStatusMessageType:
	case LeaderElectionAckMessageType:
	case LeaderElectionProposalMessageType:
	{
		if (message->getBodySize() >= sizeof(NodeId)) {
			unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
			/*
			 *  We have an array of message queues. Each nodes is assigned its
			 *  queue base on its nodeId ( nodeId % array_size). Once the node
			 *  gets its queue, we append its message to the queue so that it
			 *  can be processed by master's message handler.
			 */
			unsigned idx = nodeId % MSG_QUEUE_ARRAY_SIZE;
			Message * msg = msgAllocator.allocateMessage(message->getBodySize());
			memcpy(msg, message, sizeof(Message) + message->getBodySize());
			boost::mutex::scoped_lock lock(messageQArray[idx].qGuard);
			messageQArray[idx].messageQueue.push(msg);

		} else {
			Logger::warn("SM-CB: Incomplete message received!!");
			ASSERT(false);
		}
		break;
	}
	default:
		if (message->getBodySize() >= sizeof(NodeId)) {
			unsigned nodeId = FETCH_UNSIGNED(message->getMessageBody());
			Logger::warn("SM-CB: Bad message type received from node = %d", nodeId) ;
		}
		break;
	}
	return true;
}

/*
 *  Fetch heartbeat message from SMcallback Handler.
 */
void SMCallBackHandler::getHeartBeatMessages(Message**msg) {
	boost::mutex::scoped_lock lock(hbLock);
	*msg = msgAllocator.allocateMessage(sizeof(NodeId));
	memcpy(*msg, heartbeatMessage, sizeof(Message) + sizeof(NodeId));
}

/*
 *  Fetch recent heartbeat message's timestamp.
 */
std::time_t  SMCallBackHandler::getHeartBeatMessageTime(){
	boost::mutex::scoped_lock lock(hbLock);
	std::time_t copy = heartbeatMessageTimeEntry;
	return copy;
}

void SMCallBackHandler::setHeartBeatMessageTime(std::time_t time){
	boost::mutex::scoped_lock lock(hbLock);
	heartbeatMessageTimeEntry = time;
}

void SMCallBackHandler::getQueuedMessages(Message**inputMessage, unsigned masterNodeId){

	*inputMessage = NULL;
	unsigned index = masterNodeId % MSG_QUEUE_ARRAY_SIZE;
	if (this->messageQArray[index].messageQueue.size() > 0) {
		Message * queuedMessage = this->messageQArray[index].messageQueue.front();
		*inputMessage = msgAllocator.allocateMessage(queuedMessage->getBodySize());
		memcpy(*inputMessage, queuedMessage, sizeof(Message) + queuedMessage->getBodySize());
		removeMessageFromQueue(masterNodeId);
	}
}

/*
 *  Remove the processed message from Node's queue
 */
void SMCallBackHandler::removeMessageFromQueue(unsigned nodeId)
{
	unsigned idx = nodeId % MSG_QUEUE_ARRAY_SIZE;
	Message *ptr = NULL;
	std::queue<Message *> & ref = messageQArray[idx].messageQueue;
	{
		// x-auto-lock
		boost::mutex::scoped_lock lock(messageQArray[idx].qGuard);
		if (ref.size()) {
			ptr = ref.front();
			ref.pop();
		}
	}
	if (ptr)
		msgAllocator.deallocateByMessagePointer(ptr);
}

}}
