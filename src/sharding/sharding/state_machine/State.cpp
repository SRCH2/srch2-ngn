

#include "State.h"
#include "../transactions/Transaction.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


unsigned OperationState::nextOperationId = 10;
const unsigned OperationState::DataRecoveryOperationId = 1;

unsigned OperationState::getOperationId() const{
	return operationId;
}
void OperationState::setOperationId(unsigned operationId) {
	this->operationId = operationId;
}

void OperationState::send(SP(ShardingNotification) notification, const NodeOperationId & dest) const{
	if(! notification){
		ASSERT(false);
		return ;
	}
	notification->setDest(dest);
	notification->setSrc(NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()));
	ShardingNotification::send(notification);
}

SP(Transaction) OperationState::getTransaction(){
	return transaction;
}
void OperationState::setTransaction(SP(Transaction) sp){
	this->transaction = sp;
}
void OperationState::lock(){
	this->operationContentLock.lock();
}
void OperationState::unlock(){
	this->operationContentLock.unlock();
}

unsigned OperationState::getNextOperationId(){
	if(nextOperationId + 1 == (unsigned) -1){
		nextOperationId = 0;
		return nextOperationId;
	}
	return nextOperationId++;
}

}
}
