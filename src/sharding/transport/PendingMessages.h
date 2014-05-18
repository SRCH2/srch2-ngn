#ifndef __PENDING_MESSAGES_H__
#define __PENDING_MESSAGES_H__

#include "configuration/ShardingConstants.h"
#include "Message.h"
#include <vector>
#include "core/util/Assert.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>



namespace srch2 {
namespace httpwrapper {

class TransportManager;
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
	std::vector<Message*>& getReplyMessages();
	std::vector<Message*>& getRequestMessages();
	int& getWaitingOn();
	~RegisteredCallback(){
		delete callbackObject;
		// delete request messages
		// reply messages are deleted outside in PendingMessages::resolve(Message)
		for(std::vector<Message*>::iterator msgItr = requestMessages.begin(); msgItr != requestMessages.end(); ++msgItr){
			ASSERT(*msgItr != NULL);
			if(*msgItr != NULL){
				delete *msgItr;
			}
		}
	}

private:

	// request object
	void* originalSerializableObject;
	// callback object, example: RMCallback or SMCallback
	Callback* callbackObject;
	// number of shards that are still pending
	int waitingOn;
	// those replies that we received already
	// reply.size() + waitingOn should always be equal to the size of broadcast
	std::vector<Message*> requestMessages;
	std::vector<Message*> replyMessages;
};


class CallbackReference {
public:
	CallbackReference() : type(NULLType), waitForAll(0), broadcastFlag(0),
	extra1(0),extra2(0), ptr(0) {}
	CallbackReference(ShardingMessageType type, 
			bool waitForAll , bool broadcastFlag, RegisteredCallback* ptr) :
				type(type),waitForAll(waitForAll),broadcastFlag(broadcastFlag),ptr(ptr) {}
	CallbackReference(const CallbackReference& cpy) :
		type(cpy.type),waitForAll(cpy.waitForAll),broadcastFlag(cpy.broadcastFlag),
		ptr(cpy.ptr) {}
	CallbackReference& operator=(const CallbackReference& cpy) {
		new (this) CallbackReference(cpy);
		return *this;
	}

	bool isExtra1() const;
	bool isExtra2() const;
	bool isBroadcast() const;
	RegisteredCallback* getRegisteredCallbackPtr() const;
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
};

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
   PendingRequest() : timeout(0), msg_id(0), 
   callbackAndTypeMask(CallbackReference()) {}

	CallbackReference getCallbackAndTypeMask() const ;
	MessageTime_t getMsgId() const ;
	time_t getTimeout() const ;
   bool triggered();

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


typedef std::vector<PendingRequest>& PendingRequests;
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
	TransportManager * transportManager;
	mutable boost::shared_mutex _access;

public:
	void setTransportManager(TransportManager * transportManager);
	void addMessage(time_t, MessageTime_t, CallbackReference);
	//void trigger_timeouts(time_t);
	void resolve(Message*);
	CallbackReference prepareCallback(void*, Callback*,
			ShardingMessageType,bool = false,int = 1);
};

inline bool PendingRequest::triggered() {
   if(time_t to = timeout) {
      //find out if cb has been triggered before by this request
      if(to == -1 || 
            !__sync_bool_compare_and_swap(&to, to, (time_t) -1)) return true;
   }
   return false;
}

}}

#endif /* __PENDING_MESSAGES_H__ */
