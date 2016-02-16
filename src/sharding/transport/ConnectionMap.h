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
#ifndef __TRANSPORT_ROUTE_MAP_H__
#define __TRANSPORT_ROUTE_MAP_H__

#include "sharding/configuration/ConfigManager.h"
#include <netinet/in.h>
#include <vector>
#include <iterator>
#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "Message.h"
#include "MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {

const static char GREETING_MESSAGE[] = "SUCCESS";
const static char FAILED_GREETING_MESSAGE[] = "FAILURE";

struct NodeInfo {
	unsigned nodeId;
	unsigned ipaddress;
	unsigned communicationPort;
	char nodeName[1024];
};

class MessageBuffer {
public:
    Message* msg;
    bool lock;
private:
    unsigned readCount;
    unsigned possibleAvailableData;
public:
    char * partialMessageHeader;
    unsigned sizeOfPartialMsgHrd;
    unsigned numberOfRetriesWithZeroRead;
    unsigned timeToWait;
    MessageBuffer() : msg(NULL), lock(false), readCount(0), possibleAvailableData(0),
    		partialMessageHeader(new char[sizeof(Message)]), sizeOfPartialMsgHrd(0),
    		numberOfRetriesWithZeroRead(0), timeToWait(0){
        memset(partialMessageHeader, 0, sizeof(Message));
    }
    MessageBuffer(const MessageBuffer & msgBfr) : msg(msgBfr.msg), lock(msgBfr.lock), readCount(msgBfr.readCount),
            possibleAvailableData(msgBfr.possibleAvailableData),
            partialMessageHeader(new char[sizeof(Message)]), sizeOfPartialMsgHrd(msgBfr.sizeOfPartialMsgHrd),
            numberOfRetriesWithZeroRead(msgBfr.numberOfRetriesWithZeroRead), timeToWait(msgBfr.timeToWait){
        memcpy(partialMessageHeader, msgBfr.partialMessageHeader, sizeof(Message));
    }
    void finalizeMessageHeader(){
    	// make a message object and put it in msg pointer.
    	if(msg != NULL){
    		ASSERT(false);
    		return;
    	}
    	Message msgHeader;
    	msgHeader.populateHeader(partialMessageHeader);
    	msg = MessageAllocator().allocateMessage(msgHeader.getBodySize());
    	// body size is already set but it will be over-written in populate
    	msg->populateHeader(partialMessageHeader);

        memset(partialMessageHeader, 0, sizeof(Message));
        sizeOfPartialMsgHrd = 0;
    }

    Message * finalizeMessage(){
    	Message * completeMessage = msg;
    	msg = NULL;
    	setReadCount(0);
    	return completeMessage;
    }
    MessageBuffer & operator=(const MessageBuffer & msgBfr){
        if(this == &msgBfr){
            return *this;
        }
        msg = msgBfr.msg;
        lock = msgBfr.lock;
        readCount = msgBfr.readCount;
        possibleAvailableData = msgBfr.possibleAvailableData;
        partialMessageHeader = new char[sizeof(Message)];
        sizeOfPartialMsgHrd = msgBfr.sizeOfPartialMsgHrd;
        numberOfRetriesWithZeroRead = msgBfr.numberOfRetriesWithZeroRead;
        timeToWait = msgBfr.timeToWait;
        memcpy(partialMessageHeader, msgBfr.partialMessageHeader, sizeof(Message));
        return *this;
    }
    ~MessageBuffer(){delete[] partialMessageHeader;};
    void setReadCount(unsigned readCount){
    	this->readCount = readCount;
    	if(msg != NULL && msg->getBodySize() == readCount){
    		possibleAvailableData ++;
    	}
    }
    unsigned getReadCount() const{
    	return readCount;
    }
    void lockForRead(){
    	while(!__sync_bool_compare_and_swap(&lock, false, true));
    }
    void unlockRead(){
    	lock = false;
    }
    int getPossibleAvailableDataCount() const{
    	return possibleAvailableData;
    }
    void setPossibleAvailableDataCount(int possibleAvailableData) {
    	this->possibleAvailableData = possibleAvailableData;
    }
};

class Connection {
public:
  int fd;
  MessageBuffer buffer;
  bool sendLock;
  NodeId nodeId;
  void lockRead(){
  	while(!__sync_bool_compare_and_swap(&buffer.lock, false, true));
  }
  void lockWrite(){
  	while(!__sync_bool_compare_and_swap(&sendLock, false, true));
  }
  void unlockRead(){
  	buffer.lock = false;
  }
  void unlockWrite(){
	  sendLock = false;
  }
  Connection(int fd, NodeId nodeId) : fd(fd), sendLock(false), nodeId(nodeId) {}
  Connection():fd(-1), sendLock(false), nodeId(-1){}
};

typedef std::pair<sockaddr_in, NodeId> ConnectionInfo;

class ConnectionMap {
	// this map is from nodeID to Connection class
	std::map<NodeId, Connection> nodeConnectionMap;
	// destinations and nodeConnectionMap are changed by multiple threads
	mutable boost::shared_mutex _access;
public:
	void addNodeConnection(NodeId, int);
	Connection& getConnection(NodeId);
	bool isConnectionExist(NodeId);
	void acceptConnection(int fd, NodeId destinationNodeId);
	typedef std::map<NodeId, Connection>::iterator iterator;
	iterator begin();
	iterator end();
};

}}
#endif /* __TRANSPORT_ROUTE_MAP_H__ */
