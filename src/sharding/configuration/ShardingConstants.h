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
#ifndef __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
#define __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
#include <map>
#include <string>
#include "inttypes.h"
namespace srch2 {
namespace httpwrapper {


///////////////////////////////////////// TEMPORARY ////////////////////////////////////////////
typedef unsigned TimeoutValue;
typedef uint32_t NodeId;
// zero and negative numbers are reserved for error.
// positive numbers are used for handles
// 0 : not found
// -1 : uninitialized
// -2 : not ready (shard not in Allocated state)
class SynchronizationManager{

};


////////////////////////////////////////////////////////////////////////////////////////////////


enum ShardingMessageType{
    NULLType,
    ShardingSearchCommandMessageType,
    ShardingSearchResultsMessageType,
    DeleteCommandMessageType, // -> for DeleteCommandInput object (used for delete)
    GetInfoCommandMessageType,
    GetInfoResultsMessageType,
    StatusMessageType, // -> for CommandStatus object (object returned from insert, delete, update)
    RecordWriteCommandMessageType,



    // For SM
    HeartBeatMessageType,
    ClientStatusMessageType,
    ClusterStatusMessageType,
    LeaderElectionProposalMessageType,
    LeaderElectionAckMessageType,
    LeaderElectionDenyMessageType,
    NewNodeNotificationMessageType,
    ClusterInfoRequestMessageType,
    ClusterInfoReplyMessageType,
    ClusterUpdateMessageType,

    // for MM
    MigrationInitMessage,
    MigrationInitAckMessage,
    MigrationComponentBeginMessage,
    MigrationComponentBeginAckMessage,
    MigrationComponentEndAckMessage,
    MigrationCompleteAckMessage,

    // For SHM
    ShardingMoveToMeMessageType,
    ShardingMoveToMeACKMessageType,
    ShardingMoveToMeCleanupMessageType,
    ShardingNewNodeReadMetadataRequestMessageType,
    ShardingNewNodeReadMetadataReplyMessageType,
    ShardingLockMessageType,
    ShardingLockACKMessageType,
    ShardingLockRVReleasedMessageType,
    ShardingLoadBalancingReportMessageType,
    ShardingLoadBalancingReportRequestMessageType,
    ShardingCopyToMeMessageType,
    ShardingCopyToMeACKMessageType,
    ShardingShardCommandMessageType,
    ShardingAclAttrReadMessageType,
    ShardingAclAttrReadACKMessageType,
    ShardingAclAttrReplaceMessageType,
    ShardingAclAttrReplaceACKMessageType,

    ShardingCommitMessageType,
    ShardingCommitACKMessageType,
    ShardingShutdownMessageType,

    ShardingRecordLockMessageType,
    ShardingRecordLockACKMessageType,

    InsertUpdateCommandMessageType,
    ShardingWriteCommand2PCMessageType,
    ShardingWriteCommand2PCACKMessageType,

    // just notifications
    ShardingMMNotificationMessageType,
    ShardingNodeFailureNotificationMessageType,
    ShardingTimeoutNotificationMessageType,

