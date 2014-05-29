#include "PendingMessages.h"

#include "sharding/transport/TransportManager.h"

using namespace srch2::httpwrapper;

Callback* RegisteredCallback::getCallbackObject() const
{
	// callback object can only be used if callback object is true
	return callbackObject;
}

void* RegisteredCallback::getOriginalSerializableObject() const {
	return originalSerializableRequestObject;
}

std::vector<Message*>& RegisteredCallback::getReplyMessages() {
	return replyMessages;
}

std::vector<Message*>& RegisteredCallback::getRequestMessages(){
	return requestMessages;
}

int RegisteredCallback::getNumberOfRepliesToWaitFor() {
	boost::shared_lock< boost::shared_mutex > lock(_access);
	return numberOfRepliesToWaitFor;
}

int RegisteredCallback::incrementNumberOfRepliesToWaitFor() {
	boost::unique_lock< boost::shared_mutex > lock(_access);
	++numberOfRepliesToWaitFor;
	return numberOfRepliesToWaitFor;
}

int RegisteredCallback::decrementNumberOfRepliesToWaitFor() {
	boost::unique_lock< boost::shared_mutex > lock(_access);
	--numberOfRepliesToWaitFor;
	return numberOfRepliesToWaitFor;
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



CallbackReference PendingRequest::getCallbackObjectReference() const {
	return callbackObjectReference;
}

MessageID_t PendingRequest::getMsgId() const {
	return msg_id;
}

time_t PendingRequest::getTimeout() const {
	return timeout;
}


void PendingMessagesHandler::addPendingMessage(time_t timeout, 
		MessageID_t id, CallbackReference cb) {
	boost::unique_lock< boost::shared_mutex > lock(_access);
	pendingRequests.push_back(PendingRequest(time(NULL) + timeout, id, cb));
}

void PendingMessagesHandler::resolveResponseMessage(Message* responseMessage) {
	if(responseMessage == NULL){
		return;
	}
	// only reply messages can enter this function
	if(!responseMessage->isReply()){
		return;
	}

	Logger::console("Resolving response message, msg type is %d", responseMessage->getType());

	PendingRequest correspondingPendingRequest;

	{
		boost::unique_lock< boost::shared_mutex > lock(_access);
		// find the pending message that its ID is the same as
		// request message Id of this response
		std::vector<PendingRequest>::iterator request =
				std::find(pendingRequests.begin(),
						pendingRequests.end(), responseMessage->getRequestMessageId());
		if(request == pendingRequests.end()){
			transportManager->getMessageAllocator()->deallocateByMessagePointer(
					responseMessage);
			return;
		}

		correspondingPendingRequest = *request;
		pendingRequests.erase(request);
	}

	/*
	 * Since we have one PendingRequest per message at this point no two threads
	 * can hold the same resolution object. BUT, RegisteredCallback can be shared between
	 * pending requests.
	 */

	RegisteredCallback* registeredCallBack =
			correspondingPendingRequest.getCallbackObjectReference().getRegisteredCallbackPtr();


	// If we are resolving a response which is for a request that needs to wait for
	// all responses.
	if(correspondingPendingRequest.getCallbackObjectReference().isWaitForAll()) {
		registeredCallBack->addReplyMessage(responseMessage);
		int numberOfResponsesLeft = registeredCallBack->decrementNumberOfRepliesToWaitFor();
		Logger::console("Request needs to wait for all responses. %d of %d expected are left.", numberOfResponsesLeft ,
				registeredCallBack->getTotalNumberOfRepliesToExpect());

		// if no response is left to wait for and callback object is open to responses,
		// trigger callback object
		// e.g. aggregation
		if(numberOfResponsesLeft == 0 &&
				registeredCallBack->getTotalNumberOfRepliesToExpect() == registeredCallBack->getNumberOfReceivedReplies()) {
			Logger::console("Callback functions and finalize are called.");
			registeredCallBack->getCallbackObject()->callbackAll(registeredCallBack->getReplyMessages());
			// destructor also calls finalize on the callback
			delete registeredCallBack;
		}
	} else { // if this request does not have to wait for all responses.
		// if it's not supposed to wait for all, callback object must be ready to handle
		// responses.
		ASSERT(registeredCallBack->getTotalNumberOfRepliesToExpect() == registeredCallBack->getNumberOfReceivedReplies());
		Logger::console("Request has single response. Callback is called.");
		registeredCallBack->getCallbackObject()->callback(responseMessage);

		int numberOfResponsesLeft = registeredCallBack->decrementNumberOfRepliesToWaitFor();
		if(numberOfResponsesLeft == 0 &&
				registeredCallBack->getTotalNumberOfRepliesToExpect() == registeredCallBack->getNumberOfReceivedReplies()) {
			Logger::console("Finalize is called.");
			// calls finalize at the end
			delete registeredCallBack;
		}
	}
}

CallbackReference 
PendingMessagesHandler::prepareCallback(void *requestObj, Callback *cb, 
		ShardingMessageType type, bool cbForAll, int shards) {

	RegisteredCallback* regcb = new RegisteredCallback(requestObj, cb,shards);

	CallbackReference rtn(type, cbForAll, shards, regcb);

	return rtn;
};

void PendingMessagesHandler::setTransportManager(TransportManager * transportManager){
	this->transportManager = transportManager;
}
