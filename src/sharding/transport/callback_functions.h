#ifndef __TRANSPORT_CALLBACK_FUNCTIONS_H__
#define __TRANSPORT_CALLBACK_FUNCTIONS_H__

#include<unistd.h>
#include<errno.h>
#include "TransportManager.h"
#include "MessageBuffer.h"

using namespace srch2::httpwrapper;

namespace srch2 {
namespace httpwrapper {
struct TransportCallback {
	TransportManager *tm;
	Connection* conn;

	TransportCallback(TransportManager *tm, Connection *c) : tm(tm), conn(c) {}
};
}}

/*
 * This function reads the stream until it finds the next valid message
 * Each message starts with a special value called magic number
 */
bool findNextMagicNumberAndReadMessageHeader(Message *const msg,  int fd) {
	while(true) {
		int readRtn = read(fd, (void*) msg, sizeof(Message));

		if(readRtn == -1) {
			if(errno == EAGAIN || errno == EWOULDBLOCK) return false;

			//v1: handle  error
			return false;
		}

		if(readRtn < sizeof(Message)) {
			//v1: broken message boundary == seriously bad
			continue;
		}

		//TODO:checkMagicNumbers

		return true;
	}

	//ASSERT(false);
	return false;
}

Message* readRestOfMessage(MessageAllocator& messageAllocator,
		int fd, Message *const msgHeader, int *readCount) {
	Message *msg= messageAllocator.allocateMessage(msgHeader->bodySize);
	*readCount= read(fd, msg->buffer, msgHeader->bodySize);

	memcpy(msg, msgHeader, sizeof(Message));

	return msg;
}

bool readPartialMessage(int fd, MessageBuffer& buffer) {
	int toRead = buffer.msg->bodySize - buffer.readCount;
	if(toRead == 0) {
		//strangely we don't need to read anything;)
		return true;
	}

	int readReturnValue = read(fd, buffer.msg->buffer, toRead);
	if(readReturnValue < 0) {
		//TODO: handle errors
	}

	buffer.readCount -= readReturnValue;

	return (readReturnValue == toRead);
}

/*
 * This callback function is called from transport layer upon receiving an internal DP/SM messages
 */
void cb_recieveMessage(int fd, short eventType, void *arg) {

	TransportCallback* cb = (TransportCallback*) arg;

	if( fd != cb->conn->fd) {
		//major error
		return;
	}

	MessageBuffer& b = cb->conn->buffer;
	TransportManager *tm = cb->tm;
	while(__sync_bool_compare_and_swap(&b.lock, false, true));

	if(b.msg == NULL) {
		Message msgHeader;

		if(!findNextMagicNumberAndReadMessageHeader(&msgHeader, fd)){
			// there is some sort of error in the stream so we can't
			// get the next message
			//     b.lock = false;
			return;
		}

		// sets the distributedTime of TM to the maximum time received by a message
		// in a thread safe fashion
		while(true) {
			MessageTime_t time = tm->getDistributedTime();
			//check if time needs to be incremented
			if(msgHeader.time < time &&
					/*zero break*/ time - msgHeader.time < UINT_MAX/2 ) break;
			//make sure time did not change
			if(__sync_bool_compare_and_swap(
					&tm->getDistributedTime(), time, msgHeader.time)) break;
		}

		b.msg = readRestOfMessage(*(tm->getMessageAllocator()),
				fd, &msgHeader, &b.readCount);
		if(b.readCount != b.msg->bodySize) {
			b.lock = false;
			return;
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
	} else if(msg->isInternal()) {
		if(Message* reply = tm->getInternalTrampoline()->notify(msg)) {
			reply->initial_time = msg->time;
			reply->mask |= (REPLY_MASK | INTERNAL_MASK);
			tm->route(fd, reply);
			tm->getMessageAllocator()->deallocate(reply);
		}
	} else {
		tm->getSmHandler()->notify(msg);
	}

	tm->getMessageAllocator()->deallocate(msg);
}

#endif /* __TRANSPORT_CALLBACK_FUNCTIONS_H__ */
