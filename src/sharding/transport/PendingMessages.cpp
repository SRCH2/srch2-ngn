#include "PendingMessages.h"

using namespace srch2::httpwrapper;

Callback* RegisteredCallback::getCallbackObject() const
{
	return callbackObject;
}

void* RegisteredCallback::getOriginalSerializableObject() const {
	return originalSerializableObject;
}

std::vector<Message*>& RegisteredCallback::getReply() {
	return reply;
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

RegisteredCallback* CallbackReference::getPtr() const {
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

void PendingMessages::resolve(Message* msg) {
	if(!msg->isReply()) return;

	std::vector<PendingRequest>::iterator request =
			std::find(pendingRequests.begin(),
					pendingRequests.end(), msg->initial_time);

	RegisteredCallback *cb = request->getCallbackAndTypeMask().getPtr();

	if(request->getCallbackAndTypeMask().isWaitForAll()) {
		cb->getReply().push_back(msg);
		int num = __sync_sub_and_fetch(&cb->getWaitingOn(), 1);

		pendingRequests.erase(request);
		if(num == 0) {
			cb->getCallbackObject()->callbackAll(cb->getReply());
			delete cb;
			for(std::vector<Message*>::iterator msg = cb->getReply().begin();
					msg != cb->getReply().end(); ++msg) {
				delete *msg;
			}
		}
	} else {
		cb->getCallbackObject()->callback(msg);
		if(__sync_sub_and_fetch(&cb->getWaitingOn(), 1) == 0) delete cb;
		delete msg;
	}
}

CallbackReference 
PendingMessages::prepareCallback(void *obj, Callback *cb, 
		ShardingMessageType type, bool cbForAll, int shards) {

	RegisteredCallback* regcb = new RegisteredCallback(obj, cb,shards);

	CallbackReference rtn(type, cbForAll, shards, regcb);

	return rtn;
};

