/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * SMCallBackHandler.h
 *
 *  Created on: Mar 15, 2015
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
