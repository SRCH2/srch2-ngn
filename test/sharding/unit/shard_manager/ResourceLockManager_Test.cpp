#include "core/util/Assert.h"

#include <iostream>
#include <sstream>
#include <time.h>
#include <string>
#include <stdlib.h>

#include "sharding/sharding/metadata_manager/ResourceLocks.h"
#include "LockManagerTestHelper.h"

using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;


bool testLockingOperations_validate(LockHoldersRepository * resultRepo, LockHoldersRepository * trueResultRepo){
	if(resultRepo == NULL || trueResultRepo == NULL){
		return (resultRepo == trueResultRepo);
	}

	return (*resultRepo == *trueResultRepo);
}

void testCopyConstructor(){

	ResourceLockManager lockManager;
	LockHoldersRepository trueResultRepo;

	for(unsigned i = 0 ; i < (unsigned)LockReposTestCode_EOF ; ++i){

		initializeLockHoldersRepository((LockReposTestCode)i, &trueResultRepo);
		// copy constructor
		initializeLockHoldersRepository((LockReposTestCode)i, lockManager.getLockHolderRepository());

		ASSERT(testLockingOperations_validate(lockManager.getLockHolderRepository(), &trueResultRepo));

		lockManager.getLockHolderRepository()->clear();
		trueResultRepo.clear();
	}

}

void testLockOperation_S_operate(const LockReposTestCode caseCode, ResourceLockManager * lockManager){
	if(lockManager == NULL){
		ASSERT(false);
		return;
	}

	vector<O> lockHolders;
	switch (caseCode) {
	case LockReposTestCode_S_Lock_Grant_0:
	{
		// goal : acquire s lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,1));
		SingleResourceLockRequest lockReq(R(0,0,1) , lockHolders , ResourceLockType_S);
		ASSERT(lockManager->canAcquireLock(R(0,0,1), ResourceLockType_S));
		lockManager->executeRequest(lockReq);
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,1)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,1).getPartitionId()));
		return;
	}
	case LockReposTestCode_S_Lock_Grant_1:
	{
		// goal : acquire s lock on (0,0,0) for <(0,2),(0,1)>
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,1));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_S);
		ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_S));
		lockManager->executeRequest(lockReq);
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_S_Lock_Grant_2:
	{
		// goal : acquire s lock on (0,0,0) for <(2,1),(2,2)>
		lockHolders.push_back(O(2,1));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_S);
		ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_S));
		lockManager->executeRequest(lockReq);
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_S_Lock_Grant_3:
	{
		// goal : acquire s lock on (0,0,0) for <(2,2),(2,3)>
		lockHolders.push_back(O(2,2));
		lockHolders.push_back(O(2,3));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_S);
		ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_S));
		lockManager->executeRequest(lockReq);
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_S_Lock_Reject_0:
	{
		// goal : reject s lock on (0,1,10) for <(1,1),(2,2)>
		lockHolders.push_back(O(1,1));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_S);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_S));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_S_Lock_Reject_1:
	{
		// goal : reject s lock on (0,0,0) for <(2,2),(2,3)>
		lockHolders.push_back(O(2,2));
		lockHolders.push_back(O(2,3));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_S);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_S));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_S_Lock_Reject_2:
	{
		// goal : reject s lock on (0,1,10) for <(0,2),(0,4)>
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,4));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_S);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_S));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_S_Lock_Reject_3:
	{
		// goal : reject s lock on (0,1,10) for <(1,2),(2,2)>
		lockHolders.push_back(O(1,2));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_S);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_S));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	default:
		ASSERT(false);
		return;
	}

}


