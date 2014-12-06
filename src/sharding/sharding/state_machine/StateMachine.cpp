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
	OperationState::initOperationStateStaticEnv();
}

StateMachine::~StateMachine(){
	for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){
		ActiveOperationGroup & opGroup = getOperationGroup(groupId);
		opGroup.clear();
	}
}

void StateMachine::registerOperation(OperationState * operation){
	if(operation == NULL){
		ASSERT(false);
		return;
	}
	ActiveOperationGroup & opGroup = getOperationGroup(operation->getOperationId());
	if(! opGroup.addActiveOperation(operation)){
		return;
	}
	operation->lock();
	OperationState * nextState = operation->entry();
	operation->unlock();
	if(nextState == operation){
		return;
	}
	ASSERT(nextState == NULL);
	const string & targetOpName = operation->getOperationName();
	const unsigned opid = operation->getOperationId();
	if(! opGroup.deleteActiveOperation(operation->getOperationId())){
		Logger::sharding(Logger::Detail, "State Machine | Attempted to delete operation %s (id : %d) more than one time.",
				targetOpName.c_str(), opid);
	}
}

void StateMachine::handle(SP(ShardingNotification) notification){
	if(! notification){
		ASSERT(false);
		return;
	}
	ActiveOperationGroup & opGroup = getOperationGroup(notification->getDest().operationId);
	SP(OperationState) targetOperation = opGroup.getActiveOperation(notification->getDest().operationId);
	if(! targetOperation){
		return;
	}
	targetOperation->lock();
	OperationState * nextState = targetOperation->handle(notification);
	targetOperation->unlock();
	if(nextState == targetOperation.get()){
		return;
	}
	ASSERT(nextState == NULL);
	if(! opGroup.deleteActiveOperation(notification->getDest().operationId)){
		Logger::sharding(Logger::Detail, "State Machine | Attempted to delete operation %s (id : %d) more than one time.",
				targetOperation->getOperationName().c_str(), notification->getDest().operationId );
	}
}

// goes to everybody
void StateMachine::handle(SP(Notification) notification){
	if(! notification){
		ASSERT(false);
		return;
	}
	for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){

		ActiveOperationGroup & opGroup = getOperationGroup(groupId);
		map<unsigned , SP(OperationState)> activeOperations;
		opGroup.getAllActiveOperations(activeOperations);


		for(map<unsigned , SP(OperationState)>::iterator activeOpItr = activeOperations.begin();
				activeOpItr != activeOperations.end(); ++activeOpItr){
			OperationState * targetOperation = activeOpItr->second.get();
			targetOperation->lock();
			OperationState * nextState =
					targetOperation->handle(notification);
			targetOperation->unlock();
			if(nextState == targetOperation){
				return;
			}
			ASSERT(nextState == NULL);
			if(! opGroup.deleteActiveOperation(targetOperation->getOperationId())){
				Logger::sharding(Logger::Detail, "State Machine | Attempted to delete operation %s (id : %d) more than one time.",
						targetOperation->getOperationName().c_str(), targetOperation->getOperationId() );
			}
		}
	}
}


/*
 * Before calling print, lockStateMachine() must be invoked.
 */
