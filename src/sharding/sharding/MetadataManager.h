#ifndef __SHARDING_SHARDING_METADATA_MANAGER_H__
#define __SHARDING_SHARDING_METADATA_MANAGER_H__


#include "State.h"
#include "Notification.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class CommitNotification : public BroadcastNotification{
public:

	class ACK : public ShardingNotification{
	public:
		ACK(NodeId srcNodeId,
				unsigned srcOperationId,
				NodeId destNodeId,
				unsigned destOperationId):ShardingNotification(srcNodeId, srcOperationId, destNodeId, destOperationId){};

		NotificationType getType() const{
			return NotificationType_Sharding_Commit_ACK;
		}

	    ShardingMessageType messageType() const;
		static CommitNotification::ACK * deserializeAndConstruct(void * buffer) const{
			CommitNotification::ACK * newAck = new CommitNotification::ACK(0,0,0,0);
			newAck->deserialize(buffer);
			return newAck;
		}

	};

	CommitNotification(NodeId srcNodeId,
			unsigned srcOperationId,
			NodeId destNodeId,
			unsigned destOperationId,
			MetadataChange * metadataChange);

	NotificationType getType() const;
	MetadataChange * getMetadataChange() const;

	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer) const;
    ShardingMessageType messageType() const;
    static CommitNotification * deserializeAndConstruct(void * buffer) const{
    	CommitNotification * newCommit = new CommitNotification(0,0,0,0,NULL);
    	newCommit->deserialize(buffer);
    	return newCommit;
    }

private:
	MetadataChange * metadataChange;
};


class CommitOperation : public OperationState{
public:

	CommitOperation(unsigned operationId, MetadataChange * metadataChange);
	OperationStateType getType();
	void entry(map<NodeId, unsigned> specialTargets);
	// returns false when it's done.
	bool handle(CommitNotification::ACK * inputNotification);
	bool doesExpect(CommitNotification::ACK * inputNotification) const;
	// TODO adjustToNodeFailure ???
private:
	MetadataChange * metadataChange;

	map<NodeId, bool> haveReplied;

	bool hasAllNodesReplied() const;
};

/*
 * This class provides the API to do operations on Metadata such as commit
 */
class MetadataManager{
public:

	static MetadataManager * createMetadataManager(const ClusterResourceMetadata_Readview & readview);
	static MetadataManager * getMetadataManager();

	MetadataManager(const ClusterResourceMetadata_Readview & readview);
	ClusterResourceMetadata_Writeview & getWriteview() const;


	void resolve(CommitNotification * notification);
	// applies the change,
	// commits
	// and returns the previous versionID
	unsigned applyAndCommit(MetadataChange * shardingChange);
	ShardManager * getShardManager();

private:

	static MetadataManager * singleInstance;

	ClusterResourceMetadata_Writeview writeview;

};

}
}

#endif // __SHARDING_SHARDING_METADATA_MANAGER_H__
