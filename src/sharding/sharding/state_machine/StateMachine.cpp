#include "StateMachine.h"

#include "../../util/FramedPrinter.h"

#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

bool StateMachine::registerTransaction(Transaction * transaction){
	if(transaction == NULL){
		ASSERT(false);
		return false;
	}
	if(activeTransactions.find(transaction->getTID()) != activeTransactions.end()){
		return false;
	}
	activeTransactions[transaction->getTID()] = transaction;
	return true;
}

void StateMachine::registerOperation(OperationState * operation){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	if(! addActiveOperation(operation)){
		return;
	}
	startOperation(operation);
}

void StateMachine::removeTransaction(TRANS_ID tid){
	if(tid == TRANS_ID_NULL){
		return;
	}
	if(activeTransactions.find(tid) == activeTransactions.end()){
		return;
	}
	Transaction::deallocateTransaction(activeTransactions.find(tid)->second);
	activeTransactions.erase(tid);
}

void StateMachine::handle(ShardingNotification * notification){
	if(notification == NULL){
		ASSERT(false);
		return;
	}
	// find the operation in the map
	if(activeOperations.find(notification->getDest().operationId) == activeOperations.end()){
		return; // no operation is waiting for this notification.
	}
	OperationState * nextState =
			activeOperations.find(notification->getDest().operationId)->second->handle(notification);
	stateTransit(activeOperations.find(notification->getDest().operationId)->second, nextState);
}

// goes to everybody
void StateMachine::handle(Notification * notification){
	if(notification == NULL){
		ASSERT(false);
		return;
	}
	map<unsigned, OperationState *> activeOperationsBackup = activeOperations;
	for(map<unsigned, OperationState *>::iterator activeOpItr = activeOperationsBackup.begin();
			activeOpItr != activeOperationsBackup.end(); ++activeOpItr){
		OperationState * nextState =
				activeOpItr->second->handle(notification);
		stateTransit(activeOpItr->second, nextState);
	}
}

void StateMachine::print() const{
	if(activeOperations.size() > 0){
		cout << "**************************************************************************************************" << endl;
		cout << "State machine : " << endl;
		cout << "**************************************************************************************************" << endl;
	}
	vector<string> operationHeaders;
	operationHeaders.push_back("Name");
	operationHeaders.push_back("Status");
	vector<string> operationLabels;
	for(map<unsigned, OperationState *>::const_iterator opItr = activeOperations.begin(); opItr != activeOperations.end(); ++opItr){
		stringstream ss;
		ss << opItr->first;
		ASSERT(opItr->first == opItr->second->getOperationId());
		operationLabels.push_back(ss.str());
	}
	srch2::util::TableFormatPrinter operationTable("Active Operations", 120, operationHeaders, operationLabels);
	operationTable.printColumnHeaders();
	operationTable.startFilling();
	for(map<unsigned, OperationState *>::const_iterator opItr = activeOperations.begin(); opItr != activeOperations.end(); ++opItr){
		operationTable.printNextCell(opItr->second->getOperationName());
		operationTable.printNextCell(opItr->second->getOperationStatus());
	}
}

bool StateMachine::addActiveOperation(OperationState * operation){
	ASSERT(operation != NULL);
	if(activeOperations.find(operation->getOperationId()) != activeOperations.end()){
		ASSERT(false);
		return false; // operation id is not accepted because it already exists.
	}
	activeOperations[operation->getOperationId()] = operation;

	return true;
}

void StateMachine::startOperation(OperationState * operation){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	OperationState * nextState = operation->entry();
	stateTransit(operation, nextState);

}

void StateMachine::stateTransit(OperationState * operation, OperationState * nextState){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	if(nextState == NULL || nextState != operation){
		// 1. first find it in the map and remove it.
		if(activeOperations.find(operation->getOperationId()) !=
				activeOperations.end()){
			activeOperations.erase(operation->getOperationId());
		}
		if(nextState == NULL){
			// 2. that's it.
			// now, this is where we check this operation to see if there
			// is any ending transaction to delete
			// 3. delete the old operation
			removeTransaction(operation->getTransIdToDelete());
			delete operation;
			return;
		}
		// 3. delete the old operation
		delete operation;
		if(addActiveOperation(nextState)){
			startOperation(nextState);
		}
	}
}
}
}
