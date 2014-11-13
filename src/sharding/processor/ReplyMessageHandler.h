#ifndef __SHARDING_ROUTING_INCOMING_MESSAGE_HANDLER_H__
#define __SHARDING_ROUTING_INCOMING_MESSAGE_HANDLER_H__

#include "sharding/transport/CallbackHandler.h"
#include "PendingMessages.h"


namespace srch2 {
namespace httpwrapper {

/*
 * this class maintains all pending requests and resolves them upon receiving a response
 * or timeout.
 * This object must be thread safe because it's accessed by multiple threads
 */
class ReplyMessageHandler : public CallBackHandler{
public:

    template <class Request, class Response>
    PendingRequest<Request, Response> * registerPendingRequest(bool waitForAll,
            boost::shared_ptr<ResponseAggregatorInterface<Request, Response> > aggregator,
            unsigned totalNumberOfPendingMessages){
        // create the pendingRequest
        PendingRequest<Request, Response> * pendingRequest =
        		new PendingRequest<Request, Response>(messageAllocator,
        				waitForAll,
        				aggregator,
        				totalNumberOfPendingMessages);


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
    bool resolveMessage(Message * reply, NodeId nodeId){

    	if(reply == NULL){
    		return NULL;
    	}
    	ASSERT(reply->isDPReply());
    	switch (reply->getType()) {
			case GetInfoResultsMessageType:
			{
				GetInfoCommandResults * getInfoResult =
						GetInfoCommandResults::deserialize(Message::getBodyPointerFromMessagePointer(reply));
				return resolveReply(getInfoResult, nodeId , reply->getRequestMessageId());
			}
			default:
			{
				ASSERT(false);
				return false;
			}
		}
    	return false;
    }
    /*
     * If this response is not related to any pending requests, returns false
     * and if it returns false it's callers responsibility to deallocate the reply message
     */
    bool resolveReply(void * responseObject, NodeId srcNodeId, unsigned requestMessageId){
		// TODO : linear scan of pendingRequests vector can be a performance concern in high query rates
		// 1. get an X lock on the pendingRequest list
		boost::unique_lock< boost::shared_mutex > lock(_access);
		// 2. iterate on PendingRequests and check if any of them is corresponding to this response
		for(vector<ReplyHandlerInterface *>::iterator messageResolverItr = pendingRequests.begin();
				messageResolverItr != pendingRequests.end() ; ++messageResolverItr){
			if((*messageResolverItr)->isMessageMine(srcNodeId, requestMessageId)){
				// ask this pending request to resolve this response
				bool resolveResult = (*messageResolverItr)->resolveReply(responseObject, srcNodeId, requestMessageId);
				// if returns true, we must delete this PendingRequest from the vector.
				if(resolveResult){
					delete *messageResolverItr;
					pendingRequests.erase(messageResolverItr);
				}
				return true;
			}
		}

		return false;
    }

    ReplyMessageHandler(MessageAllocator * messageAllocator){

        this->messageAllocator = messageAllocator;

        if (pthread_create(&(this->timeoutThread), NULL, ReplyMessageHandler::timeoutHandler, this) != 0){
            perror("Cannot create thread for timeout for pending messages.");
            return;
        }
    }
    ~ReplyMessageHandler(){
        pthread_cancel(this->timeoutThread);
    }
private:

    static void * timeoutHandler(void * arg){
    	ReplyMessageHandler * messageHandler = (ReplyMessageHandler *) arg;
        while(true){
            // looks for timed out messages
        	messageHandler->resolveTimedoutMessages();
            // sleep for 2 seconds
            sleep(2);
        }
        return NULL;
    }

    void resolveTimedoutMessages(){
        // 1. get a X lock on the list if pending requests
        boost::unique_lock< boost::shared_mutex > lock(_access);
        // 2. move on all pending requests and check for timeout,
        vector<ReplyHandlerInterface *> newPendingRequests;
        for(vector<ReplyHandlerInterface *>::iterator pendingRequestItr = pendingRequests.begin();
                pendingRequestItr != pendingRequests.end() ; ++pendingRequestItr){
            bool resolveResult = (*pendingRequestItr)->resolveTimedoutRequests();
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

    // Keeps the object thread safe
    mutable boost::shared_mutex _access;

    MessageAllocator * messageAllocator;
    // This vector contains all pending requests which are waiting for response(s)
    vector<ReplyHandlerInterface *> pendingRequests;

    pthread_t timeoutThread;
};



}
}

#endif // __SHARDING_ROUTING_INCOMING_MESSAGE_HANDLER_H__
