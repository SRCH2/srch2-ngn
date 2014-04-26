#ifndef __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
#define __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__

namespace srch2 {
namespace httpwrapper {

enum ShardingMessageType{
    SearchCommandMessageType, // -> for LogicalPlan object
    SearchResultsMessageType, // -> for SerializedQueryResults object
    InsertUpdateMessageType, // -> for Record object (used for insert and update)
    DeleteInfoMessageType, // -> for DeleteCommandInput object (used for delete)
    SerializeInfoMessageType, // -> for SerializeCommandInput object (used for serializing index and records)
    GetInfoMessageType, // -> for GetInfoCommandInput object (used for getInfo)
    GetInfoResultsMessageType, // -> for GetInfoResults object
    CommitCommandMessageType, // -> for CommitCommandInput object
    ResettingLogMessageType, // -> for ResetLogCommandInput (used for resetting log)
    StatusMessageType // -> for CommandStatus object (object returned from insert, delete, update)
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
