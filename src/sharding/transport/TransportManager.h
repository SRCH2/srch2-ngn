#ifndef __TRANSPORT_MANAGER_H__
#define  __TRANSPORT_MANAGER_H__

#include "RouteMap.h"
#include <event.h>
#include<pthread.h>
#include "Message.h"
#include "MessageAllocator.h"
#include "PendingMessages.h"


#include "CallbackHandler.h"

namespace srch2 {
namespace httpwrapper {

typedef std::vector<event_base*> EventBases;
typedef std::vector<Node> Nodes;

class TransportManager {
public:


	TransportManager(EventBases&, Nodes&);

	//third argument is a timeout in seconds
	MessageTime_t route(NodeId, Message*, unsigned=0, CallbackReference=CallbackReference());
  //route message through a particular socket
	MessageTime_t route(int fd, Message*);
	CallbackReference registerCallback(void*,Callback*,
			ShardingMessageType,bool=false,int = 1);
	void registerCallbackHandlerForSynchronizeManager(CallBackHandler*);




	MessageTime_t& getDistributedTime();
	CallBackHandler* getInternalTrampoline();
	void setInternalTrampoline(CallBackHandler*);
	pthread_t getListeningThread() const;
	MessageAllocator * getMessageAllocator();
	PendingMessages * getMsgs();
	RouteMap * getRouteMap();
	CallBackHandler* getSmHandler();


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
	MessageTime_t distributedTime;

	/*
	 * The allocator used for messaging
	 */
	MessageAllocator messageAllocator;

	/*
	 * the data structure which stores all pending messages in this node
	 */
	PendingMessages pendingMessages;

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

inline CallbackReference TransportManager::registerCallback(void* obj,
		Callback* cb, ShardingMessageType type,bool all,int shards) {
	return pendingMessages.registerCallback(obj, cb, type, all, shards);
}

inline void TransportManager::setInternalTrampoline(CallBackHandler* cbh) {
  internalTrampoline = cbh;
}
}}

#endif /* __TRANSPORT_MANAGER_H__ */
