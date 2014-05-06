#ifndef __PENDING_MESSAGES_H__
#define __PENDING_MESSAGES_H__

#include "configuration/ShardingConstants.h"
#include "Message.h"
#include <vector>

namespace srch2 {
namespace httpwrapper {

/*
 * This struct is for abstracting timeout and callback behaviour
 * currently used for TM
 */
class Callback {
public:
	virtual void timeout(void*) = 0;
	virtual void callback(Message*) {};
	virtual void callbackAll(vector<Message*>&) {};
	virtual ~Callback(){};
};


/*
 *
 */
class RegisteredCallback {

public:
	RegisteredCallback(void* originalSerializableObject, Callback* callbackObject, int waitingOn){
		this->originalSerializableObject = originalSerializableObject;
		this->callbackObject = callbackObject;
		this->waitingOn = waitingOn;
	}

	Callback* getCallbackObject() const;
	void* getOriginalSerializableObject() const ;
	std::vector<Message*>& getReply() const ;
	int& getWaitingOn() const ;

private:

	// request object
	void* originalSerializableObject;
	// callback object, example: RMCallback or SMCallback
	Callback* callbackObject;
	// number of shards that are still pending
	int waitingOn;
	// those replies that we received already
	// reply.size() + waitingOn should always be equal to the size of broadcast
	std::vector<Message*> reply;
};


class CallbackReference{
public:

	CallbackReference():type(type),waitForAll(waitForAll),broadcastFlag(broadcastFlag),ptr(ptr){}//TODO how should we initialize these variables?
	CallbackReference(ShardingMessageType type, bool waitForAll , bool broadcastFlag, RegisteredCallback* const ptr):
	type(type),waitForAll(waitForAll),broadcastFlag(broadcastFlag),ptr(ptr){
	}

	bool isExtra1() const;
	bool isExtra2() const;
	bool isBroadcast() const;
	RegisteredCallback* getPtr() const;
	ShardingMessageType getType() const;
	bool isWaitForAll() const;

private:
	const __attribute__((packed)) ShardingMessageType type : 4;
	// indicates whether callback should for all before calling the callback
	const bool waitForAll : 1;
	// broadcast vs route
	const bool broadcastFlag : 1;
	// two more bits to make it aligned
	bool extra1 : 1;
	bool extra2 : 1;
	RegisteredCallback* const ptr;
}
;

/*
 * PendingRequest object is made for each message that we are waiting for its response
 * Since multiple pending messages can use a single CallbackReference, we cannot merge
 * PendingRequest and CallbackReference together.
 */
class PendingRequest {
public:

	bool operator==(const MessageTime_t msgId) {return this->msg_id == msgId;}
	PendingRequest(time_t timeout, MessageTime_t msg_id, CallbackReference cb) :
		timeout(timeout), msg_id(msg_id), callbackAndTypeMask(cb) {};

	CallbackReference getCallbackAndTypeMask() const ;
	MessageTime_t getMsgId() const ;
	time_t getTimeout() const ;
private:
	/*
	 * The physical clock time in seconds at which this request times out
	 */
	time_t timeout;

	/*
	 * Unique identifier of this message
	 */
	MessageTime_t msg_id;

	/*
	 * Callback reference for this message
	 */
	CallbackReference callbackAndTypeMask;
};


/*
 * This class stores all pending messages and one object of it is
 * kept in TM
 */
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

private:
	std::vector<PendingRequest> pendingRequests;

public:
	void addMessage(time_t, MessageTime_t, CallbackReference);
	//void trigger_timeouts(time_t);
	void resolve(Message*);
	CallbackReference registerCallback(void*, Callback*,
			ShardingMessageType,bool = false,int = 1);
};

}}

#endif /* __PENDING_MESSAGES_H__ */
