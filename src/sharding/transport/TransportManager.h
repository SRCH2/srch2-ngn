#ifndef __TRANSPORT_MANAGER_H__
#define  __TRANSPORT_MANAGER_H__

#include "RouteMap.h"
#include <event.h>
#include<pthread.h>
#include "Message.h"
#include "MessageAllocator.h"
#include "CallbackHandler.h"
#include <boost/thread.hpp>
namespace srch2 {
namespace httpwrapper {

typedef std::vector<event_base*> EventBases;
typedef std::vector<Node> Nodes;
class TransportManager ;

struct DisptchArguments{

	DisptchArguments(TransportManager * tm, Message * message, int fd, NodeId id){
		this->tm = tm;
		this->message = message;
		this->fd = fd;
		this->nodeId = id;
	}

	TransportManager * tm;
	Message * message;
	int fd;
	NodeId nodeId;
};

struct TransportCallback {
	TransportManager *const tm;
	Connection *const conn;
	struct event* ev;
	const struct event_base *const base;

	TransportCallback(TransportManager *tm, Connection *c, event* e,
			event_base* b) : tm(tm), conn(c), ev(e), base(b) {}
	TransportCallback() : tm(NULL), conn(NULL), ev(NULL), base(NULL) {}
};

class RoutingManager;
/*
 * This struct is for abstracting timeout and callback behaviour
 * currently used for TM
 */
class Callback {
public:
	virtual void timeout(void*) = 0;
	virtual void callback(Message*) {};
	virtual void callbackAll(vector<Message*>&) {};
	virtual ~Callback(){};
};

bool recieveMessage(int fd, TransportCallback *cb);

class TransportManager {
public:


	TransportManager(EventBases&, Nodes&);

	//third argument is a timeout in seconds
	MessageID_t route(NodeId,Message *, unsigned timeout=0);
  //route message through a particular socket
	MessageID_t _route(int fd, Message *);
	void registerCallbackHandlerForSynchronizeManager(CallBackHandler*);

	MessageID_t& getDistributedTime();
	MessageID_t getUniqueMessageIdValue();
	void setInternalMessageBroker(CallBackHandler*);
	pthread_t getListeningThread() const;
	MessageAllocator * getMessageAllocator();
	RoutingManager * getRoutingManager();
	void setRoutingManager(RoutingManager * rm);
	RouteMap * getRouteMap();
	CallBackHandler* getSmHandler();
	CallBackHandler* getRmHandler();
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
	 * Handles SynchManager callbacks
	 */
	CallBackHandler *synchManagerHandler;

	/*
	 * Handles internal message broker callbacks
	 */
	CallBackHandler *routeManagerHandler;

	/*
	 *  Stores the default socket read buffer size
	 */
	unsigned socketReadBuffer;
	/*
	 *  Stores the default socket write buffer size
	 */
	unsigned socketSendBuffer;

	/*
	 * Routing Manager
	 */
	RoutingManager * routingManager;
};

// TODO : please move them to TM cpp
inline void TransportManager::registerCallbackHandlerForSynchronizeManager(CallBackHandler
		*callBackHandler) {
	synchManagerHandler = callBackHandler;
}

inline void TransportManager::setInternalMessageBroker(CallBackHandler* cbh) {
  routeManagerHandler = cbh;
}
}}

#endif /* __TRANSPORT_MANAGER_H__ */
