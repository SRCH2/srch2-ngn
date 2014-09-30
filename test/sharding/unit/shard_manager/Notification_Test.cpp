#include "core/util/Assert.h"

#include <iostream>
#include <sstream>
#include <time.h>
#include <string>
#include <stdlib.h>

#include "sharding/sharding/notifications/Notification.h"
#include "sharding/sharding/notifications/NewNodeLockNotification.h"
#include "sharding/sharding/notifications/MoveToMeNotification.h"
#include "sharding/sharding/notifications/CommitNotification.h"
#include "sharding/sharding/notifications/CopyToMeNotification.h"
#include "sharding/sharding/notifications/MetadataReport.h"
#include "sharding/sharding/notifications/LockingNotification.h"
#include "sharding/sharding/notifications/LoadBalancingReport.h"
#include "sharding/transport/MessageAllocator.h"

#include "NotificationTestHelper.h"
#include "LockManagerTestHelper.h"
#include "MetadataManagerTestHelper.h"

using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;


void testNewNodeLockNotification(){

	MessageAllocator messageAllocator;
	for(unsigned i = (unsigned)ResourceLockRequestCaseCode_NewNodeLock0;
			i < (unsigned)ResourceLockRequestCaseCode_EOF; ++i){
		vector<NodeId> nodesUpToThisNode;
		nodesUpToThisNode.push_back(0);
		nodesUpToThisNode.push_back(2);
		nodesUpToThisNode.push_back(3);
		nodesUpToThisNode.push_back(4);
		nodesUpToThisNode.push_back(6);
		ResourceLockRequest * lockReq = createResourceLockRequest((ResourceLockRequestCaseCode)i);
		NewNodeLockNotification * newNodeLockNotif = new NewNodeLockNotification(nodesUpToThisNode, lockReq);
		void * buffer = messageAllocator.allocateByteArray(newNodeLockNotif->getNumberOfBytes());
		newNodeLockNotif->serialize(buffer);
		NewNodeLockNotification * deserNotif = ShardingNotification::deserializeAndConstruct<NewNodeLockNotification>(buffer);

		ASSERT(*newNodeLockNotif == *deserNotif);
	}
}

void testNewNodeLockNotificationAck(){
	MessageAllocator messageAllocator;
	LockHoldersRepository lockRepo;

	for(unsigned i = 0 ; i < (unsigned)LockReposTestCode_EOF ; ++i){

		initializeLockHoldersRepository((LockReposTestCode)i, &lockRepo);

		NewNodeLockNotification::ACK * newNodeLockAck = new NewNodeLockNotification::ACK(&lockRepo);
		void * buffer = messageAllocator.allocateByteArray(newNodeLockAck->getNumberOfBytes());
		newNodeLockAck->serialize(buffer);
		NewNodeLockNotification::ACK * deserNotif = ShardingNotification::deserializeAndConstruct<NewNodeLockNotification::ACK>(buffer);

		if(!(*newNodeLockAck == *deserNotif)){
			cout << "Serialized repository : "<< endl;
			lockRepo.print();
			cout << "=======================================" << endl;
			cout << "Deserialized repository : "<< endl;
			deserNotif->getShardLockRepository()->print();
			cout << "=======================================" << endl;
			ASSERT(false);
		}

		lockRepo.clear();
	}
}


void testLockingNotification(){

	MessageAllocator messageAllocator;
	for(unsigned i = (unsigned)ResourceLockRequestCaseCode_NewNodeLock0;
			i < (unsigned)ResourceLockRequestCaseCode_EOF; ++i){
		ResourceLockRequest * lockReq = createResourceLockRequest((ResourceLockRequestCaseCode)i);
		LockingNotification * lockingNotif = new LockingNotification(lockReq);
		void * buffer = messageAllocator.allocateByteArray(lockingNotif->getNumberOfBytes());
		lockingNotif->serialize(buffer);
		LockingNotification * deserNotif = ShardingNotification::deserializeAndConstruct<LockingNotification>(buffer);

		ASSERT(*lockingNotif == *deserNotif);
	}
}

