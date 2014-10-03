#ifndef __SHARDING_SHARDING_ATIMIC_SAVE_OPERATION_H__
#define __SHARDING_SHARDING_ATIMIC_SAVE_OPERATION_H__

#include "../State.h"
#include "../notifications/Notification.h"
#include "../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * Saves the indices and the cluster metadata on all nodes in the cluster.
 * NOTE : this operation assumes all shards are locked in S mode
 * 1. request all nodes to save their indices
 * 2. When all nodes saved their indices, request all nodes to save their cluster metadata
 * 3. When all nodes acked metadata save, write the metadata on disk and done.
 */
class AtomicSaveOperation: public OperationState {
public:

	AtomicSaveOperation(const unsigned operationId);
	~AtomicSaveOperation();

	OperationState * entry();
	OperationState * handle(SaveDataNotification::ACK * notification);
	OperationState * handle(SaveMetadataNotification::ACK * notification);
	OperationState * handle(NodeFailureNotification * nodeFailure);
	OperationState * handle(Notification * notification);

	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	map<NodeId, bool> nodesStatus;
	bool dataSavedFlag ;

	OperationState * saveMetadata();
	OperationState * finalize();

	bool haveAllNodeseReplied() const;

	static void * localSaveData(void * arg);
	static void * localSaveMetadata(void * arg);
};


}

}


#endif // __SHARDING_SHARDING_ATIMIC_SAVE_OPERATION_H__
