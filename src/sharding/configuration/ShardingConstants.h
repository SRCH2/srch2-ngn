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

    ShardingCommitMessageType,
    ShardingCommitACKMessageType,
    ShardingShutdownMessageType,

    ShardingRecordLockMessageType,
    ShardingRecordLockACKMessageType,

    InsertUpdateCommandMessageType,

    // just notifications
    ShardingMMNotificationMessageType,
    ShardingNodeFailureNotificationMessageType
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
//	GetInfoAggregateCriterion_Node, // {"details" : [{""},{},{}]}
//	GetInfoAggregateCriterion_Core,
//	GetInfoAggregateCriterion_Core_Partition,
	GetInfoAggregateCriterion_Core_Shard
	// TODO : ...
};


enum ClusterRecordOperation_Type{
	Insert_ClusterRecordOperation_Type,
	Update_ClusterRecordOperation_Type,
	Delete_ClusterRecordOperation_Type
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

//enum RecordWriteCommandCode{ // TODO : Delete if system is stable.
//	RecordWriteCommand_InsertUpdate,
//	RecordWriteCommand_Delete,
//	RecordWriteCommand_AclAttrAdd,
//	RecordWriteCommand_AclAttrAppend,
//	RecordWriteCommand_AclAttrDelete
//};

enum ShardingTransactionType{
	ShardingTransactionType_Loadbalancing,
	ShardingTransactionType_NodeJoin,
	ShardingTransactionType_ShardCommandCode,
	ShardingTransactionType_InsertUpdateCommand,
	ShardingTransactionType_Shutdown
};

const char * getTransTypeStr(ShardingTransactionType type);


typedef unsigned TRANS_ID;
const TRANS_ID TRANS_ID_NULL = 0;

}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
