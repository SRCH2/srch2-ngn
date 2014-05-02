#ifndef __PENDING_MESSAGES_H__
#define __PENDING_MESSAGES_H__

#include "configuration/ShardingConstants.h"
#include "Message.h"
#include <vector>

namespace srch2 {
namespace httpwrapper {

struct RegisteredCallback {
  void* originalSerializableObject;
  void* callbackObject;
  int waitingOn;
  std::vector<Message*> reply;
};

typedef struct {
  __attribute__((packed)) ShardingMessageType type : 4;
  bool waitForAll : 1;
  bool isBroadcast : 1;
  bool extra1 : 1;
  bool extra2 : 1;
  RegisteredCallback* ptr;
} CallbackReference;

struct PendingRequest {
  time_t timeout;
  MessageTime_t msg_id;
  CallbackReference callbackAndTypeMask;

  bool operator==(const MessageTime_t msgId) {return this->msg_id == msgId;}
  PendingRequest(time_t timeout, MessageTime_t msg_id, CallbackReference cb) :
    timeout(timeout), msg_id(msg_id), callbackAndTypeMask(cb) {}
};


class PendingMessages {
 // static final unsigned NUMBER_OF_BYTE_IN_PENDING_BITMASK = 128;
/*
  AtomicBitmask pendingRequests(NUMBER_OF_BYTE_IN_PENDING_BITMASK/sizeof(int));
  unsigned pendingMask[NUMBER_OF_BYTE_IN_PENDING_BITMASK/sizeof(int)];
  AtomicBitmask pendingCBs(NUMBER_OF_BYTE_IN_PENDING_BITMASK/sizeof(int));
  char cbMask[NUMBER_OF_BYTE_IN_PENDING_BITMASK/sizeof(int)];

  struct PendingRequest 
    pendingRequestTable[NUMBER_OF_BYTE_IN_PENDING_BITMASK*8];
  struct RegisteredCallBack callback[NUMBER_OF_BYTE_IN_PENDING_BITMASK*8];*/

  std::vector<PendingRequest> pendingRequests;

public:
  void addMessage(time_t, MessageTime_t, CallbackReference);
  //void trigger_timeouts(time_t);
  void resolve(Message*);
  CallbackReference registerCallback(void*,void*,
      ShardingMessageType,bool = false,int = 1);
};

}}

#endif /* __PENDING_MESSAGES_H__ */
