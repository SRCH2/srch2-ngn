#ifndef __SHARDING_ROUTING_PENDING_MESSAGES_H__
#define __SHARDING_ROUTING_PENDING_MESSAGES_H__

#include "sharding/configuration/ShardingConstants.h"
#include "sharding/transport/Message.h"
#include "sharding/transport/RouteMap.h"
#include "sharding/processor/ResultsAggregatorAndPrint.h"
#include "core/util/Assert.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "RoutingUtil.h"
#include <vector>
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
    void setResponseMessageAndObject(Message* responseMessage, Response * responseObj){
        ASSERT(this->responseMessage == NULL);
        this->responseMessage = responseMessage;
        this->responseObject = responseObj;
    }
    // used to access the response message
    Message * getResponseMessage(){
        return this->responseMessage;
    }
    Response * getResponseObject(){
        return this->responseObject;
    }

    Message * getRequestMessage(){
        return this->requestMessage;
    }

    Request * getRequestObject(){
        return this->requestObject;
    }
    // returns true if the nodeId that we expect the response from is equal to this nodeId
    bool doesExpectFromThisShard(NodeId nodeid){
        return (this->nodeId == nodeid);
    }

    // returns true if the value of timeout is before the current time
    bool isTimedOut(){
        if(this->timeout == 0){ // zero indicates no timeout
            return false;
        }
        time_t currentTime;
        time(&currentTime);
        Logger::console("Time to check timeout : %d" , currentTime);
        return (this->timeout < currentTime);
    }

    // returns the destination node id of this pendingMessage
    NodeId getNodeId(){
        return this->nodeId;
    }

private:
    // the constructor is private because PendingMessage object should only
    // be allocated through PendingRequest so that it also gets stored in that object
    PendingMessage(MessageAllocator * messageAllocator, NodeId nodeId, time_t timeout,
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

    ~PendingMessage(){

        // request object and message are protected by shared pointer.

        if(responseMessage != NULL){
            messageAllocator->deallocateByMessagePointer(responseMessage);
            ASSERT(responseObject != NULL);
            delete responseObject;
        } // if they are null it means this request had timed out
    }


    MessageAllocator * messageAllocator;

    // the absolute time on which this message times out
    time_t timeout;
    // the request message
    Message * requestMessage;
    // the request object
    Request * requestObject;
    // Node id of the shard which we are waiting for response from
    NodeId nodeId; // TODO : node ID should change to shard Id in V1 because we can have multiple shards in a node
    // the reply message, this member is NULL in the beginning and gets filled only
    // if a response comes for this request message
    Message* responseMessage; // NULL in the beginning
    // The reply object which is produced after deserialization.
    Response * responseObject; // NULL in the beginning.
};


/*
 * We need this class to be father of PendingRequest because
 * PendingRequestHandler needs polymorphism to store all PendingRequest objects.
 */
class PendingRequestAbstract{
public:
    // resolves the corresponding PendingMessage with this response
    // if returns true, this pendingRequest is ready to be deleted for finalizing.
    virtual bool resolveResponseMessage(Message * responseMessage, NodeId nodeIdOfResponse, void * responseObject) = 0;
    // this function looks for all messages in this request that have timed out and must be deleted from pendingMessages
    // returns true if it's time for this request to be deleted.
    virtual bool resolveTimedoutMessages() = 0;
    // returns true of it contains a PendingMessage corresponding to this responseMessage
    virtual bool isResponseMessageMine(Message * responseMessage) = 0;
    virtual ~PendingRequestAbstract(){};
};


/*
 * Pending request is the object which keeps the information of a request pending for responses.
 * Some requests like search produce multiple messages sent to other shards. For this reason a single object
 * PendingRequest is made and multiple instances of PendingMessage point to this single object. Therefore,
 * PendingRequest object is accessed by multiple threads and must be a thread safe object.
 */
template <class Request, class Response>
class PendingRequest : public PendingRequestAbstract{
    friend class PendingRequestsHandler;
public:


    // returns whether this PendingRequest waits for all replies to call callBackAll or
    // calls callBack functions separately.
    bool shouldWaitForAll(){return waitForAll;};

    // this function allocates and returns a PendingMessage. PendingMessage objects
    // are always allocated and deleted in this class.
    // this function also adds this PendingMessage to the map.
    PendingMessage<Request, Response> * registerPendingMessage(NodeId nodeId, time_t timeout,
            Message* requestMessage,
            Request* requestObject){
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
            // We assume the local message is always stored at the 0th slot.
        }else{
            this->externalRequestMessageId = requestMessage->getMessageId();
            ASSERT(pendingMessages.size() >= 1);
            pendingMessages.push_back(pendingMessage);
        }

