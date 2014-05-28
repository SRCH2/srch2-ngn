#ifndef __TRANSPORT_MANAGER_H__
#define  __TRANSPORT_MANAGER_H__

#include "RouteMap.h"
#include <event.h>
#include<pthread.h>
#include "Message.h"
#include "MessageAllocator.h"
#include "PendingMessages.h"


#include "CallbackHandler.h"
#include <boost/thread.hpp>
namespace srch2 {
namespace httpwrapper {

typedef std::vector<event_base*> EventBases;
typedef std::vector<Node> Nodes;

class TransportManager {
public:


	TransportManager(EventBases&, Nodes&);

	//third argument is a timeout in seconds
	MessageID_t route(NodeId, Message*, unsigned=0, CallbackReference=CallbackReference());
  //route message through a particular socket
	MessageID_t route(int fd, Message*);
	CallbackReference prepareCallback(void*,Callback*,
			ShardingMessageType,bool=false,int = 1);
	void registerCallbackHandlerForSynchronizeManager(CallBackHandler*);




	MessageID_t& getDistributedTime();
	CallBackHandler* getInternalTrampoline();
	void setInternalMessageBroker(CallBackHandler*);
	pthread_t getListeningThread() const;
	MessageAllocator * getMessageAllocator();
	PendingMessagesHandler * getPendingMessagesHandler();
	RouteMap * getRouteMap();
	CallBackHandler* getSmHandler();
   ~TransportManager();

private:
	/*
	 * This member maps nodes to their sockets
	 */
	RouteMap routeMap;

	/*
	 * The thread that the current node is listening for the incoming request from new nodes
	 */
	pthread_t listeningThread;

	/*
	 * The current distributed time of system
	 * It is synchronized in cb_recieveMessage(int fd, short eventType, void *arg)
	 */
	MessageID_t distributedTime;

	/*
	 * The allocator used for messaging
	 */
	MessageAllocator messageAllocator;

	/*
	 * the data structure which stores all pending messages in this node
	 */
	PendingMessagesHandler pendingMessagesHandler;

	/*
	 * Handles SynchManager callbacks
	 */
	CallBackHandler *synchManagerHandler;

	/*
	 * Handles internal message broker callbacks
	 */
	CallBackHandler *internalTrampoline;
};

inline void TransportManager::registerCallbackHandlerForSynchronizeManager(CallBackHandler
		*callBackHandler) {
	synchManagerHandler = callBackHandler;
}

inline CallbackReference TransportManager::prepareCallback(void* requestObj,
		Callback* cb, ShardingMessageType type,bool all,int shards) {
	return pendingMessagesHandler.prepareCallback(requestObj, cb, type, all, shards);
}

inline void TransportManager::setInternalMessageBroker(CallBackHandler* cbh) {
  internalTrampoline = cbh;
}
}}

#endif /* __TRANSPORT_MANAGER_H__ */
