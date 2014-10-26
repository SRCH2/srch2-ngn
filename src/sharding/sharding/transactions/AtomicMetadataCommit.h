#ifndef __SHARDING_SHARDING_METADATA_MANAGER_H__
#define __SHARDING_SHARDING_METADATA_MANAGER_H__


#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../notifications/CommitNotification.h"
#include "../metadata_manager/ResourceMetadataChange.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../../configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class AtomicRelease;
class AtomicLock;

class AtomicMetadataCommit : public ProducerInterface, public NodeIteratorListenerInterface{
public:

	AtomicMetadataCommit(const vector<NodeId> & exceptions,
			MetadataChange * metadataChange, ConsumerInterface * consumer, const bool skipLock = false);
	AtomicMetadataCommit(const NodeId & exception,
			MetadataChange * metadataChange, ConsumerInterface * consumer, const bool skipLock = false);
	AtomicMetadataCommit(MetadataChange * metadataChange,
			const vector<NodeId> & participants, ConsumerInterface * consumer, const bool skipLock = false);
	~AtomicMetadataCommit();

	Transaction * getTransaction();

	void produce();

	void lock(); // locks metadata
	void consume(bool granted);


	void commit();
	bool shouldAbort(const NodeId & failedNode);
	void end_(map<NodeOperationId , SP(ShardingNotification)> & replies) ; // receives notification when commit it done.


	void release(); // unlocks metadata
	string getName() const {return "metadata-commit";};
private:
	// control members
	vector<NodeId> participants;
	NodeOperationId selfOperationId;

	bool skipLock;

	string currentAction;
	AtomicLock * atomicLock;
	AtomicRelease * atomicRelease;
	SP(CommitNotification) commitNotification;
	MetadataChange * metadataChange;

	bool finalizedFlag ;

	void finalize(bool result);
};


}
}

#endif // __SHARDING_SHARDING_METADATA_MANAGER_H__