        return pendingMessage;
    }

    // resolves the corresponding PendingMessage with this response
    // if returns true, this pendingRequest is ready to be deleted for finalizing.
    bool resolveResponseMessage(Message * responseMessage, NodeId nodeIdOfResponse, void * responseObjectTypeVoid = NULL){
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
        // NOTE: this object is not lost, it'll move to the other vector and freed in
        // destrution.
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
        return checkAllMsgProcessed();
    }

    // this function looks for all messages in this request that have timed out and must be deleted from pendingMessages
    // returns true if it's time for this request to be deleted.
    //  the "timeout" is based on predefined intervals,
    // which does not restart after a new message is added. So the real timeout period could be as large as the parameter * 2.
    bool resolveTimedoutMessages(){
        // move on all pending messages and check if the are timed out or not
        // 1. get an X lock on the pendingMessages vector
        boost::unique_lock< boost::shared_mutex > lock(_access);
        ASSERT(pendingMessages.size() >= 1); // local pointer is always NULL, even though if it's NULL
        // 2. move on pendingMessages and check them for timeout
        for(unsigned pendingMessageIndex = 0; pendingMessageIndex < pendingMessages.size(); ++pendingMessageIndex){
            PendingMessage<Request, Response> * pendingMessage = pendingMessages.at(pendingMessageIndex);
            if(pendingMessage == NULL){
                // if pendingMessageItr is NULL, it's either the local one which has not come yet
                // or it's already accepted the response
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
                aggregator->processTimeout(pendingMessage, ResponseAggregatorMetadata());
            }
        }

        //  now check if all pendingMessages are satisfied we are ready for finalizing.
        return checkAllMsgProcessed();
    }

    // returns true of it contains a PendingMessage corresponding to this responseMessage
    bool isResponseMessageMine(Message * responseMessage){
        // get a read lock on pendingMessages,
        boost::shared_lock< boost::shared_mutex > lock(_access);
        return (this->localRequestMessageId == responseMessage->getRequestMessageId() ||
                this->externalRequestMessageId == responseMessage->getRequestMessageId());
    }

    // returns true if all pendingMessages have either timed out or received a response
    bool checkAllMsgProcessed(){
        // get a read lock on pendingMessages,
        // this function should only be called within a locked scope
        unsigned numberOfPendingMessagesWithResponse = pendingMessagesWithResponse.size() ;
        // because the first one the place holder for local pending message and it's always there
        if(pendingMessagesWithResponse.at(0) == NULL){
            numberOfPendingMessagesWithResponse--;
        }
        return (numberOfPendingMessagesWithResponse == totalNumberOfPendingMessages);
    }

    // destructor which deallocates all pending messages and calls finalize functions.

