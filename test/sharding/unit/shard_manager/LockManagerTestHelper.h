#include "core/util/Assert.h"

#include <iostream>
#include <sstream>
#include <time.h>
#include <string>
#include <stdlib.h>

#include "LockRepoInitializer.h"

using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;


typedef ClusterShardId R;
typedef NodeOperationId O;

enum ResourceLockRequestCaseCode{
	ResourceLockRequestCaseCode_NewNodeLock0,
	ResourceLockRequestCaseCode_NewNodeLock1,
	ResourceLockRequestCaseCode_NewNodeRelease0,
	ResourceLockRequestCaseCode_NewNodeRelease1,
	ResourceLockRequestCaseCode_ShardCopyLock0,
	ResourceLockRequestCaseCode_ShardCopyRelease0,
	ResourceLockRequestCaseCode_ShardMoveLock0,
	ResourceLockRequestCaseCode_ShardMoveRelease0,
	ResourceLockRequestCaseCode_ShardMoveRelease1,
	ResourceLockRequestCaseCode_EOF
};


enum LockReposTestCode{
	LockReposTestCode_S_Lock_Grant_0,
	LockReposTestCode_S_Lock_Grant_1,
	LockReposTestCode_S_Lock_Grant_2,
	LockReposTestCode_S_Lock_Grant_3,
	LockReposTestCode_S_Lock_Reject_0,
	LockReposTestCode_S_Lock_Reject_1,
	LockReposTestCode_S_Lock_Reject_2,
	LockReposTestCode_S_Lock_Reject_3,
	LockReposTestCode_EOF_S_Lock,

	LockReposTestCode_U_Lock_Grant_0,
	LockReposTestCode_U_Lock_Grant_1,
	LockReposTestCode_U_Lock_Reject_0,
	LockReposTestCode_U_Lock_Reject_1,
	LockReposTestCode_U_Lock_Reject_2,
	LockReposTestCode_U_Lock_Reject_3,
	LockReposTestCode_U_Lock_Reject_4,
	LockReposTestCode_U_Lock_Reject_5,
	LockReposTestCode_EOF_U_Lock,

	LockReposTestCode_X_Lock_Grant_0,
	LockReposTestCode_X_Lock_Reject_0,
	LockReposTestCode_X_Lock_Reject_1,
	LockReposTestCode_X_Lock_Reject_2,
	LockReposTestCode_X_Lock_Reject_3,
	LockReposTestCode_X_Lock_Reject_4,
	LockReposTestCode_X_Lock_Reject_5,
	LockReposTestCode_X_Lock_Reject_6,
	LockReposTestCode_EOF_X_Lock,

	//TODO
	LockReposTestCode_EOF
};

