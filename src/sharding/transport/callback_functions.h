#ifndef __TRANSPORT_CALLBACK_FUNCTIONS_H__
#define __TRANSPORT_CALLBACK_FUNCTIONS_H__

#include<unistd.h>
#include<errno.h>
#include "TransportManager.h"

using namespace srch2::httpwrapper;

///*
// * This function reads the stream until it finds the next valid message
// * Each message starts with a special value called magic number
// */
//bool findNextMagicNumberAndReadMessageHeader(Message *const msg,  int fd) {
//	unsigned offset = 0;
//	unsigned messageSize = sizeof(Message);
//	while(true) {
//		int readRtn = read(fd, msg + offset, messageSize);
//
//		if(readRtn == -1) {
////			if(errno == EAGAIN || errno == EWOULDBLOCK) {
////				perror("xxx eagain/ewouldblock error in message header parsing xxx");
////				return false;
////			}
//			perror("xxx READ : HEADER : xxx");
////			//v1: handle  error
//			return false;
//		}
//
//		if(readRtn < messageSize) {
//			//v1: broken message boundary
//			//Logger::console("*** incomplete read ***");
//			offset += readRtn;
//			messageSize -= readRtn;
//			continue;
//		}
//
//		//TODO:checkMagicNumbers
//
//		return true;
//	}
//
//	//ASSERT(false);
//	return false;
//}
//
//Message* readRestOfMessage(MessageAllocator& messageAllocator,
//		int fd, Message *const msgHeader) {
//	Message *msg= messageAllocator.allocateMessage(msgHeader->bodySize);
//	memcpy(msg, msgHeader, sizeof(Message));
//
//	unsigned offset = 0;
//	unsigned messageSize = msgHeader->bodySize;
//	while(messageSize) {
//		int readReturnValue = read(fd, msg->buffer + offset, messageSize);
//		if (readReturnValue < 0) {
////			if(errno == EAGAIN || errno == EWOULDBLOCK) {
////				sleep(1);
////				cout << "got EAGAIN !!" << endl;
////				continue;
////			}
////			msg->bodySize = 0;
//			perror("xxx READ : BODY xxx ");
//			return NULL;
//		}
//		if(readReturnValue < messageSize){
//			offset += readReturnValue;
//			messageSize -= readReturnValue;
//		}
//		if(readReturnValue == messageSize){
//			break;
//		}
//	}
//
//	return msg;
//}


void cb_recieveMessage(struct bufferevent *bev, void *arg){

	TransportManager* tm = (TransportManager*) arg;
	Message msgHeader;
	int n;
	struct evbuffer *inp = bufferevent_get_input(bev);
	signed totalLen = evbuffer_get_length(inp);
	signed read = evbuffer_remove(inp, &msgHeader, sizeof(Message));
	if (read != sizeof(Message)) {
		cout << "XXX buffer read error XXX" << endl;
		return;
	}
	//cout << "*** buffer read success ***" << msgHeader.bodySize << endl;

	if (totalLen == msgHeader.bodySize + sizeof(Message)) {
		//cout << "*** total len is good" << endl;
	} else {
		cout << "*** total len is bad" << endl;
	}

	Message *msg =  tm->getMessageAllocator()->allocateMessage( msgHeader.bodySize);
	memcpy(msg, &msgHeader, sizeof(Message));
	evbuffer_remove(inp, msg->buffer, msgHeader.bodySize);
	if (tm->getSmHandler())
		tm->getSmHandler()->notify(msg);

	tm->getMessageAllocator()->deallocate(msg);
}

///*
// * This callback function is called from transport layer upon receiving an internal DP/SM messages
// */
//void cb_recieveMessage(int fd, short eventType, void *arg) {
//
//
//	TransportManager* tm = (TransportManager*) arg;
//	tm->acquireLock();
//	Message msgHeader;
//
//	if(!findNextMagicNumberAndReadMessageHeader(&msgHeader, fd)){
//		// there is some sort of error in the stream so we can't get the next message
//		tm->releaseLock();
//		return;
//	}
//
//	// sets the distributedTime of TM to the maximum time received by a message
//	// in a thread safe fashion
//	while(true) {
//		MessageTime_t time = tm->getDistributedTime();
//		//check if time needs to be incremented
//		if(msgHeader.time < time &&
//				/*zero break*/ time - msgHeader.time < UINT_MAX/2 ) break;
//		//make sure time did not change
//		if(__sync_bool_compare_and_swap(
//				&tm->getDistributedTime(), time, msgHeader.time)) break;
//	}
//
//	Message *msg =
//			readRestOfMessage(*(tm->getMessageAllocator()), fd, &msgHeader);
//	tm->releaseLock();
//	if (msg == NULL){
//		return;
//	}
//	if(msg->isReply()) {
//		tm->getMsgs()->resolve(msg);
//	} else if(msg->isInternal()) {
//		if(Message* reply = tm->getInternalTrampoline()->notify(msg)) {
//			reply->initial_time = msg->time;
//			reply->mask |= REPLY_MASK & INTERNAL_MASK;
//			tm->route(fd, reply);
//			tm->getMessageAllocator()->deallocate(reply);
//		}
//	} else {
//		/*
//		 *  It is possible that SM handler is not registered yet. If it is not registered then
//		 *  skip the messages.
//		 */
//		if (tm->getSmHandler())
//			tm->getSmHandler()->notify(msg);
//	}
//
//	tm->getMessageAllocator()->deallocate(msg);
//}

#endif /* __TRANSPORT_CALLBACK_FUNCTIONS_H__ */
