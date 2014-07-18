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
    ResetLogCommandMessageType, // -> for ResetLogCommandInput (used for resetting log)
    StatusMessageType, // -> for CommandStatus object (object returned from insert, delete, update)

    // For SHM
    ShardManagerRequestReportMessageType,

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
    MigrationCompleteAckMessage
};

enum RoutingManagerAPIReturnType{
	RoutingManagerAPIReturnTypeSuccess,
	RoutingManagerAPIReturnTypeAllNodesDown
};

//Adding portions of new header file, beginning from here
enum ShardState {
	SHARDSTATE_ALLOCATED,  // must have a valid node
	SHARDSTATE_UNALLOCATED,
	SHARDSTATE_MIGRATING,
	SHARDSTATE_INDEXING,
	// these are the constants that DPEx, DPInt, RM and MM use
	SHARDSTATE_REGISTERED,
	SHARDSTATE_NOT_COMMITTED,
	SHARDSTATE_COMMITTED

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
	EndOfPortType // stop value - not valid (also used to indicate all/default ports)
};

enum CLUSTERSTATE {
	CLUSTERSTATE_GREEN,  // all nodes are green
	CLUSTERSTATE_RED,    // all nodes are red ..possible ?
	CLUSTERSTATE_YELLOW  // not all nodes are green.
};


enum TransactionStatus{
	ShardManager_Transaction_OnGoing,
	ShardManager_Transaction_Aborted,
	ShardManager_Transaction_Committed,
	// and if no status is found in the map for a transaction it's completed for this node.
	ShardManager_Transaction_Completed
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