//ResourceLockRequest * createResourceLockRequest(const ResourceLockRequestCaseCode caseCode){
//	switch (caseCode) {
//	case ResourceLockRequestCaseCode_NewNodeLock0:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(4,10));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		// all S locks
//		for(unsigned c = 0; c < 2; ++c){
//			for(unsigned p = 0; p < 4; ++p){
//				for(unsigned r = 0; r < 3; ++r){
//					SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(c,p,r) , lockHolders , ResourceLockType_S);
//					lockRequestBatch.push_back(lockReq);
//				}
//			}
//		}
//
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = true;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_NewNodeLock1:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(4,10));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		// all S locks, two of them X
//		for(unsigned c = 0; c < 2; ++c){
//			for(unsigned p = 0; p < 4; ++p){
//				for(unsigned r = 0; r < 3; ++r){
//					if((c == 0 && p == 1 && r == 0)
//							|| (c == 1 && p == 3 && r == 2)){
//						SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(c,p,r) , lockHolders , ResourceLockType_X);
//						lockRequestBatch.push_back(lockReq);
//					}else{
//						SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(c,p,r) , lockHolders , ResourceLockType_S);
//						lockRequestBatch.push_back(lockReq);
//					}
//				}
//			}
//		}
//
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = true;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_NewNodeRelease0:
//	case ResourceLockRequestCaseCode_NewNodeRelease1:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(4,10));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		// all S locks
//		for(unsigned c = 0; c < 2; ++c){
//			for(unsigned p = 0; p < 4; ++p){
//				for(unsigned r = 0; r < 3; ++r){
//					SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(c,p,r) , lockHolders);
//					lockRequestBatch.push_back(lockReq);
//				}
//			}
//		}
//
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = true;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_ShardCopyLock0:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(4,10));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(1,1,1) , lockHolders, ResourceLockType_S);
//		lockRequestBatch.push_back(lockReq);
//		SingleResourceLockRequest * lockReq2 = new SingleResourceLockRequest(R(1,1,3) , lockHolders, ResourceLockType_X);
//		lockRequestBatch.push_back(lockReq2);
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = false;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_ShardCopyRelease0:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(4,10));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(1,1,1) , lockHolders);
//		lockRequestBatch.push_back(lockReq);
//		SingleResourceLockRequest * lockReq2 = new SingleResourceLockRequest(R(1,1,3) , lockHolders);
//		lockRequestBatch.push_back(lockReq2);
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = true;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_ShardMoveLock0:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(4,10));
//		lockHolders.push_back(O(2,3));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(2,0,3) , lockHolders, ResourceLockType_X);
//		lockRequestBatch.push_back(lockReq);
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = false;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_ShardMoveRelease0:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(4,10));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(2,0,3) , lockHolders);
//		lockRequestBatch.push_back(lockReq);
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = true;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_ShardMoveRelease1:
//	{
//		vector<O> lockHolders;
//		lockHolders.push_back(O(2,3));
//		vector<SingleResourceLockRequest *> lockRequestBatch;
//		SingleResourceLockRequest * lockReq = new SingleResourceLockRequest(R(2,0,3) , lockHolders);
//		lockRequestBatch.push_back(lockReq);
//		//
//		ResourceLockRequest * resourceLockReq = new ResourceLockRequest();
//		resourceLockReq->requestBatch = lockRequestBatch;
//		resourceLockReq->isBlocking = true;
//		return resourceLockReq;
//	}
//	case ResourceLockRequestCaseCode_EOF:
//		ASSERT(false);
//		return NULL;
//	}
//}


