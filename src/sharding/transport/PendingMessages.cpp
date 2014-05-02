#include "PendingMessages.h"

using namespace srch2::httpwrapper;

void PendingMessages::addMessage(time_t timeout, 
    MessageTime_t id, CallbackReference cb) {
  pendingRequests.push_back(PendingRequest(time(NULL) + timeout, id, cb));
}

void PendingMessages::resolve(Message* msg) {
  if(!msg->isReply()) return;

  std::vector<PendingRequest>::iterator request = 
    std::find(pendingRequests.begin(),pendingRequests.end(), msg->inital_time);

  RegisteredCallback *cb = request->callbackAndTypeMask.ptr;

  if(request->callbackAndTypeMask.waitForAll) {
    cb->reply.push_back(msg);
    int num = __sync_sub_and_fetch(&(cb->waitingOn), 1);

    pendingRequests.erase(request);
    if(num == 0) {
      //call -> callback on vector
      delete cb;
      for(std::vector<Message*>::iterator msg = cb->reply.begin(); 
          msg != cb->reply.end(); ++msg) {
        delete *msg;
      }
    } 
  } else {
    //call callback
    delete cb;
    delete msg;
  }
}

CallbackReference 
PendingMessages::registerCallback(void *obj, void *cb, 
    ShardingMessageType type, bool cbForAll, int shards) {
  RegisteredCallback* regcb = new RegisteredCallback;

  regcb->originalSerializableObject = obj;
  regcb->callbackObject = cb;
  regcb->waitingOn = shards;
  
  CallbackReference rtn;
  rtn.ptr = regcb;
  rtn.type = type;
  rtn.isBroadcast = shards;
  rtn.waitForAll != cbForAll;

  return rtn;
};