void testLockOperation_U_operate(const LockReposTestCode caseCode, ResourceLockManager * lockManager){
	if(lockManager == NULL){
		ASSERT(false);
		return;
	}

	vector<O> lockHolders;
	switch (caseCode) {
	case LockReposTestCode_U_Lock_Grant_0:
	{
		// goal : acquire u lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,1));
		SingleResourceLockRequest lockReq(R(0,0,1) , lockHolders , ResourceLockType_U);
		ASSERT(lockManager->canAcquireLock(R(0,0,1), ResourceLockType_U));
		lockManager->executeRequest(lockReq);
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,1)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,1).getPartitionId()));
		return;
	}
	case LockReposTestCode_U_Lock_Grant_1:
	{
		// goal : acquire u lock on (0,0,0) for <(0,2),(0,1)>
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,1));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_U);
		ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_U));
		lockManager->executeRequest(lockReq);
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_U_Lock_Reject_0:
	{
		// goal : reject u lock on (0,1,10) for <(1,1),(2,2)>
		lockHolders.push_back(O(1,1));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_U);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_U));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_U_Lock_Reject_1:
	{
		// goal : reject u lock on (0,0,0) for <(2,2),(2,3)>
		lockHolders.push_back(O(2,2));
		lockHolders.push_back(O(2,3));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_U);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_U));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_U_Lock_Reject_2:
	{
		// goal : reject u lock on (0,1,10) for <(0,2),(0,4)>
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,4));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_U);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_U));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_U_Lock_Reject_3:
	{
		// goal : reject u lock on (0,1,10) for <(1,2),(2,2)>
		lockHolders.push_back(O(1,2));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_U);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_U));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_U_Lock_Reject_4:
	{
		// goal : acquire u lock on (0,0,0) for <(2,1),(2,2)>
		lockHolders.push_back(O(2,1));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_U);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_U));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_U_Lock_Reject_5:
	{
		// goal : acquire u lock on (0,0,0) for <(2,2),(2,3)>
		lockHolders.push_back(O(2,2));
		lockHolders.push_back(O(2,3));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_U);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_U));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	default:
		ASSERT(false);
		return;
	}

}


void testLockOperation_X_operate(const LockReposTestCode caseCode, ResourceLockManager * lockManager){
	if(lockManager == NULL){
		ASSERT(false);
		return;
	}

	vector<O> lockHolders;
	switch (caseCode) {
	case LockReposTestCode_X_Lock_Grant_0:
	{
		// goal : acquire x lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,1));
		SingleResourceLockRequest lockReq(R(0,0,1) , lockHolders , ResourceLockType_X);
		ASSERT(lockManager->canAcquireLock(R(0,0,1), ResourceLockType_X));
		lockManager->executeRequest(lockReq);
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,1)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,1).getPartitionId()));
		return;
	}
	case LockReposTestCode_X_Lock_Reject_0:
	{
		// goal : reject u lock on (0,1,10) for <(1,1),(2,2)>
		lockHolders.push_back(O(1,1));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_X);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_X));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_X_Lock_Reject_1:
	{
		// goal : reject u lock on (0,0,0) for <(2,2),(2,3)>
		lockHolders.push_back(O(2,2));
		lockHolders.push_back(O(2,3));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_X);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_X_Lock_Reject_2:
	{
		// goal : reject u lock on (0,1,10) for <(0,2),(0,4)>
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,4));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_X);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_X));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_X_Lock_Reject_3:
	{
		// goal : reject u lock on (0,1,10) for <(1,2),(2,2)>
		lockHolders.push_back(O(1,2));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,1,10) , lockHolders , ResourceLockType_X);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,1,10), ResourceLockType_X));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,10)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,10).getPartitionId()));
		return;
	}
	case LockReposTestCode_X_Lock_Reject_4:
	{
		// goal : acquire u lock on (0,0,0) for <(2,1),(2,2)>
		lockHolders.push_back(O(2,1));
		lockHolders.push_back(O(2,2));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_X);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_X_Lock_Reject_5:
	{
		// goal : acquire u lock on (0,0,0) for <(2,2),(2,3)>
		lockHolders.push_back(O(2,2));
		lockHolders.push_back(O(2,3));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_X);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	case LockReposTestCode_X_Lock_Reject_6:
	{
		// goal : acquire u lock on (0,0,0) for <(0,2),(0,1)>
		lockHolders.push_back(O(0,2));
		lockHolders.push_back(O(0,1));
		SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_X);
		ASSERT(! lockManager->canGrantRequest(lockReq));
		ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
		ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
		ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
		return;
	}
	default:
		ASSERT(false);
		return;
	}

}


void testLockOperation_S(){
	ResourceLockManager lockManager;
	LockHoldersRepository trueResultRepo;

	for(unsigned i = (unsigned)LockReposTestCode_S_Lock_Grant_0 ; i < (unsigned)LockReposTestCode_EOF_S_Lock ; ++i){

		initializeLockHoldersRepository((LockReposTestCode)i, lockManager.getLockHolderRepository());
		testLockOperation_S_operate((LockReposTestCode)i, &lockManager);
		initializeLockHoldersRepositoryResults((LockReposTestCode)i, &trueResultRepo);

		if(! testLockingOperations_validate(lockManager.getLockHolderRepository(), &trueResultRepo)){
			cout << "Lock manager repository : "<< endl;
			lockManager.getLockHolderRepository()->print();
			cout << "=======================================" << endl;
			cout << "True result repository : "<< endl;
			trueResultRepo.print();
			cout << "=======================================" << endl;
			ASSERT(false);
		}

		lockManager.getLockHolderRepository()->clear();
		trueResultRepo.clear();
	}
}

