#ifndef __SHARDING_SHARDING_STATE_H__
#define __SHARDING_SHARDING_STATE_H__

#include "../ShardManager.h"
#include "../notifications/Notification.h"
#include "server/HTTPJsonResponse.h"
#include "ConsumerProducer.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * This class provides the interface of the state of one operation
 */
class OperationState{
public:

	OperationState(unsigned operationId):operationId(operationId){
		transaction.reset();
	}

	// NOTE : OperationState must always be in locked state before destruction
	//        so that destruction starts in safe
	virtual ~OperationState(){
	};

	virtual OperationState * entry() = 0;
	// it returns this, or next state or NULL.
	// if it returns NULL, we delete the object.
	virtual OperationState * handle(SP(Notification) n) = 0;


	virtual string getOperationName() const {return "operation name";};
	virtual string getOperationStatus() const {return "operation status";};

	unsigned getOperationId() const;
	void setOperationId(unsigned operationId) ;

	void send(SP(ShardingNotification) notification, const NodeOperationId & dest) const;

	SP(Transaction) getTransaction();
	void setTransaction(SP(Transaction) sp);
	void lock();
	void unlock();
	static void initOperationStateStaticEnv();
	static unsigned getNextOperationId();
	static const unsigned DataRecoveryOperationId;

private:
	unsigned operationId;
	static unsigned nextOperationId;
	static pthread_mutex_t operationIdMutex;
	SP(Transaction) transaction;
	boost::mutex operationContentLock;
};

}
}

#endif // __SHARDING_SHARDING_STATE_H__
