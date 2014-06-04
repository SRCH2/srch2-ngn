/*
 * The implementation of PendingMessages.h classes are included in this header file because
 * template class functions cannot be placed in a CPP file. So this header file actually serves
 * as a CPP file.
 */
using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {

// sets the response message
template <class Request, class Response> inline
void PendingMessage<Request, Response>::setResponseMessageAndObject(Message* responseMessage, Response * responseObj){
	ASSERT(this->responseMessage == NULL);
	this->responseMessage = responseMessage;
	this->responseObject = responseObj;
}
// used to access the response message
template <class Request, class Response> inline
Message * PendingMessage<Request, Response>::getResponseMessage(){
	return this->responseMessage;
}

template <class Request, class Response> inline
Response * PendingMessage<Request, Response>::getResponseObject(){
	return this->responseObject;
}

template <class Request, class Response> inline
Message * PendingMessage<Request, Response>::getRequestMessage(){
	return this->requestMessage;
}

template <class Request, class Response> inline
Request * PendingMessage<Request, Response>::getRequestObject(){
	return this->requestObject;
}

// returns true if the nodeId that we expect the response from is equal to this nodeId
template <class Request, class Response> inline
bool PendingMessage<Request, Response>::doesExpectFromThisShard(NodeId nodeid){
	return (this->nodeId == nodeid);
}

// returns true if the value of timeout is before the current time
template <class Request, class Response> inline
bool PendingMessage<Request, Response>::isTimedOut(){
	time_t currentTime;
	time(&currentTime);
	return (this->timeout > currentTime);
}
template <class Request, class Response> inline
NodeId PendingMessage<Request, Response>::getNodeId(){
	return this->nodeId;
}

// the constructor is private because PendingMessage object should only
// be allocated through PendingRequest so that it also gets stored in that object
template <class Request, class Response> inline
PendingMessage<Request, Response>::PendingMessage(MessageAllocator * messageAllocator,NodeId nodeId, time_t timeout,
		Message * requestMessage,
		Request * requestObject){
	ASSERT(requestMessage != NULL);
	ASSERT(requestObject != NULL);
	this->messageAllocator = messageAllocator;
	this->nodeId = nodeId;
	this->timeout = timeout;
	this->requestMessage = requestMessage;
	this->requestObject = requestObject;
	this->responseMessage = NULL;
	this->responseObject = NULL;
}

template <class Request, class Response> inline
PendingMessage<Request, Response>::~PendingMessage(){

	// request object and message are protected by shared pointer.

	if(responseMessage != NULL){
		messageAllocator->deallocateByMessagePointer(responseMessage);
		ASSERT(responseObject != NULL);
		delete responseObject;
	} // if they are null it means this request had timed out
}


/////// PendingRequest method implementations


// this function allocates and returns a PendingMessage. PendingMessage objects
// are always allocated and deleted in this class.
// this function also adds this PendingMessage to the map.
template <class Request, class Response> inline
PendingMessage<Request, Response> * PendingRequest<Request, Response>::registerPendingMessage(NodeId nodeId,
		time_t timeout,
		Message * requestMessage,
		Request * requestObject){
	// create the pending message object
	PendingMessage<Request, Response> * pendingMessage =
			new PendingMessage<Request, Response>(messageAllocator ,nodeId, timeout, requestMessage, requestObject);

	// get a write lock on pendingMessages and add this new pendingMessage
	boost::unique_lock< boost::shared_mutex > lock(_access);

	// add it to pendingMessages
	if(requestMessage->isLocal()){
		this->localRequestMessageId = requestMessage->getMessageId();
		ASSERT(pendingMessages.size() >= 1 && pendingMessages.at(0) == NULL);
		pendingMessages.at(0) = pendingMessage;
	}else{
		this->externalRequestMessageId = requestMessage->getMessageId();
		ASSERT(pendingMessages.size() >= 1);
		pendingMessages.push_back(pendingMessage);
	}

	return pendingMessage;
}

// resolves the corresponding PendingMessage with this response
template <class Request, class Response> inline
bool PendingRequest<Request, Response>::resolveResponseMessage(Message * responseMessage, NodeId nodeIdOfResponse, void * responseObjectTypeVoid){

	Response * responseObjectArg = (Response *)responseObjectTypeVoid;

	if(responseMessage == NULL){
		ASSERT(false);
		return false;
	}

	// 1. get a write lock on pendingMessages, remove the corresponding message
	boost::unique_lock< boost::shared_mutex > lock(_access);
	ASSERT(pendingMessages.size() >= 1); // local pointer is always NULL, even though if it's NULL
	// this must be the pending request of this message
	ASSERT(responseMessage->getRequestMessageId() == this->localRequestMessageId ||
			responseMessage->getRequestMessageId() == this->externalRequestMessageId);

	// 2. find which PendingMessage is responsible
	PendingMessage<Request , Response> * pendingMessage = NULL;
	unsigned pendingMessageLocation = 0;
	// is it the local response or the external one?
	if(responseMessage->isLocal()){
		// response is for the local pending message
		ASSERT(pendingMessages.at(0)->doesExpectFromThisShard(nodeIdOfResponse));
		pendingMessage = pendingMessages.at(0);
		pendingMessageLocation = 0;
	}else{
		// if it's not local, it must be external, this fucntion only is called if this pending message is for
		// this response message
		// iterate on external pending messages and look for nodeId
		for(unsigned pendingMessageIndex = 1 ; pendingMessageIndex < pendingMessages.size() ; pendingMessageIndex ++){
			if(pendingMessages.at(pendingMessageIndex) != NULL){
				if(pendingMessages.at(pendingMessageIndex)->doesExpectFromThisShard(nodeIdOfResponse)){
					// this pending request is responsible of this response
					pendingMessage = pendingMessages.at(pendingMessageIndex);
					pendingMessageLocation = pendingMessageIndex;
					break;
				}
			}// when it is NULL it means the response of this PendingMessage has come earlier.
		}
	}
	if(pendingMessage == NULL){ // no pending message is found for this response so it is timedout
		// because of the locking scheme this cannot happen now
		// if we change locking scheme then we should handle this case
		ASSERT(false);
		return false;
	}

	//3. Remove pendingMessage from pendingMessages vector
	pendingMessages.at(pendingMessageLocation) = NULL;

	//4. deserialize response message to response object
	// deserialize the message into the response type
	// example : msg deserializes into SerializableSearchResults
	Response * responseObject = NULL;
	if(responseObjectArg == NULL){
		if(responseMessage->isLocal()){ // for local response we should just get the pointer
			responseObject = decodeInternalMessage<Response>(responseMessage);
		}else{ // for non-local response we should deserialize the message
			responseObject = decodeExternalMessage<Response>(responseMessage);
		}
	}else{
		responseObject = responseObjectArg;
	}

	// 5. set the response message and response object in the PendingMessage
	pendingMessage->setResponseMessageAndObject(responseMessage , responseObject);

	// 6. put the satisfied pending message in pendingMessagesWithResponse
	if(responseMessage->isLocal()){
		pendingMessagesWithResponse.at(0) = pendingMessage;
	}else{
		pendingMessagesWithResponse.push_back(pendingMessage);
	}

	// if it's not a waitForAll case we call callback right here.
	if(! shouldWaitForAll()){
		// now we should pass the object to aggregator
		aggregator->callBack(pendingMessage);
	}

	// now check if all pendingMessages are satisfied we are ready for finalizing.
	return shouldFinalize();
}


template <class Request, class Response> inline
bool PendingRequest<Request, Response>::resolveTimedoutMessages(){
	// move on all pending messages and check if the are timed out or not
	// 1. get an X lock on the pendingMessages vector
	boost::unique_lock< boost::shared_mutex > lock(_access);
	ASSERT(pendingMessages.size() >= 1); // local pointer is always NULL, even though if it's NULL
	// 2. move on pendingMessages and check them for timeout
	for(unsigned pendingMessageIndex = 0; pendingMessageIndex < pendingMessages.size(); ++pendingMessageIndex){
		PendingMessage<Request, Response> * pendingMessage = pendingMessages.at(pendingMessageIndex);
		if(pendingMessage == NULL){
			// if pendingMessageItr is NULL, it's either the local one which has not come yet
			// or it's arleady accepted the response
			continue;
		}
		// the message is timeout, we should move it from pendingMessages to
		// pendingMessagesWithResponse and leave response pointers empty
		if(pendingMessage->isTimedOut()){
			// set the response message and response object to NULL to indicate timeout
			pendingMessage->setResponseMessageAndObject(NULL , NULL);

			// move it to pendingMessagesWithResponse
			if(pendingMessage->getRequestMessage()->isLocal()){
				pendingMessagesWithResponse.at(0) = pendingMessage;
			}else{
				pendingMessagesWithResponse.push_back(pendingMessage);
			}

			// set it null in pendingMessages
			pendingMessages.at(pendingMessageIndex) = NULL;

			// call timeout of aggregator
			aggregator->timeoutProcessing(pendingMessage, ResultsAggregatorAndPrintMetadata());
		}
	}

	//  now check if all pendingMessages are satisfied we are ready for finalizing.
	return shouldFinalize();
}

// returns true of it contains a PendingMessage corresponding to this responseMessage
template <class Request, class Response> inline
bool PendingRequest<Request, Response>::isResponseMessageMine(Message * responseMessage){

	// get a read lock on pendingMessages,
	boost::shared_lock< boost::shared_mutex > lock(_access);
	return (this->localRequestMessageId == responseMessage->getRequestMessageId() ||
			this->externalRequestMessageId == responseMessage->getRequestMessageId());
}


// returns true if all pendingMessages have either timed out or received a response
//
template <class Request, class Response> inline
bool PendingRequest<Request, Response>::shouldFinalize(){
	// get a read lock on pendingMessages,
	// this function should only be called within a locked scope
	unsigned numberOfPendingMessagesWithResponse = pendingMessagesWithResponse.size() ;
	// because the first one the place holder for local pending message and it's always there
	if(pendingMessagesWithResponse.at(0) == NULL){
		numberOfPendingMessagesWithResponse--;
	}
	return (numberOfPendingMessagesWithResponse == totalNumberOfPendingMessages);
}


//////////////////////////////////
template <class Request, class Response> inline
PendingRequest<Request, Response> * PendingRequestsHandler::registerPendingRequest(bool waitForAll,
		boost::shared_ptr<ResultAggregatorAndPrint<Request, Response> > aggregator,
		unsigned totalNumberOfPendingMessages){

	// create the pendingRequest
	PendingRequest<Request, Response> * pendingRequest = new PendingRequest<Request, Response>(messageAllocator, waitForAll, aggregator, totalNumberOfPendingMessages);

	// register
	// get a write lock on pendingRequests and push this new request to it.
	boost::unique_lock< boost::shared_mutex > lock(_access);
	this->pendingRequests.push_back(pendingRequest);

	// return
	return pendingRequest;
}

/*
 * If this response is not related to any pending requests, returns false
 */
inline bool PendingRequestsHandler::resolveResponseMessage(Message * response, NodeId nodeId, void * responseObject){
	// 1. get an X lock on the pendingRequest list
	boost::unique_lock< boost::shared_mutex > lock(_access);
	// 2. iterate on PendingRequests and check if any of them is corresponding to this response
	for(vector<PendingRequestAbstract *>::iterator pendingRequestItr = pendingRequests.begin();
			pendingRequestItr != pendingRequests.end() ; ++pendingRequestItr){
		if((*pendingRequestItr)->isResponseMessageMine(response)){
			// ask this pending request to resolve this response
			bool resolveResult = (*pendingRequestItr)->resolveResponseMessage(response, nodeId,responseObject);
			// if returns true, we must delete this PendingRequest from the vector.
			if(resolveResult){
				pendingRequests.erase(pendingRequestItr);
				delete *pendingRequestItr;
			}
			return true;
		}
	}

	return false;
}

inline void PendingRequestsHandler::resolveTimedoutMessages(){
	// 1. get a X lock on the list if pending requests
	boost::unique_lock< boost::shared_mutex > lock(_access);
	// 2. move on all pending requests and check for timeout,
	vector<PendingRequestAbstract *> newPendingRequests;
	for(vector<PendingRequestAbstract *>::iterator pendingRequestItr = pendingRequests.begin();
			pendingRequestItr != pendingRequests.end() ; ++pendingRequestItr){
		bool resolveResult = (*pendingRequestItr)->resolveTimedoutMessages();
		// if returns true, we must delete this PendingRequest from the vector.
		// so otherwise we push it to the temporary newPendingRequests vector
		if(resolveResult){
			delete *pendingRequestItr;
			*pendingRequestItr = NULL;
		}else{
			newPendingRequests.push_back(*pendingRequestItr);
		}
	}
	pendingRequests.clear();
	// copy back new pending messages into old vector
	pendingRequests.insert(pendingRequests.begin(), newPendingRequests.begin(), newPendingRequests.end());
}

inline PendingRequestsHandler::PendingRequestsHandler(MessageAllocator * messageAllocator){

	this->messageAllocator = messageAllocator;

	if (pthread_create(&(this->timeoutThread), NULL, PendingRequestsHandler::timeoutHandler, this) != 0){
		perror("Cannot create thread for timeout for pending messages.");
		return;
	}
}

inline PendingRequestsHandler::~PendingRequestsHandler(){
	pthread_cancel(this->timeoutThread);
}

}
}
