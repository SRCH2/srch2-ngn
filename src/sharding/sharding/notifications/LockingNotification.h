#ifndef __SHARDING_SHARDING_LOCKING_NOTIFICATION_H__
#define __SHARDING_SHARDING_LOCKING_NOTIFICATION_H__

#include "Notification.h"
#include "../../configuration/ShardingConstants.h"
#include "sharding/transport/Message.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class ResourceLockRequest;
class LockingNotification : public ShardingNotification{
public:

	// used for ShardCopy lock
	LockingNotification(const ClusterShardId & srcShardId,
			const ClusterShardId & destShardId,
			const NodeOperationId & copyAgent,
			const bool releaseRequest = false);

	// used for ShardMove lock
	LockingNotification(const ClusterShardId & shardId,
			const NodeOperationId & srcMoveAgent,
			const NodeOperationId & destMoveAgent,
			const bool releaseRequest = false);

	// used for NewNodeArrival lock
	LockingNotification(const NodeOperationId & newNodeOpId,
			const vector<NodeId> & listOfOlderNodes = vector<NodeId>(),
			const LockLevel & lockLevel = LockLevel_X,
			const bool blocking = true,
			const bool releaseRequest = false);

	// used for Insertion/Deletion/Update and all requests that need primary key lock
	LockingNotification(const vector<string> & primaryKeys,
			const NodeOperationId & writerAgent,
			const ClusterPID & pid,
			const bool releaseRequest = false);
	// used for general purpose cluster shard id locking
	LockingNotification(const ClusterShardId & shardId,
			const NodeOperationId & agent,
			const LockLevel & lockLevel);
	// used for releasing the general purpose lock
	LockingNotification(const ClusterShardId & shardId, const NodeOperationId & agent);

	LockingNotification(const vector<ClusterShardId> & shardIdList, const NodeOperationId & shardIdListLockHolder, const LockLevel & lockLevel); // for lock
	LockingNotification(const vector<ClusterShardId> & shardIdList, const NodeOperationId & shardIdListLockHolder); // for release



	LockingNotification();

	static bool resolveMessage(Message * msg, NodeId sendeNode);
	static void resolveNotif(LockingNotification * notif);
	void * serialize(void * buffer) const;
	unsigned getNumberOfBytes() const;
	void * deserialize(void * buffer);
	ShardingMessageType messageType() const;
    bool operator==(const LockingNotification & lockingNotification);

    string toString();
    LockRequestType getLockRequestType(){return lockRequestType;};
    void getLockRequestInfo(ClusterShardId & srcShardId, ClusterShardId & destShardId, NodeOperationId & copyAgent) const;
    void getLockRequestInfo(ClusterShardId & shardId, NodeOperationId & srcMoveAgent, NodeOperationId & destMoveAgent) const;
    void getLockRequestInfo(NodeOperationId & newNodeOpId, vector<NodeId> & listOfOlderNodes, LockLevel & lockLevel) const;
    void getLockRequestInfo(vector<string> & primaryKeys, NodeOperationId & writerAgent, ClusterPID & pid) const;
    void getLockRequestInfo(ClusterShardId & shardId, NodeOperationId & agent, LockLevel & lockLevel) const;
    void getLockRequestInfo(vector<ClusterShardId> & shardIdList, NodeOperationId & shardIdListLockHolder,
    		LockLevel & shardIdListLockLevel) const;

    vector<string> & getPrimaryKeys(){
    	return primaryKeys;
    }
    NodeOperationId getWriterAgent() const{
    	return writerAgent;
    }

    bool isReleaseRequest() const{
    	return releaseRequestFlag;
    }

    bool isBlocking() const{
    	return blocking;
    }

    LockRequestType getType(){
    	return lockRequestType;
    }


    void getInvolvedNodes(vector<NodeId> & participants){
    	participants.clear();
    	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();
    	switch (lockRequestType) {
    	case LockRequestType_Copy:
    	{
    		// only those nodes that have a replica of this partition
    		writeview->getPatitionInvolvedNodes(srcShardId, participants);
    		break;
    	}
    	case LockRequestType_Move:
    	{
    		writeview->getPatitionInvolvedNodes(shardId, participants);
    		break;
    	}
    	case LockRequestType_PrimaryKey:
    		//TODO participants must be given from outside because it must work based on readview
    	case LockRequestType_Metadata:
    	{
    		writeview->getArrivedNodes(participants, true);
    		break;
    	}
    	case LockRequestType_GeneralPurpose:
    	{
    		writeview->getPatitionInvolvedNodes(generalPurposeShardId, participants);
    		break;
    	}
    	}
    }

private:

    bool releaseRequestFlag;

    LockRequestType lockRequestType;
    const bool blocking;
	/*
	 * in case of LockRequestType_Copy
	 */
	ClusterShardId srcShardId;
	ClusterShardId destShardId;
	NodeOperationId copyAgent;

	/*
	 * in case of LockRequestType_Move
	 */
	ClusterShardId shardId;
	NodeOperationId srcMoveAgent;
	NodeOperationId destMoveAgent;

	/*
	 * in case of LockRequestType_Metadata
	 */
	NodeOperationId newNodeOpId;
	LockLevel metadataLockLevel;
	vector<NodeId> listOfOlderNodes;

	/*
	 * in case of LockRequestType_PrimaryKey
	 */
	vector<string> primaryKeys;
	NodeOperationId writerAgent;
	ClusterPID pid;

	/*
	 * in case of LockRequestType_GeneralPurpose
	 */
	ClusterShardId generalPurposeShardId;
	NodeOperationId generalPurposeAgent;
	LockLevel generalPurposeLockLevel;

	/*
	 * Batch of cluster shard ids (they are not necessarily from the same core of partition)
	 */
	vector<ClusterShardId> shardIdList;
	NodeOperationId shardIdListLockHolder;
	LockLevel shardIdListLockLevel;



	// members after this line must not be serialized



	//////////////////////// Sub Classes ///////////////////////////
public:


	class ACK : public ShardingNotification{
	public:
		ACK(){
			granted = false;
			indexOfLastGrantedItem = 0;
		};
		ACK(bool grantedFlag);

		static bool resolveMessage(Message * msg, NodeId sendeNode);
		static void resolveNotif(ACK * ack);

		ShardingMessageType messageType() const;
		void * serialize(void * buffer) const;
		unsigned getNumberOfBytes() const;
		void * deserialize(void * buffer);

	    bool operator==(const LockingNotification::ACK & right);

		bool isGranted() const;
	    void setGranted(bool granted);

	    unsigned getIndexOfLastGrantedItem() const;
	    void setIndexOfLastGrantedItem(const unsigned index);

	private:
	    bool granted;
	    unsigned indexOfLastGrantedItem;
	};

};

}
}

#endif // __SHARDING_SHARDING_LOCKING_NOTIFICATION_H__
