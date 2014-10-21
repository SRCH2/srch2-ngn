

#include "State.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

BottomUpDeleteInterface::BottomUpDeleteInterface(){
	tidToDeletePolicy = 0;
	transId = TRANS_ID_NULL;
	parent = NULL;
}
void BottomUpDeleteInterface::setTransIdToDelete(TRANS_ID id){
	transId = id;
	tidToDeletePolicy = 1;
}
void BottomUpDeleteInterface::connectDeletePathToParent(BottomUpDeleteInterface * parent){
	if(parent == NULL){
		ASSERT(false);
		return;
	}
	tidToDeletePolicy = 2;
	this->parent = parent;
}
TRANS_ID BottomUpDeleteInterface::getTransIdToDelete(){
	switch (tidToDeletePolicy) {
		case 1:
			return transId;
		case 2:
			return this->parent->getTransIdToDelete();
		case 0:
		default:
			return TRANS_ID_NULL;
	}
}

TopDownDeleteInterface::TopDownDeleteInterface(){
	this->attachedToOtherThread = true;
}
bool TopDownDeleteInterface::isAttachedToOtherThread(){
	return attachedToOtherThread;
}
void TopDownDeleteInterface::setAttachedToOtherThread(const bool attached = true){
	this->attachedToOtherThread = attached;
}

unsigned OperationState::nextOperationId = 10;
const unsigned OperationState::DataRecoveryOperationId = 1;

unsigned OperationState::getOperationId() const{
	return operationId;
}
void OperationState::setOperationId(unsigned operationId) {
	this->operationId = operationId;
}

void OperationState::send(ShardingNotification * notification, const NodeOperationId & dest) const{
	if(notification == NULL){
		ASSERT(false);
		return ;
	}
	notification->setDest(dest);
	notification->setSrc(NodeOperationId(ShardManager::getCurrentNodeId(), this->getOperationId()));
	ShardManager::getShardManager()->send(notification);
}

// what's returned doesn't need to be started but it might be NULL
OperationState * OperationState::startOperation(OperationState * op){
	if(op == NULL){
		return NULL;
	}
	OperationState * nextState = op->entry();
	if(nextState == op){
		return op;
	}else{
		delete op;
		return startOperation(nextState);
	}
}

void OperationState::stateTransit(OperationState * & currentState, Notification * notification){
	OperationState * nextState = currentState->handle(notification);
	if(nextState == currentState){
		return;
	}
	delete currentState;
	currentState = startOperation(nextState);
}
unsigned OperationState::getNextOperationId(){
	if(nextOperationId + 1 == (unsigned) -1){
		nextOperationId = 0;
		return nextOperationId;
	}
	return nextOperationId++;
}

void Transaction::startTransaction(Transaction * trans){
	if(trans == NULL){
		ASSERT(false);
		return;
	}
	ShardManager::getStateMachine()->registerTransaction(trans);
	trans->run();
	if(! trans->isAttachedToOtherThread()){
		ShardManager::getStateMachine()->removeTransaction(trans->getTID());
	}
}

TRANS_ID Transaction::getTID() const {
	return this->transactionId;
}

static void Transaction::deallocateTransaction(Transaction * t){
	if(t == NULL){
		return;
	}
	switch (t->getTransactionType()) {
		case ShardingTransactionType_Loadbalancing:
			//TODO // delete ...
			break;
		default:
			break;
	}
}

}
}
