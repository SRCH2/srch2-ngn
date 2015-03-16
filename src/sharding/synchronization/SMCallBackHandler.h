/*
 * SMCallBackHandler.h
 *
 *  Created on: Mar 15, 2015
 *      Author: Surendra
 */

#ifndef __SHARDING_SMCALLBACKHANDLER_H__
#define __SHARDING_SMCALLBACKHANDLER_H__

#include "transport/TransportManager.h"
#include "sharding/sharding/ShardManager.h"
#include <utility>
#include <queue>

using namespace std;

namespace srch2 {
namespace httpwrapper {

#define MSG_QUEUE_ARRAY_SIZE 200000
#define FETCH_UNSIGNED(x) *((unsigned *)(x))

class SMCallBackHandler : public CallBackHandler{
public:
	/*
	 *   The function gets Message form TM and process it based on the message type.
	 *
	 *   1. Master heart beat message is stored with it arrival timestamp.
	 *   2. Client message is stored in a per message queue array.
	 *
	 */
	bool resolveMessage(Message * msg, NodeId node);

	/*
	 *  Constructor
	 */
	SMCallBackHandler(bool isMaster);

	/*
	 *  Remove message from node's queue.
	 */
	void removeMessageFromQueue(unsigned nodeId);

	/*
	 *  Remove message from node's queue.
	 */
	void getHeartBeatMessages(Message**msg);

	/*
	 *  Get heartbeat message's timestamp.
	 */
	std::time_t getHeartBeatMessageTime();

	void setHeartBeatMessageTime(std::time_t time);

	/*
	 *   Get queued message from a given node's queue
	 */
	void getQueuedMessages(Message**inputMessage, unsigned nodeId);

	~SMCallBackHandler() {}

private:
	// flag to indicate whether current node is master or not.
	bool isMaster;
	// Last time stamp when heart beat was received from master
	std::time_t heartbeatMessageTimeEntry;

	// Heart beat message
	Message* heartbeatMessage;

	boost::mutex hbLock;
public:
	// per Client entry
	struct MessageQ{
		std::queue<Message *> messageQueue;
		boost::mutex qGuard;
	} messageQArray[MSG_QUEUE_ARRAY_SIZE];

	MessageAllocator msgAllocator;
};

}}

#endif /* __SHARDING_SMCALLBACKHANDLER_H__ */
