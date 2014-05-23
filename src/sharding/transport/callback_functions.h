#ifndef __TRANSPORT_CALLBACK_FUNCTIONS_H__
#define __TRANSPORT_CALLBACK_FUNCTIONS_H__

#include<unistd.h>
#include<errno.h>
#include <event.h>
#include "TransportManager.h"
#include "MessageBuffer.h"

using namespace srch2::httpwrapper;

namespace srch2 {
namespace httpwrapper {
struct TransportCallback {
  TransportManager *const tm;
  Connection *const conn;
  struct event* ev;
  const struct event_base *const base;

  TransportCallback(TransportManager *tm, Connection *c, event* e, 
      event_base* b) : tm(tm), conn(c), ev(e), base(b) {}
  TransportCallback() : tm(NULL), conn(NULL), ev(NULL), base(NULL) {}
};
}}

/*
 * This function reads the stream until it finds the next valid message
 * Each message starts with a special value called magic number
 */
bool findNextMagicNumberAndReadMessageHeader(Message *const msg,  int fd) {
	while(true) {
		int readRtn = recv(fd, (void*) msg, sizeof(Message), MSG_DONTWAIT);

		if(readRtn == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) return false;

			//v1: handle  error
			return false;
      }

		if(readRtn < sizeof(Message)) {
			//v1: broken message boundary == seriously bad
			return false;
		}

		//TODO:checkMagicNumbers

		return true;
	}

	//ASSERT(false);
	return false;
}

Message* readRestOfMessage(MessageAllocator& messageAllocator,
		int fd, Message *const msgHeader, int *readCount) {
	Message *msg= messageAllocator.allocateMessage(msgHeader->getBodySize());
	int rv = recv(fd, Message::getBodyPointerFromMessagePointer(msg), msgHeader->getBodySize(), MSG_DONTWAIT);

	if(rv == -1) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) return NULL;

		//v1: handle error
		return NULL;
	}

	*readCount = rv;
	memcpy(msg, msgHeader, sizeof(Message));

	return msg;
}

bool readPartialMessage(int fd, MessageBuffer& buffer) {
	int toRead = buffer.msg->getBodySize() - buffer.readCount;
	if(toRead == 0) {
		//strangely we don't need to read anything;)
		return false;
	}

	int readReturnValue = recv(fd, 
			Message::getBodyPointerFromMessagePointer(buffer.msg) + buffer.readCount, toRead, MSG_DONTWAIT);
	if(readReturnValue < 0) {
		//TODO: handle errors
		return true;
	}

	buffer.readCount += readReturnValue;

	return (buffer.msg->getBodySize() == buffer.readCount);
}


void recieveMessage(int fd, TransportCallback *cb) {
	if( fd != cb->conn->fd) {
		//major error
		return;
	}

	MessageBuffer& b = cb->conn->buffer;
	TransportManager *tm = cb->tm;
	while(!__sync_bool_compare_and_swap(&b.lock, false, true));

	if(b.msg == NULL) {
		Message msgHeader;

		if(!findNextMagicNumberAndReadMessageHeader(&msgHeader, fd)){
			// there is some sort of error in the stream so we can't
			// get the next message
			b.lock = false;
			return;
		}

		// sets the distributedTime of TM to the maximum time received by a message
		// in a thread safe fashion
		while(true) {
			MessageTime_t time = tm->getDistributedTime();
			//check if time needs to be incremented
			if(msgHeader.getTime() < time &&
					/*zero break*/ time - msgHeader.getTime() < UINT_MAX/2 ) break;
			//make sure time did not change
			if(__sync_bool_compare_and_swap(
					&tm->getDistributedTime(), time, msgHeader.getTime())) break;
		}

		// we have some types of message like GetInfoCommandInfo that currently don't
		// have any information in them and therefore their body size is zero
		if(msgHeader.getBodySize() != 0){
			if(!(b.msg = readRestOfMessage(*(tm->getMessageAllocator()),
					fd, &msgHeader, &b.readCount))) {
            b.lock = false;
            return;
         }

			if(b.readCount != b.msg->getBodySize()) {
				b.lock = false;
				return;
			}
		}else{
		    b.msg= tm->getMessageAllocator()->allocateMessage(msgHeader.getBodySize());
		    memcpy(b.msg, &msgHeader, sizeof(Message));
		}
	} else {
		if(!readPartialMessage(fd, b)) {
			b.lock = false;
			return;
		}
	}

	Message* msg = b.msg;
	b.msg = NULL;
	b.lock = false;

	if(msg->isReply()) {
		tm->getMsgs()->resolve(msg);
		return;
	} else if(msg->isInternal()) { // receiving a message which

		if(msg->isNoReply()){
			// This msg comes from another node and does not need a reply
			// it comes from a broadcast or route with no callback
			tm->getInternalTrampoline()->notifyNoReply(msg);
			// and delete the msg
			tm->getMessageAllocator()->deallocateByMessagePointer(msg);
			return;
		}
		Message* replyMessage = tm->getInternalTrampoline()->notifyWithReply(msg);
		if(replyMessage != NULL) {
			replyMessage->setInitialTime(msg->getTime());
			replyMessage->setReply()->setInternal();
			tm->route(fd, replyMessage);
			tm->getMessageAllocator()->deallocateByMessagePointer(msg);
			tm->getMessageAllocator()->deallocateByMessagePointer(replyMessage);
			return;
		}
	} else {
		tm->getSmHandler()->notifyWithReply(msg);
	}

	tm->getMessageAllocator()->deallocateByMessagePointer(msg);
}

/*
 * This callback function is called from transport layer upon receiving an internal DP/SM messages
 */
void cb_recieveMessage(int fd, short eventType, void *arg) {
	TransportCallback* cb = (TransportCallback*) arg;
  recieveMessage(fd, cb);
  event_add(cb->ev, NULL);
}

#endif /* __TRANSPORT_CALLBACK_FUNCTIONS_H__ */