void StateMachine::print(JsonResponseHandler * response) const{
	bool isEmpty = true;
	for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){
		const ActiveOperationGroup & activeOperations = activeOperationGroups[groupId];
		if(activeOperations.activeOperations.size() > 0){
			isEmpty = false;
			break;
		}
	}

	if(response != NULL){
		if(isEmpty){
			return;
		}
		Json::Value activeOperationsJson(Json::arrayValue);
		unsigned i = 0;
		for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){
			const ActiveOperationGroup & activeOperationsGroup = activeOperationGroups[groupId];

			const map<unsigned , SP(OperationState)> & activeOperations = activeOperationsGroup.activeOperations;
			for(map<unsigned, SP(OperationState)>::const_iterator opItr = activeOperations.begin();
					opItr != activeOperations.end(); ++opItr){
				ASSERT(opItr->first == opItr->second->getOperationId());
				activeOperationsJson[i]["id"] = opItr->second->getOperationName();
				activeOperationsJson[i]["name"] = opItr->second->getOperationName();
				activeOperationsJson[i]["state"] = opItr->second->getOperationStatus();

				i ++;
			}
		}
		response->setResponseAttribute("active-operations", activeOperationsJson);
		return;
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

	for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){
		const ActiveOperationGroup & activeOperationsGroup = activeOperationGroups[groupId];

		const map<unsigned , SP(OperationState)> & activeOperations = activeOperationsGroup.activeOperations;
		for(map<unsigned, SP(OperationState)>::const_iterator opItr = activeOperations.begin();
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

	for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){
		const ActiveOperationGroup & activeOperationsGroup = activeOperationGroups[groupId];

		const map<unsigned , SP(OperationState)> & activeOperations = activeOperationsGroup.activeOperations;
		for(map<unsigned, SP(OperationState)>::const_iterator opItr = activeOperations.begin();
				opItr != activeOperations.end(); ++opItr){
			operationTable.printNextCell(opItr->second->getOperationName());
			operationTable.printNextCell(opItr->second->getOperationStatus());
		}
	}

}


bool StateMachine::lockStateMachine(){
	for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){
		ActiveOperationGroup & activeOperationsGroup = activeOperationGroups[groupId];
		if(! activeOperationsGroup.contentMutex.try_lock()){
			for(unsigned releaseGroupId = 0; releaseGroupId < groupId; ++releaseGroupId){
				activeOperationGroups[releaseGroupId].contentMutex.unlock();
			}
			return false;
		}
	}
	return true;
}
void StateMachine::unlockStateMachine(){
	for(unsigned groupId = 0; groupId < ACTIVE_OPERATINS_GROUP_COUNT; ++groupId){
		ActiveOperationGroup & activeOperationsGroup = activeOperationGroups[groupId];
		activeOperationsGroup.contentMutex.unlock();
	}
}

StateMachine::ActiveOperationGroup & StateMachine::getOperationGroup(unsigned opid){
	unsigned groupId = opid % ACTIVE_OPERATINS_GROUP_COUNT;
	return activeOperationGroups[groupId];
}

bool StateMachine::ActiveOperationGroup::addActiveOperation(OperationState * operation){
	ASSERT(operation != NULL);
	contentMutex.lock();
	if(activeOperations.find(operation->getOperationId()) != activeOperations.end()){
		ASSERT(false);
		contentMutex.unlock();
		return false; // operation id is not accepted because it already exists.
	}
	activeOperations[operation->getOperationId()] = SP(OperationState)(operation);
	contentMutex.unlock();
	return true;
}


bool StateMachine::ActiveOperationGroup::deleteActiveOperation(const unsigned operationId){
	// delete the old operation
	contentMutex.lock();

	if(activeOperations.find(operationId) != activeOperations.end()){
		SP(OperationState) operation = activeOperations.at(operationId);
		activeOperations.erase(operationId);
		contentMutex.unlock();
		return true;
	}

	contentMutex.unlock();
	return false;
}


SP(OperationState) StateMachine::ActiveOperationGroup::getActiveOperation(const unsigned operationId){
	SP(OperationState) operation;
	operation.reset();
	contentMutex.lock();
	if(activeOperations.find(operationId) != activeOperations.end()){
		operation = activeOperations.at(operationId);
	}
	contentMutex.unlock();
	return operation;
}

void StateMachine::ActiveOperationGroup::getAllActiveOperations(map<unsigned , SP(OperationState)> & activeOperations){
	contentMutex.lock();
	activeOperations = this->activeOperations;
	contentMutex.unlock();
}

unsigned StateMachine::ActiveOperationGroup::size(){
	unsigned numberActiveOperations = 0;
	contentMutex.lock();
	numberActiveOperations = activeOperations.size();
	contentMutex.unlock();
	return numberActiveOperations;
}

void StateMachine::ActiveOperationGroup::clear(){
	contentMutex.lock();
	activeOperations.clear();
	contentMutex.unlock();
}





}
}
