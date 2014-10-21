#ifndef __SHARDING_SHARDING_STATE_H__
#define __SHARDING_SHARDING_STATE_H__

#include "ShardManager.h"
#include "./notifications/Notification.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class BottomUpDeleteInterface{
public:
	virtual ~BottomUpDeleteInterface(){};
	BottomUpDeleteInterface();
	void setTransIdToDelete(TRANS_ID id);
	void connectDeletePathToParent(BottomUpDeleteInterface * parent);
	TRANS_ID getTransIdToDelete();


private:
	// if == 0 : return NULL
	// if == 1 : return TID (TID is set)
	// if == 2 : return parent->getTransIdToDelete()
	unsigned tidToDeletePolicy;
	TRANS_ID transId;
	BottomUpDeleteInterface * parent;
};
/*
 * This class provides the interface of the state of one operation
 */
class OperationState : public BottomUpDeleteInterface{
public:

	OperationState(unsigned operationId):operationId(operationId){
	}

	virtual ~OperationState(){};

	virtual OperationState * entry() = 0;
	// it returns this, or next state or NULL.
	// if it returns NULL, we delete the object.
	virtual OperationState * handle(Notification * n) = 0;


	virtual string getOperationName() const {return "operation name";};
	virtual string getOperationStatus() const {return "operation status";};

	unsigned getOperationId() const;
	void setOperationId(unsigned operationId) ;

	void send(ShardingNotification * notification, const NodeOperationId & dest) const;

	// what's returned doesn't need to be started but it might be NULL
	static OperationState * startOperation(OperationState * op);

	static void stateTransit(OperationState * & currentState, Notification * notification);
	static unsigned getNextOperationId();
	static const unsigned DataRecoveryOperationId;
private:
	unsigned operationId;
	static unsigned nextOperationId;
};


class TopDownDeleteInterface{
public:
	virtual ~TopDownDeleteInterface(){};
	TopDownDeleteInterface();
	bool isAttachedToOtherThread();
	void setAttachedToOtherThread(const bool attached = true);
private:
	bool attachedToOtherThread;

};


class ConsumerInterface : public BottomUpDeleteInterface{
public:
	virtual ~ConsumerInterface(){};
	virtual void consume(bool booleanResult){};
	virtual void consume(const vector<string> & rejectedPKs){};
};


class ProducerInterface : public TopDownDeleteInterface, public BottomUpDeleteInterface{
	virtual ~ProducerInterface(){};
	virtual void produce() = 0;
	virtual void receiveStatus(const ShardMigrationStatus & migrationStatus){};
};

/*
 * This class must be implemented by every transaction class
 * which want to connect to the state machine.
 */
class Transaction : public TopDownDeleteInterface{
public:
	static void startTransaction(Transaction * trans);
public:

	Transaction():transactionId(OperationState::getNextOperationId()){	}
	virtual ~Transaction() {};

	TRANS_ID getTID() const ;

	virtual ShardingTransactionType getTransactionType() = 0 ;
	virtual void run() {};
	static void deallocateTransaction(Transaction * t);
private:
	const TRANS_ID transactionId;
};

}
}

#endif // __SHARDING_SHARDING_STATE_H__
