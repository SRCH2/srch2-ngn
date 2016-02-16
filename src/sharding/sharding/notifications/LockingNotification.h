/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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


class Transaction;

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

	bool resolveNotification(SP(ShardingNotification) _notif);
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer);
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

    vector<string> & getPrimaryKeys();
    NodeOperationId getWriterAgent() const;

    bool isReleaseRequest() const;

    bool isBlocking() const;

    LockRequestType getType() const;


    void getInvolvedNodes(SP(Transaction) sp , vector<NodeId> & participants) const;

private:

    LockRequestType lockRequestType;
    bool releaseRequestFlag;
    bool blocking;
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


		bool resolveNotification(SP(ShardingNotification) _notif);

		ShardingMessageType messageType() const;
		void * serializeBody(void * buffer) const;
		unsigned getNumberOfBytesBody() const;
		void * deserializeBody(void * buffer);

	    bool operator==(const LockingNotification::ACK & right);

		bool isGranted() const;
	    void setGranted(bool granted);
	    bool hasResponse() const {
				return true;
		}
	    int getIndexOfLastGrantedItem() const;
	    void setIndexOfLastGrantedItem(const int index);

	private:
	    bool granted;
	    int32_t indexOfLastGrantedItem;
	};

};

}
}

#endif // __SHARDING_SHARDING_LOCKING_NOTIFICATION_H__
