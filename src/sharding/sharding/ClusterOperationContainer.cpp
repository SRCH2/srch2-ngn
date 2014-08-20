#include "ClusterOperationContainer.h"

#include "core/util/Assert.h"
#include "sharding/util/FramedPrinter.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

void ClusterOperationStateMachine::registerOperation(OperationState * operation){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	if(! addActiveOperation(operation)){
		return;
	}
	startOperation(operation);
}

void ClusterOperationStateMachine::handle(ShardingNotification * notification){
	if(notification == NULL){
		ASSERT(false);
		return;
	}
	// find the operation in the map
	if(activeOperations.find(notification->getSrc().operationId) == activeOperations.end()){
		return; // no operation is waiting for this notification.
	}
	OperationState * nextState =
			activeOperations.find(notification->getSrc().operationId)->second->handle(notification);
	stateTransit(activeOperations.find(notification->getSrc().operationId)->second, nextState);
}

// goes to everybody
void ClusterOperationStateMachine::handle(Notification * notification){
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

void ClusterOperationStateMachine::print() const{
	if(activeOperations.size() == 0){
		cout << "**************************************" << endl;
		cout << "No active operation." << endl;
		cout << "**************************************" << endl;
		return;
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

bool ClusterOperationStateMachine::addActiveOperation(OperationState * operation){
	ASSERT(operation != NULL);
	if(activeOperations.find(operation->getOperationId()) != activeOperations.end()){
		ASSERT(false);
		return false; // operation id is not accepted because it already exists.
	}
	activeOperations[operation->getOperationId()] = operation;
	return true;
}

void ClusterOperationStateMachine::startOperation(OperationState * operation){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	OperationState * nextState = operation->entry();
	stateTransit(operation, nextState);
}

void ClusterOperationStateMachine::stateTransit(OperationState * operation, OperationState * nextState){
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
		// 2. delete the old operation
		delete operation;
		if(nextState == NULL){
			// 3. that's it.
			return;
		}
		if(addActiveOperation(nextState)){
			startOperation(nextState);
		}
	}
}

}
}