private:

    // PendingRequest can only be created in PendingRequestHandler to make sure it's always registered there.
    PendingRequest(MessageAllocator * messageAllocator, bool waitForAll,
            boost::shared_ptr<ResponseAggregator<Request, Response> > aggregator,
            unsigned totalNumberOfPendingMessages) :
                waitForAll(waitForAll), totalNumberOfPendingMessages(totalNumberOfPendingMessages){
        this->messageAllocator = messageAllocator;

        this->aggregator = aggregator;
        this->aggregator->preProcess(ResponseAggregatorMetadata() );
        // index zero is for the local node.
        pendingMessages.push_back(NULL);
        pendingMessagesWithResponse.push_back(NULL);
        // initialize these members to avoid garbage values
        localRequestMessageId = 0;
        externalRequestMessageId = 0;
    }
    ~PendingRequest(){
        // aggregator is wrapped in a shared_ptr so it's deleted when all shared_ptr objects are deleted
        // go over all pending messages and delete them.
        if(shouldWaitForAll()){
            // we should call callBackAll here
            this->aggregator->callBack(pendingMessagesWithResponse);
        }
        this->aggregator->finalize(ResponseAggregatorMetadata());
        for(typename vector <PendingMessage<Request, Response> *>::iterator pendingMessageItr = pendingMessages.begin() ;
                pendingMessageItr != pendingMessages.end() ; ++pendingMessageItr){
            ASSERT(*pendingMessageItr == NULL);
        }


        // request message and object are shared so they must be deleted only once.
        // delete local request message
        if(pendingMessagesWithResponse.at(0) != NULL){ // for example for insert, local pending message can be NULL
            messageAllocator->deallocateByMessagePointer(pendingMessagesWithResponse.at(0)->requestMessage);
        }
        // delete external request message
        if(pendingMessagesWithResponse.size() > 1){
            ASSERT(pendingMessagesWithResponse.at(1) != NULL);
            messageAllocator->deallocateByMessagePointer(pendingMessagesWithResponse.at(1)->requestMessage);
        }
        // delete request object which is even shared between local and external messages
        if(pendingMessagesWithResponse.at(0) != NULL){
            delete pendingMessagesWithResponse.at(0)->requestObject;
        }else{
            if(pendingMessagesWithResponse.size() > 1){
                ASSERT(pendingMessagesWithResponse.at(1) != NULL);
                delete pendingMessagesWithResponse.at(1)->requestObject;
            }
        }


        for(typename vector <PendingMessage<Request, Response> *>::iterator resolvedPendingMessageItr = pendingMessagesWithResponse.begin() ;
                resolvedPendingMessageItr != pendingMessagesWithResponse.end() ; ++resolvedPendingMessageItr){
            ASSERT(resolvedPendingMessageItr == pendingMessagesWithResponse.begin() ||
                    *resolvedPendingMessageItr != NULL);
            if(*resolvedPendingMessageItr != NULL){
                delete *resolvedPendingMessageItr;
            }
        }
    }

    MessageAllocator * messageAllocator;

    // this lock is used to make PendingRequest thread safe
    mutable boost::shared_mutex _access;

    // local and external message ids corresponding to this request.
    MessageID_t localRequestMessageId;
    MessageID_t externalRequestMessageId;

    // This vector contains the pointer to all pending messages which have not received any responses
    // NOTE: index 0 of this vector points to the pending message corresponding to the local
    // message sent for this request. from index 1 we keep pending messages of external shards.
    vector<PendingMessage<Request, Response> *> pendingMessages;

    // When a response comes, it's passed to the corresponding pendingMessage by
    // iterating over pendingMessages. This pendingMessage then stores the response message and
    // object and if it's a waitForAll we store that in this vector. Otherwise, it's directly passed to aggregator
    // functions.
    vector <PendingMessage<Request, Response> *> pendingMessagesWithResponse;

    // indicates whether PendingMessage needs to wait for all PendingMessages to resolve to call
    // aggregators callback or it can call the callback upon receiving of each one.
    const bool waitForAll;

    // the aggregator object
    // the reason we use a shared pointer instead of a real pointer is this :
    // for some cases like when we receive a batch of inserts, multiple PendingRequest objects
    // (one per each insert) need to have a single aggregator (batch aggregator), therefore, we
    // the aggregator to be deleted when the last PendingRequest object is deleted.
    boost::shared_ptr<ResponseAggregator<Request, Response> > aggregator;

    // This member gives us the total number of pending messages that will be registered in this
    // pending request. If waitForAll is true we call CallBackAll only if we have this many pendingMessages
    // and all of them have response message stored in them.
    const unsigned totalNumberOfPendingMessages;

};


/*
 * this class maintains all pending requests and resolves them upon receiving a response
 * or timeout.
 * This object must be thread safe because it's accessed by multiple threads
 */
class PendingRequestsHandler{
public:

    template <class Request, class Response>
    PendingRequest<Request, Response> * registerPendingRequest(bool waitForAll,
            boost::shared_ptr<ResponseAggregator<Request, Response> > aggregator,
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

    // TODO : nodeId should be replaced with shardId
    /*
     * If this response is not related to any pending requests, returns false
     */
    bool resolveResponseMessage(Message * response, NodeId nodeId, void * responseObject = NULL){
    	// TODO : linear scan of pendingRequests vector can be a performance concern in high query rates
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
                    delete *pendingRequestItr;
                    pendingRequests.erase(pendingRequestItr);
                }
                return true;
            }
        }

        return false;
    }

    void resolveTimedoutMessages(){
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

    static void * timeoutHandler(void * arg){
        PendingRequestsHandler * pendingRequestsHandler = (PendingRequestsHandler *) arg;
        while(true){
            // looks for timed out messages
            pendingRequestsHandler->resolveTimedoutMessages();
            // sleep for 2 seconds
            sleep(2);
        }
        return NULL;
    }

    PendingRequestsHandler(MessageAllocator * messageAllocator){

        this->messageAllocator = messageAllocator;

        if (pthread_create(&(this->timeoutThread), NULL, PendingRequestsHandler::timeoutHandler, this) != 0){
            perror("Cannot create thread for timeout for pending messages.");
            return;
        }
    }
    ~PendingRequestsHandler(){
        pthread_cancel(this->timeoutThread);
    }
private:

    // Keeps the object thread safe
    mutable boost::shared_mutex _access;

    MessageAllocator * messageAllocator;
    // This vector contains all pending requests which are waiting for response(s)
    vector<PendingRequestAbstract *> pendingRequests;

    pthread_t timeoutThread;
};

}}

#endif /* __SHARDING_ROUTING_PENDING_MESSAGES_H__ */
