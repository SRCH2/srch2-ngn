

#include "State.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

unsigned OperationState::nextOperationId = 10;

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
	return nextOperationId++;
}

}
}
