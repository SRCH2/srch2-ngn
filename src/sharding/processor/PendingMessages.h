#ifndef __SHARDING_ROUTING_PENDING_MESSAGES_H__
#define __SHARDING_ROUTING_PENDING_MESSAGES_H__

#include "aggregators/ResponseAggregator.h"
#include "sharding/transport/MessageAllocator.h"

namespace srch2 {
namespace httpwrapper {

class TransportManager;

template <class Request, class Response>
class PendingRequest;

template <class Request, class Response>
class PendingMessage{
    // PendingRequest is a friend class because PendingMessage objects are only allocated through that
    friend class PendingRequest<Request, Response>;
public:
    // sets the response message
    void setResponseObject(Response * responseObj){
        ASSERT(this->responseObject == NULL);
        this->responseObject = responseObj;
    }
    Response * getResponseObject() const{
        return this->responseObject;
    }
    // returns true if the nodeId that we expect the response from is equal to this nodeId
    bool doesExpectThisReply(NodeId nodeid, unsigned requestMessageId) const{
        return (this->nodeId == nodeid) && (this->requestMessageId == requestMessageId);
    }

    // returns true if the value of timeout is before the current time
    bool isTimedOut() const{
        if(this->timeout == 0){ // zero indicates no timeout
            return false;
        }
        time_t currentTime;
        time(&currentTime);
        Logger::console("Time to check timeout : %d" , currentTime);
        return (this->timeout < currentTime);
    }

    unsigned getRequestMessageId() const{
    	return requestMessageId;
    }

    const Request * getRequestObject(){
    	return requestObject;
    }

    // returns the destination node id of this pendingMessage
    NodeId getNodeId() const{
        return this->nodeId;
    }

private:
    // the constructor is private because PendingMessage object should only
    // be allocated through PendingRequest so that it also gets stored in that object
    PendingMessage(const Request * requestObj, MessageAllocator * messageAllocator, NodeId nodeId,
    		time_t timeout, unsigned requestMessageId):requestObject(requestObj){
        ASSERT(requestObj != NULL);
        this->messageAllocator = messageAllocator;
        this->nodeId = nodeId;
        this->timeout = timeout;
        this->requestMessageId = requestMessageId;
        this->responseObject = NULL;
    }

    ~PendingMessage(){

        if(responseObject != NULL){
            delete responseObject;
        } // if they are null it means this request had timed out
    }


    MessageAllocator * messageAllocator;
    const Request * requestObject;
    // the absolute time on which this message times out
    time_t timeout;
    // the messageId of request
    unsigned requestMessageId;
    // Node id of the shard which we are waiting for response from
    NodeId nodeId; // TODO : node ID should change to shard Id in V1 because we can have multiple shards in a node
    // the reply message, this member is NULL in the beginning and gets filled only
    // if a response comes for this request message
    // The reply object which is produced after deserialization.
    Response * responseObject; // NULL in the beginning.
};



class ReplyHandlerInterface{
public:
	virtual bool isMessageMine(NodeId srcNodeId, unsigned requestMessageId) const = 0;
	virtual bool resolveReply(void * replyObject, NodeId srcNodeId , unsigned requestMessageId) = 0;
	virtual bool resolveTimedoutRequests() = 0;
	virtual ~ReplyHandlerInterface(){};
};

/*
 * Pending request is the object which keeps the information of a request pending for responses.
 * Some requests like search produce multiple messages sent to other shards. For this reason a single object
 * PendingRequest is made and multiple instances of PendingMessage point to this single object. Therefore,
 * PendingRequest object is accessed by multiple threads and must be a thread safe object.
 */
template <class Request, class Response>
class PendingRequest : public ReplyHandlerInterface{
    friend class ReplyMessageHandler;
public:


    // returns whether this PendingRequest waits for all replies to call callBackAll or
    // calls callBack functions separately.
    bool shouldWaitForAll() const{
    	return waitForAll;
    }

    // this function allocates and returns a PendingMessage. PendingMessage objects
    // are always allocated and deleted in this class.
    // this function also adds this PendingMessage to the map.
    PendingMessage<Request, Response> * registerPendingMessage(const Request * requestObj, NodeId nodeId, time_t timeout,
            unsigned requestMessageId, bool isLocal){
        // create the pending message object
        PendingMessage<Request, Response> * pendingMessage =
                new PendingMessage<Request, Response>(requestObj, messageAllocator ,nodeId, timeout, requestMessageId);


        // add it to pendingMessages
        if(isLocal){
            localPendingMessages.push_back(pendingMessage);
            // We assume the local message is always stored at the 0th slot.
        }else{
            externalPendingMessages.push_back(pendingMessage);
        }

        return pendingMessage;
    }


