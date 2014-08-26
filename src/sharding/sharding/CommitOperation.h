#ifndef __SHARDING_SHARDING_METADATA_MANAGER_H__
#define __SHARDING_SHARDING_METADATA_MANAGER_H__


#include "State.h"
#include "./notifications/Notification.h"
#include "sharding/configuration/ShardingConstants.h"
#include "./notifications/CommitNotification.h"
#include "./metadata_manager/ResourceMetadataChange.h"
#include "metadata_manager/ResourceMetadataManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class CommitOperation : public OperationState{
public:

	CommitOperation(const unsigned & operationId, const vector<NodeId> & exceptions, MetadataChange * metadataChange);
	CommitOperation(const unsigned & operationId, const NodeId & exception, MetadataChange * metadataChange);
	~CommitOperation(){
		if(metadataChange != NULL){
			delete metadataChange;
		}
	}
	OperationState * entry();

	OperationState * handle(NodeFailureNotification * nodeFailure);
	OperationState * handle(CommitNotification::ACK * inputNotification);
	OperationState * handle(Notification * notification){
		if(notification == NULL){
			ASSERT(false);
			return this;
		}
		switch (notification->messageType()) {
			case ShardingNodeFailureNotificationMessageType:
				return handle((NodeFailureNotification *)notification);
			case ShardingCommitACKMessageType:
				return handle((CommitNotification::ACK *)notification);
			default:
				// ignore;
				return this;
		}
	}
	// returns false when it's done.
	bool doesExpect(CommitNotification::ACK * inputNotification) const;

	string getOperationName() const ;
	string getOperationStatus() const ;

private:
	MetadataChange * metadataChange;
	vector<NodeId> participants;
};


}
}

#endif // __SHARDING_SHARDING_METADATA_MANAGER_H__
