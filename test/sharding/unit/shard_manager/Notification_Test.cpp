#include "core/util/Assert.h"

#include <iostream>
#include <sstream>
#include <time.h>
#include <string>
#include <stdlib.h>

#include "sharding/sharding/notifications/Notification.h"
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


void testLockingNotification(){

//	MessageAllocator messageAllocator;
//	for(unsigned i = (unsigned)ResourceLockRequestCaseCode_NewNodeLock0;
//			i < (unsigned)ResourceLockRequestCaseCode_EOF; ++i){
//		ResourceLockRequest * lockReq = createResourceLockRequest((ResourceLockRequestCaseCode)i);
//		LockingNotification * lockingNotif = new LockingNotification(lockReq);
//		void * buffer = messageAllocator.allocateByteArray(lockingNotif->getNumberOfBytes());
//		lockingNotif->serialize(buffer);
//		LockingNotification * deserNotif = ShardingNotification::deserializeAndConstruct<LockingNotification>(buffer);
//
//		ASSERT(*lockingNotif == *deserNotif);
//	}
}

void testLockingNotificationAck(){
	MessageAllocator messageAllocator;

	{
		SP(LockingNotification::ACK) lockAck = SP(LockingNotification::ACK)(new LockingNotification::ACK(true));
		void * buffer = messageAllocator.allocateByteArray(lockAck->getNumberOfBytesAll());
		lockAck->serializeAll(buffer);
		SP(LockingNotification::ACK) deserNotif = ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(buffer);

		ASSERT(*lockAck == *deserNotif);
	}

	{
		SP(LockingNotification::ACK) lockAck = SP(LockingNotification::ACK)(new LockingNotification::ACK(false));
		void * buffer = messageAllocator.allocateByteArray(lockAck->getNumberOfBytesAll());
		lockAck->serializeAll(buffer);
		SP(LockingNotification::ACK) deserNotif = ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(buffer);

		ASSERT(*lockAck == *deserNotif);
	}
}

void testCommitNotification(){

	MessageAllocator messageAllocator;
	for(unsigned i = 0 ; i < (unsigned)MetadataChangeTestCaseCode_EOF; ++i){
		//TODO
		MetadataChange * change = createMetadataChange((MetadataChangeTestCaseCode)i);
		SP(CommitNotification) commitNotif = SP(CommitNotification)(new CommitNotification(change));
		void * buffer = messageAllocator.allocateByteArray(commitNotif->getNumberOfBytesAll());
		commitNotif->serializeAll(buffer);
		SP(CommitNotification) deserNotif = ShardingNotification::deserializeAndConstruct<CommitNotification>(buffer);

		ASSERT(*commitNotif == *deserNotif);
	}
}

void testCommitNotificationAck(){
	MessageAllocator messageAllocator;
	SP(CommitNotification::ACK) commitNotifAck = SP(CommitNotification::ACK)(new CommitNotification::ACK());
	void * buffer = messageAllocator.allocateByteArray(commitNotifAck->getNumberOfBytesAll());
	commitNotifAck->serializeAll(buffer);
	SP(CommitNotification::ACK) deserNotif = ShardingNotification::deserializeAndConstruct<CommitNotification::ACK>(buffer);
	ASSERT(*commitNotifAck == *deserNotif);
}


void testMoveToMeNotification(){
//	MessageAllocator messageAllocator;
//	ClusterShardId shardId(0,1,1);
//	MoveToMeNotification::START * moveNotif = new MoveToMeNotification::START(shardId);
//	void * buffer = messageAllocator.allocateByteArray(moveNotif->getNumberOfBytes());
//	moveNotif->serialize(buffer);
//	MoveToMeNotification::START * deserNotif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::START>(buffer);
//	ASSERT(*moveNotif == *deserNotif);

}

void testCopyToMeNotification(){
	MessageAllocator messageAllocator;
	ClusterShardId shardId(0,1,1);
	SP(CopyToMeNotification) copyNotif = SP(CopyToMeNotification)(new CopyToMeNotification(shardId));
	void * buffer = messageAllocator.allocateByteArray(copyNotif->getNumberOfBytesAll());
	copyNotif->serializeAll(buffer);
	SP(CopyToMeNotification) deserNotif = ShardingNotification::deserializeAndConstruct<CopyToMeNotification>(buffer);
	ASSERT(*copyNotif == *deserNotif);

}

void testMetadataReport(){
	string confPath(getenv("ConfigManagerFilePath"));
	for(unsigned code = 0 ; code < 20; ++code){
		// remove lock file.
		Cluster_Writeview * writeview = createWriteview(code, confPath);
		MessageAllocator messageAllocator;
		SP(MetadataReport) report = SP(MetadataReport)(new MetadataReport(writeview));
		void * buffer = messageAllocator.allocateByteArray(report->getNumberOfBytesAll());
		report->serializeAll(buffer);
		SP(MetadataReport) deserNotif = ShardingNotification::deserializeAndConstruct<MetadataReport>(buffer);
		ASSERT(report->getWriteview()->isEqualDiscardingLocalShards(*(deserNotif->getWriteview())));
	}
}

int main(){
	testLockingNotification();
	testLockingNotificationAck();
	testCommitNotification();
	testCommitNotificationAck();
	testMoveToMeNotification();
	testCopyToMeNotification();
	testMetadataReport();

    cout << "Notification unit tests: Passed" << endl;
}
