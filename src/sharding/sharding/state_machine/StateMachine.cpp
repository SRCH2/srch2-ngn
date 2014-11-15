#include "StateMachine.h"
#include "../transactions/Transaction.h"
#include "../../util/FramedPrinter.h"

#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


StateMachine::StateMachine(){
	for(unsigned i = 0 ; i < ACTIVE_OPERATINS_GROUP_COUNT; ++i){
		activeOpertationGroups.push_back(std::make_pair(new boost::recursive_mutex(), map<unsigned, OperationState *>()));
	}
}

StateMachine::~StateMachine(){
	for(unsigned i = 0 ; i < ACTIVE_OPERATINS_GROUP_COUNT; ++i){
		activeOpertationGroups.at(i).first->lock();
		map<unsigned, OperationState *> & activeOperations = activeOpertationGroups.at(i).second;
		for(map<unsigned, OperationState *>::iterator opItr = activeOperations.begin();
				opItr != activeOperations.end(); ++opItr){
			delete opItr->second;
		}
		activeOperations.clear();
		activeOpertationGroups.at(i).first->unlock();
		delete activeOpertationGroups.at(i).first;
	}
	activeOpertationGroups.clear();
}

void StateMachine::registerOperation(OperationState * operation){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	lockOperationGroup(operation->getOperationId());
	if(! addActiveOperation(operation)){
		unlockOperationGroup(operation->getOperationId());
		return;
	}
	startOperation(operation);
	unlockOperationGroup(operation->getOperationId());
}

void StateMachine::handle(SP(ShardingNotification) notification){
	if(! notification){
		ASSERT(false);
		return;
	}
	// lock the operation group
	lockOperationGroup(notification->getDest().operationId);
	map<unsigned, OperationState *> & activeOperations = getOperationGroup(notification->getDest().operationId);
	// find the operation in the map
	if(activeOperations.find(notification->getDest().operationId) == activeOperations.end()){
		// notification target is not there
		unlockOperationGroup(notification->getDest().operationId);
		return;
	}
	OperationState * targetOperation = activeOperations[notification->getDest().operationId];
	OperationState * nextState = targetOperation->handle(notification);
	stateTransit(targetOperation, nextState);

	unlockOperationGroup(notification->getDest().operationId);

}

// goes to everybody
void StateMachine::handle(SP(Notification) notification){
	if(! notification){
		ASSERT(false);
		return;
	}
	//TODO : how can I make sure it doesn't lock it twice ?
	///////////////////////////////////////////////////////////////////
	for(unsigned groupId = 0; groupId < activeOpertationGroups.size(); ++groupId){
		// lock the operation group
		lockOperationGroup(groupId);
		map<unsigned, OperationState *> & activeOperations = getOperationGroup(groupId);
		map<unsigned, OperationState *> activeOperationsBackup = activeOperations;
		for(map<unsigned, OperationState *>::iterator activeOpItr = activeOperationsBackup.begin();
				activeOpItr != activeOperationsBackup.end(); ++activeOpItr){
			OperationState * nextState =
					activeOpItr->second->handle(notification);
			stateTransit(activeOpItr->second, nextState);
		}
		unlockOperationGroup(groupId);
	}

}


/*
 * Before calling print, lockStateMachine() must be invoked.
 */
void StateMachine::print() const{
	bool isEmpty = true;
	for(unsigned groupId = 0; groupId < activeOpertationGroups.size(); ++groupId){
		const map<unsigned, OperationState *> & activeOperations = activeOpertationGroups.at(groupId).second;
		if(activeOperations.size() > 0){
			isEmpty = false;
			break;
		}
	}

	if(! isEmpty){
		cout << "**************************************************************************************************" << endl;
		cout << "State machine : " << endl;
		cout << "**************************************************************************************************" << endl;
	}else{
		return;
	}
	vector<string> operationHeaders;
	operationHeaders.push_back("Name");
	operationHeaders.push_back("Status");
	vector<string> operationLabels;

	for(unsigned groupId = 0; groupId < activeOpertationGroups.size(); ++groupId){
		const map<unsigned, OperationState *> & activeOperations = activeOpertationGroups.at(groupId).second;

		for(map<unsigned, OperationState *>::const_iterator opItr = activeOperations.begin();
				opItr != activeOperations.end(); ++opItr){
			stringstream ss;
			ss << opItr->first;
			ASSERT(opItr->first == opItr->second->getOperationId());
			operationLabels.push_back(ss.str());
		}
	}

	srch2::util::TableFormatPrinter operationTable("Active Operations", 120, operationHeaders, operationLabels);
	operationTable.printColumnHeaders();
	operationTable.startFilling();

	for(unsigned groupId = 0; groupId < activeOpertationGroups.size(); ++groupId){
		const map<unsigned, OperationState *> & activeOperations = activeOpertationGroups.at(groupId).second;
		for(map<unsigned, OperationState *>::const_iterator opItr = activeOperations.begin();
				opItr != activeOperations.end(); ++opItr){
			operationTable.printNextCell(opItr->second->getOperationName());
			operationTable.printNextCell(opItr->second->getOperationStatus());
		}
	}

}

bool StateMachine::addActiveOperation(OperationState * operation){
	ASSERT(operation != NULL);

	map<unsigned, OperationState *> & activeOperations =
			getOperationGroup(operation->getOperationId());
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
	// NOTE : we do not call transaction->preProcess() before entry
	OperationState * nextState = operation->entry();
	stateTransit(operation, nextState);
}

void StateMachine::stateTransit(OperationState * operation,
		OperationState * nextState){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	if(nextState != NULL && nextState != operation){
		ASSERT(false);
		return;
	}
	if(nextState == operation){
		return;
	}
	// nextState == NULL
	ASSERT(nextState == NULL);
	// 3. delete the old operation
	map<unsigned, OperationState *> & activeOperations = getOperationGroup(operation->getOperationId());
	if(activeOperations.find(operation->getOperationId()) !=
			activeOperations.end()){
		activeOperations.erase(operation->getOperationId());
		delete operation;
	}
	return ;
}

void StateMachine::lockStateMachine(){
	for(unsigned gidx = 0; gidx < ACTIVE_OPERATINS_GROUP_COUNT; ++gidx){
		activeOpertationGroups.at(gidx).first->lock();
	}
}

void StateMachine::unlockStateMachine(){
	for(unsigned gidx = 0; gidx < ACTIVE_OPERATINS_GROUP_COUNT; ++gidx){
		activeOpertationGroups.at(gidx).first->unlock();
	}
}

void StateMachine::lockOperationGroup(unsigned opid){
	unsigned groupId = opid % ACTIVE_OPERATINS_GROUP_COUNT;
	activeOpertationGroups.at(groupId).first->lock();
}
void StateMachine::unlockOperationGroup(unsigned opid){
	unsigned groupId = opid % ACTIVE_OPERATINS_GROUP_COUNT;
	activeOpertationGroups.at(groupId).first->unlock();
}

map<unsigned, OperationState *> & StateMachine::getOperationGroup(unsigned opid){
	unsigned groupId = opid % ACTIVE_OPERATINS_GROUP_COUNT;
	return activeOpertationGroups.at(groupId).second;
}

}
}