    ShardingMessageTypeFirst = NULLType,
    ShardingMessageTypeLast = ShardingNodeFailureNotificationMessageType
};


const char * getShardingMessageTypeStr(ShardingMessageType shardingMessageType);

//Adding portions of new header file, beginning from here
enum ShardState {
	SHARDSTATE_UNASSIGNED,
	SHARDSTATE_PENDING,
	SHARDSTATE_READY
};

enum ShardingNodeState{
	ShardingNodeStateNotArrived,
	ShardingNodeStateArrived,
	ShardingNodeStateFailed
};

// enum to allow loop iteration over listening ports
enum PortType_t {
	SearchPort = 0,
	SuggestPort,
	InfoPort,
	DocsPort,
	UpdatePort,
	SavePort,
	ExportPort,
	ResetLoggerPort,
	CommitPort,
	MergePort,
    AttributeAclAdd,
    AttributeAclAppend,
    AttributeAclDelete,
    RecordAclAdd,
    RecordAclAppend,
    RecordAclDelete,
    BulkLoadRecords,
    BulkLoadRecordAcl,
    BulkLoadAttributeAcl,
    UserFeedback,
    //--------------------------------
	GlobalPortsStart, // used in portNameMap
	InfoPort_Nodes_NodeID,
	InfoPort_Cluster_Stats,
	DebugStatsPort,
    SearchAllPort,
	ShutdownPort,
	NodeShutdownPort,
	EndOfPortType // stop value - not valid (also used to indicate all/default ports)
};

enum CLUSTERSTATE {
	CLUSTERSTATE_GREEN,  // all nodes are green
	CLUSTERSTATE_RED,    // all nodes are red ..possible ?
	CLUSTERSTATE_YELLOW  // not all nodes are green.
};

enum PartitionLockValue{
	PartitionLock_Locked,
	PartitionLock_Unlocked
};

enum ResourceLockRequestType{
	ResourceLockRequestTypeLock,
	ResourceLockRequestTypeRelease,
	ResourceLockRequestTypeUpgrade,
	ResourceLockRequestTypeDowngrade,
	RecordLockRequestType,
	RecordReleaseRequestType
};

enum MultipleResourceLockRequestType{
	MultipleResourceLockRequestTypeSerial, // used for lock and upgrade
	MultipleResourceLockRequestTypeParallel // used for release and downgrade
};

enum MetadataChangeType {
	ShardingChangeTypeNodeAdd,
	ShardingChangeTypeShardAssign,
	ShardingChangeTypeShardMove,
	ShardingChangeTypeLoadChange
};

enum OperationStateType{
	OperationStateType_LockSerial,
	OperationStateType_LockParallel,
	OperationStateType_Commit,
	OperationStateType_NewNode_Lock,
	OperationStateType_NewNode_Join,
	OperationStateType_NewNode_Finalize,
	OperationStateType_LoadBalancing_Start,
	OperationStateType_ShardAssign_Start,
	OperationStateType_ShardAssign_Commit,
	OperationStateType_ShardCopy_Start,
	OperationStateType_ShardMove_Start,
	OperationStateType_ShardMove_Transfer,
	OperationStateType_ShardMove_Commit,
	OperationStateType_ShardMove_Finalize,
	OperationStateType_NodeInitializer_NewNode_Init,
	OperationStateType_NodeInitializer_NewNode_Ready,
	OperationStateType_NodeInitializer_NewNode_Done,
	OperationStateType_NodeInitializer_HostNode_Wait,
	OperationStateType_NodeInitializer_HostNode_Offered,
	OperationStateType_NodeInitializer_HostNode_Updating,
	OperationStateType_NodeInitializer_OtherNodes_Wait,
	OperationStateType_NodeInitializer_OtherNodes_Recovery,
	OperationStateType_NodeInitializer_OtherNodes_Updating,
	OperationStateType_ShardCopy_Src_Copying,
	OperationStateType_ShardCopy_Dest_Proposed,
	OperationStateType_ShardCopy_Dest_Locking,
	OperationStateType_ShardCopy_Dest_Copying,
	OperationStateType_ShardCopy_Dest_Commiting,
	OperationStateType_ShardAssign_Proposed,
	OperationStateType_ShardAssign_Commiting,
	OperationStateType_ShardMove_Dest_Proposed,
	OperationStateType_ShardMove_Dest_Locking,
	OperationStateType_ShardMove_Dest_Moving,
	OperationStateType_ShardMove_Dest_Committing,
	OperationStateType_ShardMove_Src_Moving,
	OperationStateType_ShardMove_Src_Cleanup,
	OperationStateType_ShardMove_Src_Recovery,
	OperationStateType_ClusterExecuteOperation,
	OperationStateType_ClusterJoinOperation

};


enum GetInfoRequestType{
	GetInfoRequestType_
};

enum GetInfoAggregateCriterion{
	GetInfoAggregateCriterion_Core_Shard
	// TODO : ...
};


enum ClusterRecordOperation_Type{
	Insert_ClusterRecordOperation_Type,
	Update_ClusterRecordOperation_Type,
	Delete_ClusterRecordOperation_Type,
	AclRecordAdd_ClusterRecordOperation_Type,
	AclRecordAppend_ClusterRecordOperation_Type,
	AclRecordDelete_ClusterRecordOperation_Type,
	AclAttrReplace_ClusterRecordOperation_Type,
	AclAttrDelete_ClusterRecordOperation_Type,
	AclAttrAppend_ClusterRecordOperation_Type,
	Feedback_ClusterRecordOperation_Type
};


enum LockLevel{
	LockLevel_S,
	LockLevel_X
};

enum LockRequestType{
	LockRequestType_Copy,
	LockRequestType_Move,
	LockRequestType_Metadata,
	LockRequestType_PrimaryKey,
	LockRequestType_GeneralPurpose,
	LockRequestType_ShardIdList
};

enum ShardCommandCode{
	ShardCommandCode_SaveData_SaveMetadata,
	ShardCommandCode_SaveData,
	ShardCommandCode_SaveMetadata,
	ShardCommandCode_Export,
    ShardCommandCode_Commit,
    ShardCommandCode_Merge,
    ShardCommandCode_MergeSetOn,
    ShardCommandCode_MergeSetOff,
    ShardCommandCode_ResetLogger
};

enum ShardingTransactionType{
	ShardingTransactionType_Loadbalancing,
	ShardingTransactionType_NodeJoin,
	ShardingTransactionType_ShardCommandCode,
	ShardingTransactionType_AttributeAclCommandCode,
	ShardingTransactionType_RecordAclCommandCode,
	ShardingTransactionType_InsertUpdateCommand,
	ShardingTransactionType_ReadCommand,
	ShardingTransactionType_Shutdown,
	ShardingTransactionType_DebugCollectInfo,
	ShardingTransactionType_FeedbackCommandCode
};

const char * getTransTypeStr(ShardingTransactionType type);


typedef unsigned TRANS_ID;
const TRANS_ID TRANS_ID_NULL = 0;

}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
