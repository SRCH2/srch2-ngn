#ifndef __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
#define __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__

namespace srch2 {
namespace httpwrapper {


/*
 * Mapping between ShardingMessageType and callback functions in DP and ResultsAggregator :
 *
 * SearchCommandMessageType  => DPInternalRequestHandler::internalSearchCommand
 * InsertUpdateMessageType => DPInternalRequestHandler::internalInsertUpdateCommand
 * DeleteInfoMessageType => DPInternalRequestHandler::internalDeleteCommand
 * SerializeInfoMessageType => DPInternalRequestHandler::internalSerializeCommand
 * GetInfoMessageType => DPInternalRequestHandler::internalGetInfoCommand
 * ResettingLogMessageType => DPInternalRequestHandler::internalResetLogCommand
 *
 * SearchResultsMessageType => ResultAggregatorAndPrint::aggregateSearchResults
 * StatusMessageType => ResultAggregatorAndPrint::aggregateCommandStatus
 *
 *
 */
enum ShardingMessageType{
    SearchCommandMessageType, // -> for LogicalPlan object
    SearchResultsMessageType, // -> for SerializedQueryResults object
    InsertUpdateMessageType, // -> for Record object (used for insert and update)
    DeleteInfoMessageType, // -> for DeleteCommandInput object (used for delete)
    SerializeInfoMessageType, // -> for SerializeCommandInput object (used for serializing index and records)
    GetInfoMessageType, // -> for GetInfoCommandInput object (used for getInfo)
    ResettingLogMessageType, // -> for ResetLogCommandInput (used for resetting log)
    StatusMessageType // -> for CommandStatus object (object returned from insert, delete, update)
};

}
}

#endif // __SHARDING_CONFIGURATION_SHARDING_CONSTANTS_H__
