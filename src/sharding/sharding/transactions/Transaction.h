/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
				this->writeviewLock = NULL;
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
	virtual void init();
	virtual ~Transaction();
	virtual void finalizeWork(Transaction::Params * arg){} ;
	virtual bool isReadviewTransaction()= 0 ;
	void finalize();
	void setFinalizeArgument(bool arg , bool needWriteviewLock = false);
	Transaction::Params * getFinalizeArgument();
	virtual void threadBegin(SP(Transaction) sp){ // sets sharedPointer to sp
		this->transMutex.lock();
		this->sharedPointer = sp;
	}
	virtual void threadEnd(){ // resets sharedPointer
		this->sharedPointer.reset();
	    if(this->finalizeArgument == NULL ||
	            (this->finalizeArgument != NULL && ! this->finalizeArgument->needWriteviewLock)){
            this->transMutex.unlock();
	    }
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
	void setWriteview(Cluster_Writeview * newWriteview);
	void threadBegin(SP(Transaction) sp);
	void threadEnd();
private:
	Cluster_Writeview * writeview;
	boost::unique_lock<boost::shared_mutex> * writeviewLock;
};

}
}

#endif // __SHARDING_SHARDING_TRNASACTION_TRANSACTION_H__
