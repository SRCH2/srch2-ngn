

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

void OperationState::stateTransit(OperationState * & currentState, SP(Notification) notification){
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

}
}
