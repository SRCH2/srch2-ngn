#ifndef __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
#define __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__

namespace srch2 {
namespace httpwrapper {

///////////////////////////////////////// TEMPORARY ////////////////////////////////////////////
typedef unsigned TimeoutValue;
typedef unsigned NodeId;

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

    // For SM
    HeartBeatMessageType,
    ClientStatusMessageType,
    ClusterStatusMessageType,
    LeaderElectionProposalMessageType,
    LeaderElectionAckMessageType,
    LeaderElectionDenyMessageType
};

enum RoutingManagerAPIReturnType{
	RoutingManagerAPIReturnTypeSuccess,
	RoutingManagerAPIReturnTypeAllNodesDown
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
