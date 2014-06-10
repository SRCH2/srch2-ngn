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
    void setResponseMessageAndObject(Message* responseMessage, Response * responseObj);
    // used to access the response message
    Message * getResponseMessage();
    Response * getResponseObject();

    Message * getRequestMessage();
    Request * getRequestObject();
    // returns true if the nodeId that we expect the response from is equal to this nodeId
    bool doesExpectFromThisShard(NodeId nodeid);

    // returns true if the value of timeout is before the current time
    bool isTimedOut();

    // returns the destination node id of this pendingMessage
    NodeId getNodeId();

private:
    // the constructor is private because PendingMessage object should only
    // be allocated through PendingRequest so that it also gets stored in that object
    PendingMessage(MessageAllocator * messageAllocator, NodeId nodeId, time_t timeout,
            Message * requestMessage,
            Request * requestObject);
    ~PendingMessage();


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
            Request* requestObject);

    // resolves the corresponding PendingMessage with this response
    // if returns true, this pendingRequest is ready to be deleted for finalizing.
    bool resolveResponseMessage(Message * responseMessage, NodeId nodeIdOfResponse, void * responseObject = NULL);

    // this function looks for all messages in this request that have timed out and must be deleted from pendingMessages
    // returns true if it's time for this request to be deleted.
    bool resolveTimedoutMessages();

    // returns true of it contains a PendingMessage corresponding to this responseMessage
    bool isResponseMessageMine(Message * responseMessage);

    // returns true if all pendingMessages have either timed out or received a response
    bool shouldFinalize();

    // destructor which deallocates all pending messages and calls finalize functions.

private:

    // PendingRequest can only be created in PendingRequestHandler to make sure it's always registered there.
    PendingRequest(MessageAllocator * messageAllocator, bool waitForAll,
            boost::shared_ptr<ResultAggregatorAndPrint<Request, Response> > aggregator,
            unsigned totalNumberOfPendingMessages) :
                waitForAll(waitForAll), totalNumberOfPendingMessages(totalNumberOfPendingMessages){
        this->messageAllocator = messageAllocator;

        this->aggregator = aggregator;
        this->aggregator->preProcessing(ResultsAggregatorAndPrintMetadata() );
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
        this->aggregator->finalize(ResultsAggregatorAndPrintMetadata());
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
    boost::shared_ptr<ResultAggregatorAndPrint<Request, Response> > aggregator;

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
            boost::shared_ptr<ResultAggregatorAndPrint<Request, Response> > aggregator,
            unsigned totalNumberOfPendingMessages);

    // TODO : nodeId should be replaced with shardId
    /*
     * If this response is not related to any pending requests, returns false
     */
    bool resolveResponseMessage(Message * response, NodeId nodeId, void * responseObject = NULL);

    void resolveTimedoutMessages();

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

    PendingRequestsHandler(MessageAllocator * messageAllocator);
    ~PendingRequestsHandler();
private:

    // Keeps the object thread safe
    mutable boost::shared_mutex _access;

    MessageAllocator * messageAllocator;
    // This vector contains all pending requests which are waiting for response(s)
    vector<PendingRequestAbstract *> pendingRequests;

    pthread_t timeoutThread;
};

}}

/*
 * Since we have template classes in this file we have to implement the methods in the header file
 * so we include another header file here which serves as the CPP file.
 */
#include "PendingMessagesCPP.h"

#endif /* __SHARDING_ROUTING_PENDING_MESSAGES_H__ */
