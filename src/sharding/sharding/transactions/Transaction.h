#ifndef __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__
#define __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__

#include "../../configuration/ShardingConstants.h"
#include "../notifications/Notification.h"

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

	/*
	 * these two methods are called by state machine before and after an operation
	 * 	handles a notification (or executes entry())
	 * 	NOTE : preProcess is not called before entry() and postProcess() is not called when we are going
	 * 	to delete the transaction.
	 */
	virtual void preProcess() = 0;
	virtual void postProcess() = 0;

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
	SP(ClusterNodes_Writeview) getNodesWriteview_write() const;
	SP(ClusterNodes_Writeview) getNodesWriteview_read() const;
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


class ReadviewTransaction : public Transaction{
public:

	// nodesWriteview must come in locked state
	ReadviewTransaction(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview);

	void preProcess(){};
	void postProcess(){};

	boost::shared_ptr<const ClusterResourceMetadata_Readview> getReadview() const;
private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
};

class WriteviewTransaction : public Transaction{
public:
	WriteviewTransaction();
	void preProcess();
	void postProcess();
	Cluster_Writeview * getWriteview();
private:
	Cluster_Writeview * writeview;
	boost::unique_lock<boost::mutex> xLock; // when this is destroyed, writeview lock will be released.
	bool automaticLocking;
};

}
}

#endif // __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__