void testLockingNotificationAck(){
	MessageAllocator messageAllocator;

	{
		LockingNotification::ACK * lockAck = new LockingNotification::ACK(true);
		void * buffer = messageAllocator.allocateByteArray(lockAck->getNumberOfBytes());
		lockAck->serialize(buffer);
		LockingNotification::ACK * deserNotif = ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(buffer);

		ASSERT(*lockAck == *deserNotif);
	}

	{
		LockingNotification::ACK * lockAck = new LockingNotification::ACK(false);
		void * buffer = messageAllocator.allocateByteArray(lockAck->getNumberOfBytes());
		lockAck->serialize(buffer);
		LockingNotification::ACK * deserNotif = ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(buffer);

		ASSERT(*lockAck == *deserNotif);
	}
}

void testCommitNotification(){

	MessageAllocator messageAllocator;
	for(unsigned i = 0 ; i < (unsigned)MetadataChangeTestCaseCode_EOF; ++i){
		//TODO
		MetadataChange * change = createMetadataChange((MetadataChangeTestCaseCode)i);
		CommitNotification * commitNotif = new CommitNotification(change);
		void * buffer = messageAllocator.allocateByteArray(commitNotif->getNumberOfBytes());
		commitNotif->serialize(buffer);
		CommitNotification * deserNotif = ShardingNotification::deserializeAndConstruct<CommitNotification>(buffer);

		ASSERT(*commitNotif == *deserNotif);
	}
}

void testCommitNotificationAck(){
	MessageAllocator messageAllocator;
	CommitNotification::ACK * commitNotifAck = new CommitNotification::ACK();
	void * buffer = messageAllocator.allocateByteArray(commitNotifAck->getNumberOfBytes());
	commitNotifAck->serialize(buffer);
	CommitNotification::ACK * deserNotif = ShardingNotification::deserializeAndConstruct<CommitNotification::ACK>(buffer);
	ASSERT(*commitNotifAck == *deserNotif);
}


void testMoveToMeNotification(){
	MessageAllocator messageAllocator;
	ClusterShardId shardId(0,1,1);
	MoveToMeNotification::START * moveNotif = new MoveToMeNotification::START(shardId);
	void * buffer = messageAllocator.allocateByteArray(moveNotif->getNumberOfBytes());
	moveNotif->serialize(buffer);
	MoveToMeNotification::START * deserNotif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::START>(buffer);
	ASSERT(*moveNotif == *deserNotif);

}

void testCopyToMeNotification(){
	MessageAllocator messageAllocator;
	ClusterShardId shardId(0,1,1);
	CopyToMeNotification * copyNotif = new CopyToMeNotification(shardId);
	void * buffer = messageAllocator.allocateByteArray(copyNotif->getNumberOfBytes());
	copyNotif->serialize(buffer);
	CopyToMeNotification * deserNotif = ShardingNotification::deserializeAndConstruct<CopyToMeNotification>(buffer);
	ASSERT(*copyNotif == *deserNotif);

}

void testMetadataReport(){
	string confPath(getenv("ConfigManagerFilePath"));
	for(unsigned code = 0 ; code < 20; ++code){
		// remove lock file.
		Cluster_Writeview * writeview = createWriteview(code, confPath);
		MessageAllocator messageAllocator;
		MetadataReport * report = new MetadataReport(writeview);
		void * buffer = messageAllocator.allocateByteArray(report->getNumberOfBytes());
		report->serialize(buffer);
		MetadataReport * deserNotif = ShardingNotification::deserializeAndConstruct<MetadataReport>(buffer);
		ASSERT(report->getWriteview()->isEqualDiscardingLocalShards(*(deserNotif->getWriteview())));
	}
}

int main(){
	testNewNodeLockNotification();
	testNewNodeLockNotificationAck();
	testLockingNotification();
	testLockingNotificationAck();
	testCommitNotification();
	testCommitNotificationAck();
	testMoveToMeNotification();
	testCopyToMeNotification();
	testMetadataReport();

    cout << "Notification unit tests: Passed" << endl;
}
