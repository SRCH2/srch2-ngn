#ifndef __TRANSPORT_CALLBACK_FUNCTIONS_H__
#define __TRANSPORT_CALLBACK_FUNCTIONS_H__

#include<unistd.h>
#include<errno.h>
#include "TransportManager.h"

using namespace srch2::httpwrapper;

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
			//v1: broken message boundary
			continue;
		}

		//TODO:checkMagicNumbers

		return true;
	}

	//ASSERT(false);
	return false;
}

Message* readRestOfMessage(MessageAllocator& messageAllocator,
		int fd, Message *const msgHeader) {
	Message *msg= messageAllocator.allocateMessage(msgHeader->bodySize);
	int readReturnValue = read(fd, msg->buffer, msgHeader->bodySize);

	if(readReturnValue != msgHeader->bodySize){  //handle error
	}

	memcpy(msg, msgHeader, sizeof(Message));

	return msg;
}


/*
 * This callback function is called from transport layer upon receiving an internal DP/SM messages
 */
void cb_recieveMessage(int fd, short eventType, void *arg) {

	TransportManager* tm = (TransportManager*) arg;
	Message msgHeader;

	if(!findNextMagicNumberAndReadMessageHeader(&msgHeader, fd)){
		// there is some sort of error in the stream so we can't get the next message
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

	Message *msg =
			readRestOfMessage(*(tm->getMessageAllocator()), fd, &msgHeader);

	if(msg->isReply()) {
		tm->getMsgs()->resolve(msg);
	} else if(msg->isInternal()) {
		if(Message* reply = tm->getInternalTrampoline()->notify(msg)) {
      reply->initial_time = msg->time;
      reply->mask |= REPLY_MASK & INTERNAL_MASK;
      tm->route(fd, reply);
      tm->getMessageAllocator()->deallocate(reply);
    }
	} else {
		tm->getSmHandler()->notify(msg);
	}

	tm->getMessageAllocator()->deallocate(msg);
}

#endif /* __TRANSPORT_CALLBACK_FUNCTIONS_H__ */
