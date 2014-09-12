#ifndef __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__
#define __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__

#include "ClusterSaveOperation.h"

#include "../State.h"
#include "../notifications/Notification.h"
#include "../metadata_manager/Shard.h"
#include "../../processor/ProcessorUtil.h"


#include "core/util/Logger.h"
#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Safely shuts down the engine.
 * 1. Save the cluster data shards by disabling final release.
 * 2. Shut down the cluster.
 */
class ClusterShutdownOperation : public OperationState {
public:

	ClusterShutdownOperation():OperationState(this->getOperationId()){}
	~ClusterShutdownOperation(){}

private:

	OperationState * entry();

	OperationState * save();

public:

	OperationState * handleSaveOperation(Notification * notification);

	OperationState * shutdown();

private:

	OperationState * handle(Notification * notification);

	OperationState * handle(NodeFailureNotification * nodeFailureNotif);

private:

	OperationState * saveOperation;

};


}

}


#endif // __SHARDING_SHARDING_CLUSTER_SHUTDOWN_OPERATION_H__
