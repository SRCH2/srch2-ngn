
#include "ShardingConstants.h"

namespace srch2 {
namespace httpwrapper {

const char * getShardingMessageTypeStr(ShardingMessageType shardingMessageType){
	   switch (shardingMessageType) {
	   case ShardingMoveToMeMessageType:
		   return "ShardingMoveToMeMessageType";
	   case ShardingMoveToMeACKMessageType:
		   return "ShardingMoveToMeACKMessageType";
	   case ShardingMoveToMeCleanupMessageType:
		   return "ShardingMoveToMeCleanupMessageType";
	   case ShardingNewNodeReadMetadataRequestMessageType:
		   return "ShardingNewNodeReadMetadataRequestMessageType";
	   case ShardingNewNodeReadMetadataReplyMessageType:
		   return "ShardingNewNodeReadMetadataReplyMessageType";
	   case ShardingLockMessageType:
		   return "ShardingLockMessageType";
	   case ShardingLockACKMessageType:
		   return "ShardingLockACKMessageType";
	   case ShardingLockRVReleasedMessageType:
		   return "ShardingLockACKMessageType";
	   case ShardingLoadBalancingReportMessageType:
		   return "ShardingLoadBalancingReportMessageType";
	   case ShardingLoadBalancingReportRequestMessageType:
		   return "ShardingLoadBalancingReportRequestMessageType";
	   case ShardingCopyToMeMessageType:
		   return "ShardingCopyToMeMessageType";
	   case ShardingCommitMessageType:
		   return "ShardingCommitMessageType";
	   case ShardingCommitACKMessageType:
		   return "ShardingCommitACKMessageType";
	   case ShardingMMNotificationMessageType:
		   return "ShardingMMNotificationMessageType";
	   case ShardingNodeFailureNotificationMessageType:
		   return "ShardingNodeFailureNotificationMessageType";
	   case ShardingWriteCommand2PCMessageType:
		   return "ShardingWriteCommand2PCMessageType";
	   case ShardingWriteCommand2PCACKMessageType:
		   return "ShardingWriteCommand2PCACKMessageType";
	   default:
		   return "Message Does Not Have Description";
		   break;
	   }
	   return "";
}

const char * getTransTypeStr(ShardingTransactionType type){
	switch (type) {
		case ShardingTransactionType_Loadbalancing:
			return "load-balancer";
		case ShardingTransactionType_NodeJoin:
			return "node-joiner";
		case ShardingTransactionType_ShardCommandCode:
			return "shard-command";
		case ShardingTransactionType_InsertUpdateCommand:
			return "insert-update-command";
		case ShardingTransactionType_Shutdown:
			return "shutdown-command";
		case ShardingTransactionType_AttributeAclCommandCode:
			return "attribute-acl-command";
		case ShardingTransactionType_RecordAclCommandCode:
			return "record-acl-command";
		case ShardingTransactionType_ReadCommand:
			return "read-command";
	}
	return "unknown-command";
}


}
}
