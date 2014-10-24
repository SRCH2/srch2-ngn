#ifndef __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__
#define __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__

#include "../../configuration/ShardingConstants.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {

class TransactionSession;

/*
 * This class must be implemented by every transaction class
 * which want to connect to the state machine.
 */
class Transaction {
public:
	static void startTransaction(Transaction * trans);
public:

	Transaction();
	virtual ~Transaction();

	TRANS_ID getTID() const ;

	virtual ShardingTransactionType getTransactionType() = 0 ;
	virtual bool run() {
		return false;
	};
	virtual void initSession() = 0;

	void setAttached();
	void setUnattached();
	bool isAttached() const;

	bool isFinished() const;
	TransactionSession * getSession();
private:
	const TRANS_ID transactionId;
	bool attachedFlag;
	bool finishedFlag;
	TransactionSession * session;
protected:
	void setSession(TransactionSession * session);
	// only this class and its children can call this method
	void setFinished(){
		this->finishedFlag = true;
	}
};

}
}

#endif // __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__