//void initializeLockHoldersRepository(const LockReposTestCode caseValue, LockHoldersRepository * repo);
//
//void initializeLockHoldersRepositoryResults(const LockReposTestCode caseValue, LockHoldersRepository * repo){
//	if(repo == NULL){
//		return;
//	}
//
//	initializeLockHoldersRepository(caseValue, repo);
//
//	LockRepoInitializer repoStream(repo);
//
//	switch (caseValue) {
//	//////////////////////////////// S lock test cases //////////////////////////////////
//	case LockReposTestCode_S_Lock_Grant_0:
//		// goal : acquire s lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,1) << O(0,2) << O(0,1);
//		return;
//	case LockReposTestCode_S_Lock_Grant_1:
//		// goal : acquire s lock on (0,0,0) for <(0,2),(0,1)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,2) << O(0,1) ;
//		return;
//	case LockReposTestCode_S_Lock_Grant_2:
//		// goal : acquire s lock on (0,0,0) for <(2,1),(2,2)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(0,2)>
//		// and x lock for (0,1,11) on <(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(2,1) << O(2,2);
//		return;
//	case LockReposTestCode_S_Lock_Grant_3:
//		// goal : acquire s lock on (0,0,0) for <(2,2),(2,3)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and u lock for (0,0,0) on <(0,1),(2,2)>
//		// and u lock for (0,0,1) on <(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(2,2) << O(2,3) ;
//		return;
//	case LockReposTestCode_S_Lock_Reject_0:
//		// goal : reject s lock on (0,1,10) for <(1,1),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and x lock for (0,1,10) on <(0,1),(0,2)>
//		// and x lock for (0,1,11) on <(0,1)>
//		return;
//	case LockReposTestCode_S_Lock_Reject_1:
//		// goal : reject s lock on (0,0,0) for <(2,2),(2,3)>
//		// add x lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(2,2)>
//		return;
//	case LockReposTestCode_S_Lock_Reject_2:
//		// goal : reject s lock on (0,1,10) for <(0,2),(0,4)>
//		// add s lock for (0,0,4) on <(0,4)>
//		// add s lock for (0,0,5) on <(0,5)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add u lock for (0,0,1) on <(0,4),(0,1),(1,1)>
//		// and x lock for (0,1,10) on <(0,1)>
//		return;
//	case LockReposTestCode_S_Lock_Reject_3:
//		// goal : reject s lock on (0,1,10) for <(1,2),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and u lock for (0,0,0) on <(1,2),(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		return;
//
//		//////////////////////////////// U lock test cases //////////////////////////////////
//	case LockReposTestCode_U_Lock_Grant_0:
//		// goal : acquire u lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'U';
//		repoStream << R(0,0,1) << O(0,2) << O(0,1);
//		return;
//	case LockReposTestCode_U_Lock_Grant_1:
//		// goal : acquire u lock on (0,0,0) for <(0,2),(0,1)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,2) << O(0,1) ;
//		return;
//	case LockReposTestCode_U_Lock_Reject_0:
//		// goal : reject u lock on (0,1,10) for <(1,1),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and x lock for (0,1,10) on <(0,1),(0,2)>
//		// and x lock for (0,1,11) on <(0,1)>
//		return;
//	case LockReposTestCode_U_Lock_Reject_1:
//		// goal : reject u lock on (0,0,0) for <(2,2),(2,3)>
//		// add x lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(2,2)>
//		return;
//	case LockReposTestCode_U_Lock_Reject_2:
//		// goal : reject u lock on (0,1,10) for <(0,2),(0,4)>
//		// add s lock for (0,0,4) on <(0,4)>
//		// add s lock for (0,0,5) on <(0,5)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add u lock for (0,0,1) on <(0,4),(0,1),(1,1)>
//		// and x lock for (0,1,10) on <(0,1)>
//		return;
//	case LockReposTestCode_U_Lock_Reject_3:
//		// goal : reject u lock on (0,1,10) for <(1,2),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and u lock for (0,0,0) on <(1,2),(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		return;
//	case LockReposTestCode_U_Lock_Reject_4:
//		// goal : acquire u lock on (0,0,0) for <(2,1),(2,2)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(0,2)>
//		// and x lock for (0,1,11) on <(0,2),(1,1)>
//		return;
//	case LockReposTestCode_U_Lock_Reject_5:
//		// goal : acquire u lock on (0,0,0) for <(2,2),(2,3)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and u lock for (0,0,0) on <(0,1),(2,2)>
//		// and u lock for (0,0,1) on <(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		return;
//		//////////////////////////////// X lock test cases //////////////////////////////////
//	case LockReposTestCode_X_Lock_Grant_0:
//		// goal : acquire x lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'X';
//		repoStream << R(0,0,1) << O(0,2) << O(0,1);
//		return;
//	case LockReposTestCode_X_Lock_Reject_0:
//		// goal : reject x lock on (0,1,10) for <(1,1),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and x lock for (0,1,10) on <(0,1),(0,2)>
//		// and x lock for (0,1,11) on <(0,1)>
//		return;
//	case LockReposTestCode_X_Lock_Reject_1:
//		// goal : reject x lock on (0,0,0) for <(2,2),(2,3)>
//		// add x lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(2,2)>
//		return;
//	case LockReposTestCode_X_Lock_Reject_2:
//		// goal : reject x lock on (0,1,10) for <(0,2),(0,4)>
//		// add s lock for (0,0,4) on <(0,4)>
//		// add s lock for (0,0,5) on <(0,5)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add u lock for (0,0,1) on <(0,4),(0,1),(1,1)>
//		// and x lock for (0,1,10) on <(0,1)>
//		return;
//	case LockReposTestCode_X_Lock_Reject_3:
//		// goal : reject x lock on (0,1,10) for <(1,2),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and u lock for (0,0,0) on <(1,2),(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		return;
//	case LockReposTestCode_X_Lock_Reject_4:
//		// goal : reject x lock on (0,0,0) for <(2,1),(2,2)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(0,2)>
//		// and x lock for (0,1,11) on <(0,2),(1,1)>
//		return;
//	case LockReposTestCode_X_Lock_Reject_5:
//		// goal : reject x lock on (0,0,0) for <(2,2),(2,3)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and u lock for (0,0,0) on <(0,1),(2,2)>
//		// and u lock for (0,0,1) on <(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		return;
//	case LockReposTestCode_X_Lock_Reject_6:
//		// goal : reject x lock on (0,0,0) for <(0,2),(0,1)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		return;
//	case LockReposTestCode_EOF:
//		ASSERT(false);
//		return;
//	}
//}
//
//void initializeLockHoldersRepository(const LockReposTestCode caseValue, LockHoldersRepository * repo){
//	if(repo == NULL){
//		return;
//	}
//
//	LockRepoInitializer repoStream(repo);
//
//	switch (caseValue) {
//	//////////////////////////////// S lock test cases //////////////////////////////////
//	case LockReposTestCode_S_Lock_Grant_0:
//		// goal : acquire s lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(0,2) ;
//		return;
//	case LockReposTestCode_S_Lock_Grant_1:
//		// goal : acquire s lock on (0,0,0) for <(0,2),(0,1)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(0,2) ;
//		return;
//	case LockReposTestCode_S_Lock_Grant_2:
//		// goal : acquire s lock on (0,0,0) for <(2,1),(2,2)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(0,2)>
//		// and x lock for (0,1,11) on <(0,2),(1,1)>
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,2) ;
//		repoStream << R(0,1,11) << O(0,2) << O(1,1) ;
//		return;
//	case LockReposTestCode_S_Lock_Grant_3:
//		// goal : acquire s lock on (0,0,0) for <(2,2),(2,3)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and u lock for (0,0,0) on <(0,1),(2,2)>
//		// and u lock for (0,0,1) on <(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,1) << O(2,2) << O(2,1);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(2,2);
//		repoStream << R(0,0,1) << O(2,2);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2) << O(1,1) ;
//		return;
//	case LockReposTestCode_S_Lock_Reject_0:
//		// goal : reject s lock on (0,1,10) for <(1,1),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and x lock for (0,1,10) on <(0,1),(0,2)>
//		// and x lock for (0,1,11) on <(0,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,1) << O(2,2) << O(2,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2);
//		repoStream << R(0,1,11) << O(0,1);
//		return;
//	case LockReposTestCode_S_Lock_Reject_1:
//		// goal : reject s lock on (0,0,0) for <(2,2),(2,3)>
//		// add x lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(2,2)>
//		repoStream << 'X';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(2,2);
//		return;
//	case LockReposTestCode_S_Lock_Reject_2:
//		// goal : reject s lock on (0,1,10) for <(0,2),(0,4)>
//		// add s lock for (0,0,4) on <(0,4)>
//		// add s lock for (0,0,5) on <(0,5)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add u lock for (0,0,1) on <(0,4),(0,1),(1,1)>
//		// and x lock for (0,1,10) on <(0,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,4) << O(0,4);
//		repoStream << R(0,0,5) << O(0,5);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,4) << O(0,1) << O(1,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1);
//		return;
//	case LockReposTestCode_S_Lock_Reject_3:
//		// goal : reject s lock on (0,1,10) for <(1,2),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and u lock for (0,0,0) on <(1,2),(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(1,2) << O(2,2);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2) << O(1,1) ;
//		return;
//		//////////////////////////////// U lock test cases //////////////////////////////////
//	case LockReposTestCode_U_Lock_Grant_0:
//		// goal : acquire u lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(0,2) ;
//		return;
//	case LockReposTestCode_U_Lock_Grant_1:
//		// goal : acquire u lock on (0,0,0) for <(0,2),(0,1)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(0,2) ;
//		return;
//	case LockReposTestCode_U_Lock_Reject_0:
//		// goal : reject u lock on (0,1,10) for <(1,1),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and x lock for (0,1,10) on <(0,1),(0,2)>
//		// and x lock for (0,1,11) on <(0,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,1) << O(2,2) << O(2,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2);
//		repoStream << R(0,1,11) << O(0,1);
//		return;
//	case LockReposTestCode_U_Lock_Reject_1:
//		// goal : reject u lock on (0,0,0) for <(2,2),(2,3)>
//		// add x lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(2,2)>
//		repoStream << 'X';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(2,2);
//		return;
//	case LockReposTestCode_U_Lock_Reject_2:
//		// goal : reject u lock on (0,1,10) for <(0,2),(0,4)>
//		// add s lock for (0,0,4) on <(0,4)>
//		// add s lock for (0,0,5) on <(0,5)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add u lock for (0,0,1) on <(0,4),(0,1),(1,1)>
//		// and x lock for (0,1,10) on <(0,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,4) << O(0,4);
//		repoStream << R(0,0,5) << O(0,5);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,4) << O(0,1) << O(1,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1);
//		return;
//	case LockReposTestCode_U_Lock_Reject_3:
//		// goal : reject u lock on (0,1,10) for <(1,2),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and u lock for (0,0,0) on <(1,2),(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(1,2) << O(2,2);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2) << O(1,1) ;
//		return;
//	case LockReposTestCode_U_Lock_Reject_4:
//		// goal : acquire u lock on (0,0,0) for <(2,1),(2,2)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(0,2)>
//		// and x lock for (0,1,11) on <(0,2),(1,1)>
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,2) ;
//		repoStream << R(0,1,11) << O(0,2) << O(1,1) ;
//		return;
//	case LockReposTestCode_U_Lock_Reject_5:
//		// goal : acquire u lock on (0,0,0) for <(2,2),(2,3)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and u lock for (0,0,0) on <(0,1),(2,2)>
//		// and u lock for (0,0,1) on <(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,1) << O(2,2) << O(2,1);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(2,2);
//		repoStream << R(0,0,1) << O(2,2);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2) << O(1,1) ;
//		return;
//		//////////////////////////////// X lock test cases //////////////////////////////////
//	case LockReposTestCode_X_Lock_Grant_0:
//		// goal : acquire x lock on (0,0,1) for <(0,2),(0,1)> (no possible conflict)
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(0,2) ;
//		return;
//	case LockReposTestCode_X_Lock_Reject_0:
//		// goal : reject x lock on (0,1,10) for <(1,1),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and x lock for (0,1,10) on <(0,1),(0,2)>
//		// and x lock for (0,1,11) on <(0,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,1) << O(2,2) << O(2,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2);
//		repoStream << R(0,1,11) << O(0,1);
//		return;
//	case LockReposTestCode_X_Lock_Reject_1:
//		// goal : reject x lock on (0,0,0) for <(2,2),(2,3)>
//		// add x lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(2,2)>
//		repoStream << 'X';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(2,2);
//		return;
//	case LockReposTestCode_X_Lock_Reject_2:
//		// goal : reject x lock on (0,1,10) for <(0,2),(0,4)>
//		// add s lock for (0,0,4) on <(0,4)>
//		// add s lock for (0,0,5) on <(0,5)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add u lock for (0,0,1) on <(0,4),(0,1),(1,1)>
//		// and x lock for (0,1,10) on <(0,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,4) << O(0,4);
//		repoStream << R(0,0,5) << O(0,5);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,4) << O(0,1) << O(1,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1);
//		return;
//	case LockReposTestCode_X_Lock_Reject_3:
//		// goal : reject x lock on (0,1,10) for <(1,2),(2,2)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and u lock for (0,0,0) on <(1,2),(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(1,2) << O(2,2);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2) << O(1,1) ;
//		return;
//	case LockReposTestCode_X_Lock_Reject_4:
//		// goal : acquire x lock on (0,0,0) for <(2,1),(2,2)>
//		// add u lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and x lock for (0,1,10) on <(0,2)>
//		// and x lock for (0,1,11) on <(0,2),(1,1)>
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,2) ;
//		repoStream << R(0,1,11) << O(0,2) << O(1,1) ;
//		return;
//	case LockReposTestCode_X_Lock_Reject_5:
//		// goal : acquire x lock on (0,0,0) for <(2,2),(2,3)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// add s lock for (0,0,1) on <(0,1),(2,2),(2,1)>
//		// and u lock for (0,0,0) on <(0,1),(2,2)>
//		// and u lock for (0,0,1) on <(2,2)>
//		// and x lock for (0,1,10) on <(0,1),(0,2),(1,1)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,0,1) << O(0,1) << O(2,2) << O(2,1);
//		repoStream << 'U';
//		repoStream << R(0,0,0) << O(0,1) << O(2,2);
//		repoStream << R(0,0,1) << O(2,2);
//		repoStream << 'X';
//		repoStream << R(0,1,10) << O(0,1) << O(0,2) << O(1,1) ;
//		return;
//	case LockReposTestCode_X_Lock_Reject_6:
//		// goal : acquire x lock on (0,0,0) for <(0,2),(0,1)>
//		// add s lock for (0,0,0) on <(0,1),(0,2),(1,1)>
//		// and s lock for (0,1,10) on <(0,2)>
//		repoStream << 'S';
//		repoStream << R(0,0,0) << O(0,1) << O(0,2) << O(1,1);
//		repoStream << R(0,1,10) << O(0,2) ;
//		return;
//	case LockReposTestCode_EOF:
//		ASSERT(false);
//		return;
//	}
//}
