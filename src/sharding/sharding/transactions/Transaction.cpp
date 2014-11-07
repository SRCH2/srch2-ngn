#include "Transaction.h"
#include "TransactionSession.h"
#include "../state_machine/State.h"

#include "core/util/Assert.h"
#include "core/util/Logger.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


Transaction::Transaction():transactionId(OperationState::getNextOperationId()){
	attachedFlag = true;
	finishedFlag = false;
	session = NULL;
}

Transaction::~Transaction(){
//	if(this->getTransactionType() != ShardingTransactionType_Loadbalancing){
//		__FUNC_LINE__
//		Logger::sharding(Logger::Step, "Deallocating transaction.");
//	}
	delete session;
}

void Transaction::startTransaction(Transaction * trans){
	if(trans == NULL){
		ASSERT(false);
		return;
	}
	if(trans->getTransactionType() != ShardingTransactionType_Loadbalancing){
		Logger::sharding(Logger::Step, "Starting transaction %s", getTransTypeStr(trans->getTransactionType()));
	}
	if(! trans->run() || ! trans->isAttached()){
		__FUNC_LINE__
	    Logger::sharding(Logger::Error, "New transaction could not be attached to another thread, so it was aborted.");
		delete trans;
	}
}

TRANS_ID Transaction::getTID() const {
	return this->transactionId;
}

void Transaction::setAttached(){
	this->attachedFlag = true;
}
void Transaction::setUnattached(){
		this->attachedFlag = false;
}
bool Transaction::isAttached() const{
	return this->attachedFlag;
}

bool Transaction::isFinished() const{
	return this->finishedFlag;
}
TransactionSession * Transaction::getSession(){
	return this->session;
}

SP(ClusterNodes_Writeview) Transaction::getNodesWriteview_write() const{
	SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getShardManager()->
			getMetadataManager()->getClusterNodesWriteview_write();
	return nodesWriteview;
}

SP(ClusterNodes_Writeview) Transaction::getNodesWriteview_read() const{
	SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getShardManager()->
			getMetadataManager()->getClusterNodesWriteview_read();
	return nodesWriteview;
}

void Transaction::setSession(TransactionSession * session){
	ASSERT(session != NULL);
	this->session = session;
}


ReadviewTransaction::ReadviewTransaction(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview):
		Transaction(){
	this->clusterReadview = clusterReadview;
}

boost::shared_ptr<const ClusterResourceMetadata_Readview> ReadviewTransaction::getReadview() const{
	return clusterReadview;
}

WriteviewTransaction::WriteviewTransaction():Transaction(){
	writeview = ShardManager::getWriteview_write(xLock);
	automaticLocking = true;
}
void WriteviewTransaction::preProcess(){
	xLock.lock();
}
void WriteviewTransaction::postProcess(){
	xLock.unlock();
}

Cluster_Writeview * WriteviewTransaction::getWriteview(){
	return writeview;
}

}
}