    bool resolveReply(void * responseVoidPointer, NodeId srcNodeId , unsigned requestMessageId){
    	Response * replyObject = (Response *) responseVoidPointer;
    	const bool isLocal = (aggregator->getClusterReadview()->getCurrentNodeId() == srcNodeId);
    	if(replyObject == NULL){
            ASSERT(false);
            return false;
    	}
        ASSERT(isMessageMine(srcNodeId, requestMessageId));

        // 1. find which PendingMessage is responsible
        PendingMessage<Request , Response> * pendingMessage = NULL;
        unsigned pendingMessageLocation = 0;
        // is it the local response or the external one?
        if(isLocal){
            // response is for the local pending message
            for(unsigned pendingMessageIndex = 0; pendingMessageIndex < localPendingMessages.size(); ++pendingMessageIndex){
                PendingMessage<Request, Response> * localPendingMessage = localPendingMessages.at(pendingMessageIndex);
                if(localPendingMessage != NULL && localPendingMessage->doesExpectThisReply(srcNodeId, requestMessageId)){
                	pendingMessage = localPendingMessage;
                	pendingMessageLocation = pendingMessageIndex;
                	break;
                }
            }
        }else{
            // if it's not local, it must be external, this function only is called if this pending message is for
            // this response message
            // iterate on external pending messages and look for nodeId
            for(unsigned pendingMessageIndex = 0; pendingMessageIndex < externalPendingMessages.size(); ++pendingMessageIndex){
                PendingMessage<Request, Response> * externalPendingMessage = externalPendingMessages.at(pendingMessageIndex);
                if(externalPendingMessage != NULL && externalPendingMessage->doesExpectThisReply(srcNodeId, requestMessageId)){
                	pendingMessage = externalPendingMessage;
                	pendingMessageLocation = pendingMessageIndex;
                	break;
                }
            }
        }

        //3. Remove pendingMessage from pendingMessages vector
        // NOTE: this object is not lost, it'll moved to the other vector and freed in
        // destrution.
        if(isLocal){
        	localPendingMessages.at(pendingMessageLocation) = NULL;
        }else{
        	externalPendingMessages.at(pendingMessageLocation) = NULL;
        }

        // 4. set the response message and response object in the PendingMessage
        pendingMessage->setResponseObject(replyObject);

        // 5. put the satisfied pending message in pendingMessagesWithResponse
        if(isLocal){
            localPendingMessagesWithResponse.push_back(pendingMessage);
        }else{
            externalPendingMessagesWithResponse.push_back(pendingMessage);
        }

        // if it's not a waitForAll case we call callback right here.
        if(! shouldWaitForAll()){
            // now we should pass the object to aggregator
            aggregator->callBack(pendingMessage);
        }

        // now check if all pendingMessages are satisfied we are ready for finalizing.
        return checkAllMsgProcessed();

    }

    // this function looks for all messages in this request that have timed out and must be deleted from pendingMessages
    // returns true if it's time for this request to be deleted.
    //  the "timeout" is based on predefined intervals,
    // which does not restart after a new message is added. So the real timeout period could be as large as the parameter * 2.
    bool resolveTimedoutRequests(){
        // move on all pending messages and check if the are timed out or not
        // 1. move on pendingMessages and check them for timeout
        for(unsigned pendingMessageIndex = 0; pendingMessageIndex < localPendingMessages.size(); ++pendingMessageIndex){
            PendingMessage<Request, Response> * pendingMessage = localPendingMessages.at(pendingMessageIndex);
            if(pendingMessage == NULL){
                // if pendingMessageItr is NULL, it's already accepted the response
                continue;
            }
            // the message is timeout, we should move it from pendingMessages to
            // pendingMessagesWithResponse and leave response pointers empty
            if(pendingMessage->isTimedOut()){
                // set the response message and response object to NULL to indicate timeout
                pendingMessage->setResponseObject(NULL);
                // move it to pendingMessagesWithResponse
                localPendingMessagesWithResponse.push_back(pendingMessage);
                // set it null in pendingMessages
                localPendingMessages.at(pendingMessageIndex) = NULL;
                // call timeout of aggregator
                aggregator->processTimeout(pendingMessage, ResponseAggregatorMetadata());
            }
        }
        for(unsigned pendingMessageIndex = 0; pendingMessageIndex < externalPendingMessages.size(); ++pendingMessageIndex){
            PendingMessage<Request, Response> * pendingMessage = externalPendingMessages.at(pendingMessageIndex);
            if(pendingMessage == NULL){
                // if pendingMessageItr is NULL, it's already accepted the response
                continue;
            }
            // the message is timeout, we should move it from pendingMessages to
            // pendingMessagesWithResponse and leave response pointers empty
            if(pendingMessage->isTimedOut()){
                // set the response message and response object to NULL to indicate timeout
                pendingMessage->setResponseObject(NULL);
                // move it to pendingMessagesWithResponse
                externalPendingMessagesWithResponse.push_back(pendingMessage);
                // set it null in pendingMessages
                externalPendingMessages.at(pendingMessageIndex) = NULL;
                // call timeout of aggregator
                aggregator->processTimeout(pendingMessage, ResponseAggregatorMetadata());
            }
        }

        //  now check if all pendingMessages are satisfied we are ready for finalizing.
        return checkAllMsgProcessed();
    }