void testLockOperation_U(){
	ResourceLockManager lockManager;
	LockHoldersRepository trueResultRepo;

	for(unsigned i = (unsigned)LockReposTestCode_U_Lock_Grant_0 ; i < (unsigned)LockReposTestCode_EOF_U_Lock ; ++i){

		initializeLockHoldersRepository((LockReposTestCode)i, lockManager.getLockHolderRepository());
		testLockOperation_U_operate((LockReposTestCode)i, &lockManager);
		initializeLockHoldersRepositoryResults((LockReposTestCode)i, &trueResultRepo);

		if(! testLockingOperations_validate(lockManager.getLockHolderRepository(), &trueResultRepo)){
			cout << "Lock manager repository : "<< endl;
			lockManager.getLockHolderRepository()->print();
			cout << "=======================================" << endl;
			cout << "True result repository : "<< endl;
			trueResultRepo.print();
			cout << "=======================================" << endl;
			ASSERT(false);
		}

		lockManager.getLockHolderRepository()->clear();
		trueResultRepo.clear();
	}
}

void testLockOperation_X(){
	ResourceLockManager lockManager;
	LockHoldersRepository trueResultRepo;

	for(unsigned i = (unsigned)LockReposTestCode_X_Lock_Grant_0 ; i < (unsigned)LockReposTestCode_EOF_X_Lock ; ++i){

		initializeLockHoldersRepository((LockReposTestCode)i, lockManager.getLockHolderRepository());
		testLockOperation_X_operate((LockReposTestCode)i, &lockManager);
		initializeLockHoldersRepositoryResults((LockReposTestCode)i, &trueResultRepo);

		if(! testLockingOperations_validate(lockManager.getLockHolderRepository(), &trueResultRepo)){
			cout << "Lock manager repository : "<< endl;
			lockManager.getLockHolderRepository()->print();
			cout << "=======================================" << endl;
			cout << "True result repository : "<< endl;
			trueResultRepo.print();
			cout << "=======================================" << endl;
			ASSERT(false);
		}

		lockManager.getLockHolderRepository()->clear();
		trueResultRepo.clear();
	}
}

void testLockOperation(){

	testLockOperation_S();

	testLockOperation_U();

	testLockOperation_X();
}

