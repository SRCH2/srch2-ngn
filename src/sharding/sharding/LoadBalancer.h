#ifndef __SHARDING_SHARDING_LOAD_BALANCER_H__
#define __SHARDING_SHARDING_LOAD_BALANCER_H__

#include "State.h"
#include "Notification.h"
#include "MetadataManager.h"
#include "LockManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {




class LoadBalancingOperation : public OperationState{
public:
	LoadBalancingOperation(unsigned operationId): OperationState(operationId){};
private:

};
/*
 * This class is responsible of keeping the load of cluster balanced upon node arrival and
 * node failure.
 */
class LoadBalancer{
public:

	static LoadBalancer * createLoadBalancer();
	static LoadBalancer * getLoadBalancer();


	LoadBalancer(){}

	void handlerAll();

	// called from migration manager to inform us about the status of a migration
	void resolveMMNotification(ShardMigrationStatus migrationStatus);
	void resolve(CopyToMeNotification * copyToMeNotification);
	void resolve(MoveToMeNotification * moveToMeNotification);
	void resolve(ProposalNotification * proposal);
	void resolve(ProposalNotification::OK * proposalAck);
	void resolve(ProposalNotification::NO * proposalNo);
	void resolve(CommitNotification::ACK * commitAckNotification);

	void resolve(CommitNotification * commitNotification);
	void doesExpect(CommitNotification * commitNotification);

private:
	static LoadBalancer * singleInstance ;


	map<unsigned , LoadBalancingOperation * > operations;
	std::queue<Notification *> buffer;
};

}
}


#endif // __SHARDING_SHARDING_LOAD_BALANCER_H__
