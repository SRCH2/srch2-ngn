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


    // For SHM
	ShardingCommitMessageType,
	ShardingCommitACKMessageType,
	ShardingProposalMessageType,
	ShardingProposalOkMessageType,
	ShardingProposalNoMessageType,
	ShardingLockMessageType,
	ShardingLockGrantedMessageType,
	ShardingLockRejectedMessageType,
	ShardingLockReleasedMessageType,
	ShardingLockRvReleasesMessageType,
	ShardingNewNodeWelcomeMessageType,
	ShardingNewNodeBusyMessageType,
	ShardingNewNodeNewHostMessageType,
	ShardingNewNodeShardRequestMessageType,
	ShardingNewNodeShardOfferMessageType,
	ShardingNewNodeShardsReadyMessageType,
	ShardingNewNodeJoinPermitMessageType,
	ShardingCopyToMeMessageType,
	ShardingMoveToMeMessageType
};


//Adding portions of new header file, beginning from here
enum ShardState {
	SHARDSTATE_NULL,
	SHARDSTATE_ALLOCATED,  // must have a valid node
	SHARDSTATE_UNALLOCATED,
	SHARDSTATE_MIGRATING,
	SHARDSTATE_INDEXING,
	// these are the constants that DPEx, DPInt, RM and MM use
	SHARDSTATE_REGISTERED,
	SHARDSTATE_NOT_COMMITTED,
	SHARDSTATE_COMMITTED,

	//
	SHARDSTATE_UNASSIGNED,
	SHARDSTATE_PENDING,
	SHARDSTATE_READY

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
	EndOfPortType // stop value - not valid (also used to indicate all/default ports)
};

enum CLUSTERSTATE {
	CLUSTERSTATE_GREEN,  // all nodes are green
	CLUSTERSTATE_RED,    // all nodes are red ..possible ?
	CLUSTERSTATE_YELLOW  // not all nodes are green.
};

enum NotificationType{
	NotificationType_SM_NodeArrival,
	NotificationType_SM_NodeFailure,
	NotificationType_MM_Failed,
	NotificationType_MM_Finished,
	NotificationType_DP_UpdateLoads,
	NotificationType_Sharding_Commit_ShardAssignChange,
	NotificationType_Sharding_Commit_ShardCopyChange,
	NotificationType_Sharding_Commit_ShardMoveChange,
	NotificationType_Sharding_Commit_ShardLoadChange,
	NotificationType_Sharding_Commit_ACK,
	NotificationType_Sharding_Proposal_ShardAssignChange,
	NotificationType_Sharding_Proposal_ShardMoveChange,
	NotificationType_Sharding_Proposal_ShardLoadChange,
	NotificationType_Sharding_Proposal_OK,
	NotificationType_Sharding_Proposal_NO,
	NotificationType_Sharding_Lock_S_Lock,
	NotificationType_Sharding_Lock_S_UnLock,
	NotificationType_Sharding_Lock_X_Lock,
	NotificationType_Sharding_Lock_X_UnLock,
	NotificationType_Sharding_Lock_GRANTED,
	NotificationType_Sharding_Lock_REJECTED,
	NotificationType_Sharding_Lock_RELEASED,
	NotificationType_Sharding_Lock_RV_RELEASED,
	NotificationType_Sharding_NewNode_Welcome,
	NotificationType_Sharding_NewNode_Busy,
	NotificationType_Sharding_NewNode_NewHost,
	NotificationType_Sharding_NewNode_ShardRequest,
	NotificationType_Sharding_NewNode_ShardOffer,
	NotificationType_Sharding_NewNode_ShardsReady,
	NotificationType_Sharding_NewNode_JoinPermit,
	NotificationType_Sharding_CopyToMe,
	NotificationType_Sharding_MoveToMe,
	NotificationType_Default
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
