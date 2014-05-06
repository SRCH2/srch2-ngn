#ifndef __TRANSPORT_CALLBACK_FUNCTIONS_H__
#define __TRANSPORT_CALLBACK_FUNCTIONS_H__

#include<unistd.h>
#include<errno.h>
#include "TransportManager.h"

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
}

Message* readRestOfMessage(std::allocator<char> messageAllocator,
    int fd, Message *const msgHeader) {
  char *buffer= messageAllocator.allocate(msgHeader->bodySize);

  int readReturnValue = read(fd, buffer, msgHeader->bodySize);

  if(readReturnValue != msgHeader->bodySize); //handle error

  Message* msg = (Message*) (buffer - sizeof(msgHeader));
  memcpy(msg, msgHeader, sizeof(Message));

  return msg;
}
  


void cb_recieveMessage(int fd, short eventType, void *arg) {
  TransportManager* tm = (TransportManager*) arg;
  Message msgHeader;
  if(!findNextMagicNumberAndReadMessageHeader(&msgHeader, fd)) return;

  while(true) {
    MessageTime_t time = tm->distributedTime;
    //check if time needs to be incremented
    if(msgHeader.time < time && 
        /*zero break*/ time - msgHeader.time < UINT_MAX/2 ) break;
    //make sure time did not change
    if(__sync_bool_compare_and_swap(
          &tm->distributedTime, time, msgHeader.time)) break;
  }

  Message *msg = 
    readRestOfMessage(tm->messageAllocator, fd, &msgHeader);

  if(msg.isReply()) {
    tm->msgs.resolve(msg);
  } else if(msg.isInternalMessage()) {
    tm->internalTrampoline->notify(msg);
  } else {
    tm->smHandler->notify(msg);
  }

  tm->messageAllocator.deallocate(msg);
}

#endif /* __TRANSPORT_CALLBACK_FUNCTIONS_H__ */
