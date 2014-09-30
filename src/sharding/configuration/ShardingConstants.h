#ifndef __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
#define __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__

namespace srch2 {
namespace httpwrapper {

///////////////////////////////////////// TEMPORARY ////////////////////////////////////////////
typedef unsigned TimeoutValue;
typedef unsigned NodeId;
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
    SearchCommandMessageType, // -> for LogicalPlan object
    SearchResultsMessageType, // -> for SerializedQueryResults object
    InsertUpdateCommandMessageType, // -> for Record object (used for insert and update)
    DeleteCommandMessageType, // -> for DeleteCommandInput object (used for delete)
    SerializeCommandMessageType, // -> for SerializeCommandInput object (used for serializing index and records)
    GetInfoCommandMessageType, // -> for GetInfoCommandInput object (used for getInfo)
    GetInfoResultsMessageType, // -> for GetInfoResults object
    CommitCommandMessageType, // -> for CommitCommandInput object
    MergeCommandMessageType, // -> for MergeCommandInput object
    ResetLogCommandMessageType, // -> for ResetLogCommandInput (used for resetting log)
    StatusMessageType, // -> for CommandStatus object (object returned from insert, delete, update)


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
    ShardingNewNodeLockMessageType,
    ShardingNewNodeLockACKMessageType,
    ShardingMoveToMeMessageType,
    ShardingMoveToMeStartMessageType,
    ShardingMoveToMeACKMessageType,
    ShardingMoveToMeFinishMessageType,
    ShardingMoveToMeAbortMessageType,
    ShardingNewNodeReadMetadataRequestMessageType,
    ShardingNewNodeReadMetadataReplyMessageType,
    ShardingLockMessageType,
    ShardingLockACKMessageType,
    ShardingLockRVReleasedMessageType,
    ShardingLoadBalancingReportMessageType,
    ShardingLoadBalancingReportRequestMessageType,
    ShardingCopyToMeMessageType,
    ShardingCommitMessageType,
    ShardingCommitACKMessageType,
    ShardingSaveDataMessageType,
    ShardingSaveDataACKMessageType,
    ShardingSaveMetadataMessageType,
    ShardingSaveMetadataACKMessageType,
    ShardingMergeMessageType,
    ShardingShutdownMessageType,
    ShardingMergeACKMessageType,
    // just notifications
    ShardingMMNotificationMessageType,
    ShardingNodeFailureNotificationMessageType
};


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
	SearchPort,
	SuggestPort,
	InfoPort,
	DocsPort,
	UpdatePort,
	SavePort,
	ExportPort,
	ResetLoggerPort,
	CommitPort,
	MergePort,
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

enum ResourceLockType{
	ResourceLockType_S,
	ResourceLockType_X,
	ResourceLockType_U
};

enum ResourceLockRequestType{
	ResourceLockRequestTypeLock,
	ResourceLockRequestTypeRelease,
	ResourceLockRequestTypeUpgrade,
	ResourceLockRequestTypeDowngrade
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


}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