    // returns true of it contains a PendingMessage corresponding to this responseMessage
    bool isMessageMine(NodeId srcNodeId, unsigned requestMessageId) const{

        for(unsigned pMIdx = 0 ; pMIdx < localPendingMessages.size(); ++pMIdx){
        	if(localPendingMessages.at(pMIdx) == NULL){
        		continue;
        	}
        	if(localPendingMessages.at(pMIdx)->doesExpectThisReply(srcNodeId, requestMessageId)){
        		return true;
        	}
        }
        for(unsigned pMIdx = 0 ; pMIdx < externalPendingMessages.size(); ++pMIdx){
        	if(externalPendingMessages.at(pMIdx) == NULL){
        		continue;
        	}
        	if(externalPendingMessages.at(pMIdx)->doesExpectThisReply(srcNodeId, requestMessageId)){
        		return true;
        	}
        }
        return false;
    }

    // returns true if all pendingMessages have either timed out or received a response
    bool checkAllMsgProcessed(){
        // get a read lock on pendingMessages,
        // this function should only be called within a locked scope
        unsigned numberOfPendingMessagesWithResponse =
        		localPendingMessagesWithResponse.size() +
        		externalPendingMessagesWithResponse.size() ;
        return (numberOfPendingMessagesWithResponse == totalNumberOfPendingMessages);
    }

    // destructor which deallocates all pending messages and calls finalize functions.

private:

    // PendingRequest can only be created in PendingRequestHandler to make sure it's always registered there.
    PendingRequest(MessageAllocator * messageAllocator, bool waitForAll,
            boost::shared_ptr<ResponseAggregatorInterface<Request, Response> > aggregator,
            unsigned totalNumberOfPendingMessages) :
                waitForAll(waitForAll), totalNumberOfPendingMessages(totalNumberOfPendingMessages){
        this->messageAllocator = messageAllocator;

        this->aggregator = aggregator;
        this->aggregator->preProcess(ResponseAggregatorMetadata() );
    }
    ~PendingRequest(){
        // aggregator is wrapped in a shared_ptr so it's deleted when all shared_ptr objects are deleted
        // go over all pending messages and delete them.
    	vector <PendingMessage<Request, Response> *> pendingMessagesWithResponse;
    	for(typename vector <PendingMessage<Request, Response> *>::iterator pendingMessageItr =
    			localPendingMessagesWithResponse.begin() ;
                pendingMessageItr != localPendingMessagesWithResponse.end() ; ++pendingMessageItr){
    		pendingMessagesWithResponse.push_back(*pendingMessageItr);
    	}
    	for(typename vector <PendingMessage<Request, Response> *>::iterator pendingMessageItr =
    			externalPendingMessagesWithResponse.begin() ;
                pendingMessageItr != externalPendingMessagesWithResponse.end() ; ++pendingMessageItr){
    		pendingMessagesWithResponse.push_back(*pendingMessageItr);
    	}
    	if(shouldWaitForAll()){
            // we should call callBackAll here
            this->aggregator->callBack(pendingMessagesWithResponse);
        }
        this->aggregator->finalize(ResponseAggregatorMetadata());
    	for(typename vector <PendingMessage<Request, Response> *>::iterator pendingMessageItr =
    			localPendingMessages.begin() ;
                pendingMessageItr != localPendingMessages.end() ; ++pendingMessageItr){
            ASSERT(*pendingMessageItr == NULL);
    	}
    	for(typename vector <PendingMessage<Request, Response> *>::iterator pendingMessageItr = externalPendingMessages.begin() ;
                pendingMessageItr != externalPendingMessages.end() ; ++pendingMessageItr){
            ASSERT(*pendingMessageItr == NULL);
    	}

    	for(typename vector <PendingMessage<Request, Response> *>::iterator pendingMessageItr = pendingMessagesWithResponse.begin() ;
                pendingMessageItr != pendingMessagesWithResponse.end() ; ++pendingMessageItr){
    		if(*pendingMessageItr != NULL){
    			delete *pendingMessageItr;
    		}
    	}
    }

    MessageAllocator * messageAllocator;


    // This vector contains the pointer to all pending messages which have not received any responses
    vector<PendingMessage<Request, Response> *> localPendingMessages;
    vector<PendingMessage<Request, Response> *> externalPendingMessages;

    // When a response comes, it's passed to the corresponding pendingMessage by
    // iterating over pendingMessages. This pendingMessage then stores the response message and
    // object and if it's a waitForAll we store that in this vector. Otherwise, it's directly passed to aggregator
    // functions.
    vector <PendingMessage<Request, Response> *> localPendingMessagesWithResponse;
    vector <PendingMessage<Request, Response> *> externalPendingMessagesWithResponse;

    // indicates whether PendingMessage needs to wait for all PendingMessages to resolve to call
    // aggregators callback or it can call the callback upon receiving of each one.
    const bool waitForAll;

    boost::shared_ptr<ResponseAggregatorInterface<Request, Response> > aggregator;

    // This member gives us the total number of pending messages that will be registered in this
    // pending request. If waitForAll is true we call CallBackAll only if we have this many pendingMessages
    // and all of them have response message stored in them.
    const unsigned totalNumberOfPendingMessages;

};


}}

#endif /* __SHARDING_ROUTING_PENDING_MESSAGES_H__ */
