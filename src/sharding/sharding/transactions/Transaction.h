#ifndef __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__
#define __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__

#include "../../configuration/ShardingConstants.h"
#include "../notifications/Notification.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {

class TransactionSession;
class Cluster_Writeview;
class ClusterNodes_Writeview;
class ClusterResourceMetadata_Readview;
/*
 * This class must be implemented by every transaction class
 * which want to connect to the state machine.
 */
class Transaction {
public:
	static void startTransaction(SP(Transaction) trans);
public:

	struct Params{
		Params(){
			this->needWriteviewLock = false;
			this->writeviewLock = NULL;
		}
		~Params(){
			if(writeviewLock != NULL){
				delete writeviewLock;
			}
		}
		Params(bool shouldLock){
			this->shouldLock = shouldLock;
			this->needWriteviewLock = false;
			this->writeviewLock = NULL;
		}
		Params(SP(Transaction) sp){
			this->sp = sp;
			this->needWriteviewLock = false;
			this->writeviewLock = NULL;
		}
		Params(bool shouldLock, SP(Transaction) sp){
			this->shouldLock = shouldLock;
			this->sp = sp;
			this->needWriteviewLock = false;
			this->writeviewLock = NULL;
		}

		void clear(){
			sp.reset();
			// mutex is transfered by copy
		}

		bool shouldLock; // if it's passed to finalize, this boolean is interpreted as finalizeResult
		SP(Transaction) sp;
		bool needWriteviewLock;
		boost::unique_lock<boost::shared_mutex> * writeviewLock;

	};

	Transaction();
	virtual ~Transaction();
	virtual void finalizeWork(Transaction::Params * arg){} ;
	virtual bool isReadviewTransaction()= 0 ;
	void finalize();
	void setFinalizeArgument(bool arg , bool needWriteviewLock = false);
	virtual void threadBegin(SP(Transaction) sp){ // sets sharedPointer to sp
		this->transMutex.lock();
		this->sharedPointer = sp;
	}
	virtual void threadEnd(){ // resets sharedPointer
		this->transMutex.unlock();
		this->sharedPointer.reset();
	}
	TRANS_ID getTID() const ;

	virtual ShardingTransactionType getTransactionType() = 0 ;
	virtual void run() = 0;

	virtual void initSession();
	TransactionSession * getSession();

	SP(ClusterNodes_Writeview) getNodesWriteview_write() const;
	SP(const ClusterNodes_Writeview) getNodesWriteview_read() const;
private:
	const TRANS_ID transactionId;
	TransactionSession * session;
	bool attachedToThreadFlag; // tells us onThreadAttach is called last or onThreadDetach

protected:
	Params * finalizeArgument;
	void setSession(TransactionSession * session);
	SP(Transaction) sharedPointer;
	boost::mutex transMutex ;
};


class ReadviewTransaction : public Transaction{
public:

	// nodesWriteview must come in locked state
	ReadviewTransaction(SP(const ClusterResourceMetadata_Readview) clusterReadview);

	bool isReadviewTransaction(){
		return true;
	}
	SP(const ClusterResourceMetadata_Readview) getReadview() const;
private:
	SP(const ClusterResourceMetadata_Readview) clusterReadview;
};

class WriteviewTransaction : public Transaction{
public:
	WriteviewTransaction();
	bool isReadviewTransaction(){
		return false;
	}
	Cluster_Writeview * getWriteview();
	void threadBegin(SP(Transaction) sp);
	void threadEnd();
private:
	Cluster_Writeview * writeview;
	boost::unique_lock<boost::shared_mutex> * writeviewLock;
};

}
}

#endif // __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__