void testRelease_1(ResourceLockManager * lockManager){
	vector<O> lockHolders;
	// 1. lock
	lockHolders.push_back(O(2,2));
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_X);
	ASSERT(lockManager->canGrantRequest(lockReq));
	lockManager->executeRequest(lockReq);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	// 2. release
	SingleResourceLockRequest relReq(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	lockManager->executeRequest(relReq);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

}
void testRelease_2(ResourceLockManager * lockManager){
	vector<O> lockHolders;
	// 1. lock multiple resources
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_X);
	ASSERT(lockManager->canGrantRequest(lockReq));
	lockManager->executeRequest(lockReq);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq2(R(0,1,0) , lockHolders , ResourceLockType_X);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	lockManager->executeRequest(lockReq2);
	ASSERT(! lockManager->canGrantRequest(lockReq2));
	ASSERT(! lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,0).getPartitionId()));

	// 2. lock conflicting resource
	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq3(R(0,1,0) , lockHolders , ResourceLockType_U);
	ASSERT(! lockManager->canGrantRequest(lockReq3));
	ASSERT(! lockManager->canAcquireLock(R(0,1,0), ResourceLockType_U));

	// 3. release all resources
	lockHolders.clear();
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest relReq(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	lockManager->executeRequest(relReq);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	lockHolders.clear();
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest relReq2(R(0,1,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq2));
	lockManager->executeRequest(relReq2);
	ASSERT(! lockManager->canGrantRequest(lockReq2));
	ASSERT(! lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,0).getPartitionId()));

	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest relReq3(R(0,1,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq3));
	lockManager->executeRequest(relReq3);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,1,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,0).getPartitionId()));
}
void testRelease_3(ResourceLockManager * lockManager){
	vector<O> lockHolders;
	// 1. lock
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_S);
	ASSERT(lockManager->canGrantRequest(lockReq));
	lockManager->executeRequest(lockReq);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	// 2. lock
	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq2(R(0,0,0) , lockHolders , ResourceLockType_S);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	lockManager->executeRequest(lockReq2);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	// 3. release first
	lockHolders.clear();
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest relReq(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	lockManager->executeRequest(relReq);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	// 4. lock
	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq3(R(0,1,0) , lockHolders , ResourceLockType_X);
	ASSERT(lockManager->canGrantRequest(lockReq3));
	lockManager->executeRequest(lockReq3);
	ASSERT(! lockManager->canGrantRequest(lockReq3));
	ASSERT(! lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,0).getPartitionId()));
	// 5. release all

	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest relReq2(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq2));
	lockManager->executeRequest(relReq2);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));


}
void testRelease_4(ResourceLockManager * lockManager){
	vector<O> lockHolders;
	// 1. lock for two holders
	lockHolders.push_back(O(2,2));
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_S);
	ASSERT(lockManager->canGrantRequest(lockReq));
	lockManager->executeRequest(lockReq);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	// 2. release first holder
	lockHolders.clear();
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest relReq(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	lockManager->executeRequest(relReq);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	// 3. release second holder
	lockHolders.clear();
	lockHolders.push_back(O(2,2));
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest relReq2(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq2));
	lockManager->executeRequest(relReq2);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
}
void testRelease_5(ResourceLockManager * lockManager){
	vector<O> lockHolders;
	// 1. lock for two holders
	lockHolders.push_back(O(2,2));
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_U);
	ASSERT(lockManager->canGrantRequest(lockReq));
	lockManager->executeRequest(lockReq);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	// 2. release one holder
	lockHolders.clear();
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest relReq(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	lockManager->executeRequest(relReq);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	// 3. lock for two other holders
	lockHolders.clear();
	lockHolders.push_back(O(1,2));
	lockHolders.push_back(O(1,3));
	SingleResourceLockRequest lockReq2(R(0,0,0) , lockHolders , ResourceLockType_S);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	lockManager->executeRequest(lockReq2);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	// 4. release all holders
	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	lockHolders.push_back(O(1,3));
	lockHolders.push_back(O(1,2));
	SingleResourceLockRequest relReq2(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq2));
	lockManager->executeRequest(relReq2);
	lockManager->getLockHolderRepository()->print();
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
}
void testRelease_6(ResourceLockManager * lockManager){
	vector<O> lockHolders;
	// 1. lock for multiple holders
	lockHolders.push_back(O(2,2));
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_U);
	ASSERT(lockManager->canGrantRequest(lockReq));
	lockManager->executeRequest(lockReq);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	// 2. lock for multiple holders for some other resource
	lockHolders.push_back(O(1,2));
	lockHolders.push_back(O(1,3));
	SingleResourceLockRequest lockReq2(R(1,1,0) , lockHolders , ResourceLockType_U);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	lockManager->executeRequest(lockReq2);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canGrantRequest(lockReq2));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->canAcquireLock(R(1,1,0), ResourceLockType_X));
	ASSERT(lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(1,1,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(1,1,0).getPartitionId()));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(1,1,1)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(1,1,1).getPartitionId()));
	// 3. release 1 holder of the first resource
	lockHolders.clear();
	lockHolders.push_back(O(2,2));
	SingleResourceLockRequest relReq(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	lockManager->executeRequest(relReq);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	// 4. release all holders of the second resource
	lockHolders.clear();
	lockHolders.push_back(O(1,2));
	lockHolders.push_back(O(1,3));
	SingleResourceLockRequest relReq2(R(1,1,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq2));
	lockManager->executeRequest(relReq2);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	// 5. release all resources and all holders
	lockHolders.clear();
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest relReq3(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	ASSERT(lockManager->canGrantRequest(relReq3));
	lockManager->executeRequest(relReq3);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
}

void testRelease_7(ResourceLockManager * lockManager){
	vector<O> lockHolders;
	// 1. lock multiple resources of the same partition
	lockHolders.push_back(O(2,3));
	SingleResourceLockRequest lockReq(R(0,0,0) , lockHolders , ResourceLockType_U);
	ASSERT(lockManager->canGrantRequest(lockReq));
	lockManager->executeRequest(lockReq);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	SingleResourceLockRequest lockReq2(R(0,0,1) , lockHolders , ResourceLockType_U);
	ASSERT(lockManager->canGrantRequest(lockReq2));
	lockManager->executeRequest(lockReq2);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canGrantRequest(lockReq2));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->canAcquireLock(R(0,0,1), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,1)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,1).getPartitionId()));

	SingleResourceLockRequest lockReq3(R(0,1,0) , lockHolders , ResourceLockType_U);
	ASSERT(lockManager->canGrantRequest(lockReq3));
	lockManager->executeRequest(lockReq3);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canGrantRequest(lockReq2));
	ASSERT(! lockManager->canGrantRequest(lockReq3));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->canAcquireLock(R(0,0,1), ResourceLockType_X));
	ASSERT(! lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,1)));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,1,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,1).getPartitionId()));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,0).getPartitionId()));

	SingleResourceLockRequest lockReq4(R(0,1,1) , lockHolders , ResourceLockType_U);
	ASSERT(lockManager->canGrantRequest(lockReq4));
	lockManager->executeRequest(lockReq4);
	ASSERT(! lockManager->canGrantRequest(lockReq));
	ASSERT(! lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(! lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	// 2. release resources one by one and check partition lock value
	SingleResourceLockRequest relReq(R(0,0,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq));
	lockManager->executeRequest(relReq);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));

	SingleResourceLockRequest relReq2(R(0,0,1) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq2));
	lockManager->executeRequest(relReq2);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->canAcquireLock(R(0,0,1), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,0,1)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,1).getPartitionId()));

	SingleResourceLockRequest relReq3(R(0,1,0) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq3));
	lockManager->executeRequest(relReq3);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(lockManager->canGrantRequest(lockReq3));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->canAcquireLock(R(0,0,1), ResourceLockType_X));
	ASSERT(lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,1,0)));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,0).getPartitionId()));
	ASSERT(lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,1).getPartitionId()));

	SingleResourceLockRequest relReq4(R(0,1,1) , lockHolders );
	ASSERT(lockManager->canGrantRequest(relReq4));
	lockManager->executeRequest(relReq4);
	ASSERT(lockManager->canGrantRequest(lockReq));
	ASSERT(lockManager->canGrantRequest(lockReq2));
	ASSERT(lockManager->canGrantRequest(lockReq3));
	ASSERT(lockManager->canGrantRequest(lockReq4));
	ASSERT(lockManager->canAcquireLock(R(0,0,0), ResourceLockType_X));
	ASSERT(lockManager->canAcquireLock(R(0,0,1), ResourceLockType_X));
	ASSERT(lockManager->canAcquireLock(R(0,1,0), ResourceLockType_X));
	ASSERT(lockManager->canAcquireLock(R(0,1,1), ResourceLockType_X));
	ASSERT(lockManager->getLockHolderRepository()->isFree(R(0,1,1)));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,0).getPartitionId()));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,0,1).getPartitionId()));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,0).getPartitionId()));
	ASSERT(! lockManager->getLockHolderRepository()->isPartitionLocked(R(0,1,1).getPartitionId()));

}

void testReleaseOperation(){
	ResourceLockManager lockManager;
	testRelease_1(&lockManager);
	lockManager.getLockHolderRepository()->clear();
	testRelease_2(&lockManager);
	lockManager.getLockHolderRepository()->clear();
	testRelease_3(&lockManager);
	lockManager.getLockHolderRepository()->clear();
	testRelease_4(&lockManager);
	lockManager.getLockHolderRepository()->clear();
	testRelease_5(&lockManager);
	lockManager.getLockHolderRepository()->clear();
	testRelease_6(&lockManager);
	lockManager.getLockHolderRepository()->clear();
	testRelease_7(&lockManager);

}

void testUpgradeOperation(){
	//TODO
}

void testDowngradeOperation(){
	//TODO
}




void testLockingOperations(){

	testCopyConstructor();

	testLockOperation();

	testReleaseOperation();

	testUpgradeOperation();

	testDowngradeOperation();

}

int testNotificationHandlers(){

}


/*
 * This test has two parts :
 * 1. Testing handle functions of lock manager.
 * 2. Testing locking operations on lock manager.
 */
int main(){

	testLockingOperations();

	testNotificationHandlers();


    cout << "Resource lock manager unit tests: Passed" << endl;
}
