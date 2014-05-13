#include "PendingMessages.h"

#include "sharding/transport/TransportManager.h"

using namespace srch2::httpwrapper;

Callback* RegisteredCallback::getCallbackObject() const
{
	return callbackObject;
}

void* RegisteredCallback::getOriginalSerializableObject() const {
	return originalSerializableObject;
}

std::vector<Message*>& RegisteredCallback::getReplyMessages() {
	return replyMessages;
}

std::vector<Message*>& RegisteredCallback::getRequestMessages(){
	return requestMessages;
}

int& RegisteredCallback::getWaitingOn() {
	return waitingOn;
}



bool CallbackReference::isExtra1() const
{
	return extra1;
}

bool CallbackReference::isExtra2() const {
	return extra2;
}

bool CallbackReference::isBroadcast() const {
	return broadcastFlag;
}

RegisteredCallback* CallbackReference::getRegisteredCallbackPtr() const {
	return ptr;
}

ShardingMessageType CallbackReference::getType() const {
	return type;
}

bool CallbackReference::isWaitForAll() const {
	return waitForAll;
}



CallbackReference PendingRequest::getCallbackAndTypeMask() const {
	return callbackAndTypeMask;
}

MessageTime_t PendingRequest::getMsgId() const {
	return msg_id;
}

time_t PendingRequest::getTimeout() const {
	return timeout;
}


void PendingMessages::addMessage(time_t timeout, 
		MessageTime_t id, CallbackReference cb) {
	pendingRequests.push_back(PendingRequest(time(NULL) + timeout, id, cb));
}

void PendingMessages::resolve(Message* message) {
	// only reply messages can enter this function
	if(!message->isReply()){
		return;
	}

	std::vector<PendingRequest>::iterator request =
			std::find(pendingRequests.begin(),
					pendingRequests.end(), message->getInitialTime());
	if(request == pendingRequests.end()){
		//TODO
		// what should we if there is no request for this response ? Maybe response is late?
		transportManager->getMessageAllocator()->deallocateByMessagePointer(message);
		return;
	}

	RegisteredCallback *cb = request->getCallbackAndTypeMask().getRegisteredCallbackPtr();

	if(request->getCallbackAndTypeMask().isWaitForAll()) {
		cb->getReplyMessages().push_back(message);
		int num = __sync_sub_and_fetch(&cb->getWaitingOn(), 1);

		pendingRequests.erase(request);
		if(num == 0) {
			cb->getCallbackObject()->callbackAll(cb->getReplyMessages());
			for(std::vector<Message*>::iterator msgItr = cb->getReplyMessages().begin();
					msgItr != cb->getReplyMessages().end(); ++msgItr) {
				transportManager->getMessageAllocator()->deallocateByMessagePointer(*msgItr);
			}
			delete cb;
		}
	} else {
		cb->getCallbackObject()->callback(message);
		if(__sync_sub_and_fetch(&cb->getWaitingOn(), 1) == 0){
			delete cb;
		}
		transportManager->getMessageAllocator()->deallocateByMessagePointer(message);
	}
}

CallbackReference 
PendingMessages::prepareCallback(void *obj, Callback *cb, 
		ShardingMessageType type, bool cbForAll, int shards) {

	RegisteredCallback* regcb = new RegisteredCallback(obj, cb,shards);

	CallbackReference rtn(type, cbForAll, shards, regcb);

	return rtn;
};

void PendingMessages::setTransportManager(TransportManager * transportManager){
	this->transportManager = transportManager;
}
