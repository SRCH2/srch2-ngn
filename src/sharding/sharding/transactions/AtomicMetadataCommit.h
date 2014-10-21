#ifndef __SHARDING_SHARDING_METADATA_MANAGER_H__
#define __SHARDING_SHARDING_METADATA_MANAGER_H__


#include "../State.h"
#include "../notifications/Notification.h"
#include "../../configuration/ShardingConstants.h"
#include "../notifications/CommitNotification.h"
#include "../metadata_manager/ResourceMetadataChange.h"
#include "../metadata_manager/ResourceMetadataManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class AtomicMetadataCommit : public ConsumerInterface, public ProducerInterface, public NodeIteratorListenerInterface{
public:

	AtomicMetadataCommit(const vector<NodeId> & exceptions,
			MetadataChange * metadataChange, ConsumerInterface * consumer);
	AtomicMetadataCommit(const NodeId & exception,
			MetadataChange * metadataChange, ConsumerInterface * consumer);
	AtomicMetadataCommit(MetadataChange * metadataChange,
			const vector<NodeId> & participants, ConsumerInterface * consumer);
	~AtomicMetadataCommit();

	void produce();

	void lock(); // locks metadata
	void consume(bool granted);


	void commit();
	bool shouldAbort(const NodeId & failedNode);
	void end_(map<NodeOperationId , ShardingNotification *> & replies) ; // receives notification when commit it done.


	void release(); // unlocks metadata

private:
	// control members
	vector<NodeId> participants;
	NodeOperationId selfOperationId;

	string currentAction;
	AtomicLock * atomicLock;
	AtomicRelease * atomicRelease;
	CommitNotification * commitNotification;
	MetadataChange * metadataChange;
	ConsumerInterface * consumer;

	bool finalizedFlag ;

	void finalize(bool result);
};


}
}

#endif // __SHARDING_SHARDING_METADATA_MANAGER_H__
