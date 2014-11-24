
#include "ShardingConstants.h"
#include "boost/assign/list_of.hpp"
#include "core/util/Assert.h"
using namespace srch2::instantsearch;
namespace srch2 {
namespace httpwrapper {

std::map<ShardingMessageType,std::string> __shardingMessageTypeEnumNames = boost::assign::map_list_of
	(NULLType, "NULLType")
	(SearchCommandMessageType, "SearchCommandMessageType")
	(SearchResultsMessageType, "SearchResultsMessageType")
	(DeleteCommandMessageType, "DeleteCommandMessageType")
	(GetInfoCommandMessageType, "GetInfoCommandMessageType")
	(GetInfoResultsMessageType, "GetInfoResultsMessageType")
	(StatusMessageType, "StatusMessageType")
	(RecordWriteCommandMessageType, "RecordWriteCommandMessageType")
	// For SM
	(HeartBeatMessageType, "HeartBeatMessageType")
	(ClientStatusMessageType, "ClientStatusMessageType")
	(ClusterStatusMessageType, "ClusterStatusMessageType")
	(LeaderElectionProposalMessageType, "LeaderElectionProposalMessageType")
	(LeaderElectionAckMessageType, "LeaderElectionAckMessageType")
	(LeaderElectionDenyMessageType, "LeaderElectionDenyMessageType")
	(NewNodeNotificationMessageType, "NewNodeNotificationMessageType")
	(ClusterInfoRequestMessageType, "ClusterInfoRequestMessageType")
	(ClusterInfoReplyMessageType, "ClusterInfoReplyMessageType")
	(ClusterUpdateMessageType, "ClusterUpdateMessageType")
	// for MM
	(MigrationInitMessage, "MigrationInitMessage")
	(MigrationInitAckMessage, "MigrationInitAckMessage")
	(MigrationComponentBeginMessage, "MigrationComponentBeginMessage")
	(MigrationComponentBeginAckMessage, "MigrationComponentBeginAckMessage")
	(MigrationComponentEndAckMessage, "MigrationComponentEndAckMessage")
	(MigrationCompleteAckMessage, "MigrationCompleteAckMessage")
	// For SHM
	(ShardingMoveToMeMessageType, "ShardingMoveToMeMessageType")
	(ShardingMoveToMeACKMessageType, "ShardingMoveToMeACKMessageType")
	(ShardingMoveToMeCleanupMessageType, "ShardingMoveToMeCleanupMessageType")
	(ShardingNewNodeReadMetadataRequestMessageType, "ShardingNewNodeReadMetadataRequestMessageType")
	(ShardingNewNodeReadMetadataReplyMessageType, "ShardingNewNodeReadMetadataReplyMessageType")
	(ShardingLockMessageType, "ShardingLockMessageType")
	(ShardingLockACKMessageType, "ShardingLockACKMessageType")
	(ShardingLockRVReleasedMessageType, "ShardingLockRVReleasedMessageType")
	(ShardingLoadBalancingReportMessageType, "ShardingLoadBalancingReportMessageType")
	(ShardingLoadBalancingReportRequestMessageType, "ShardingLoadBalancingReportRequestMessageType")
	(ShardingCopyToMeMessageType, "ShardingCopyToMeMessageType")
	(ShardingCopyToMeACKMessageType, "ShardingCopyToMeACKMessageType")
	(ShardingShardCommandMessageType, "ShardingShardCommandMessageType")
	(ShardingAclAttrReadMessageType, "ShardingAclAttrReadMessageType")
	(ShardingAclAttrReadACKMessageType, "ShardingAclAttrReadACKMessageType")
	(ShardingAclAttrReplaceMessageType, "ShardingAclAttrReplaceMessageType")
	(ShardingAclAttrReplaceACKMessageType, "ShardingAclAttrReplaceACKMessageType")
	(ShardingCommitMessageType, "ShardingCommitMessageType")
	(ShardingCommitACKMessageType, "ShardingCommitACKMessageType")
	(ShardingShutdownMessageType, "ShardingShutdownMessageType")
	(ShardingRecordLockMessageType, "ShardingRecordLockMessageType")
	(ShardingRecordLockACKMessageType, "ShardingRecordLockACKMessageType")
	(InsertUpdateCommandMessageType, "InsertUpdateCommandMessageType")
	(ShardingWriteCommand2PCMessageType, "ShardingWriteCommand2PCMessageType")
	(ShardingWriteCommand2PCACKMessageType, "ShardingWriteCommand2PCACKMessageType")
	(ShardingSearchCommandMessageType, "ShardingSearchCommandMessageType")
	(ShardingSearchResultsMessageType, "ShardingSearchResultsMessageType")
	// just notifications
	(ShardingMMNotificationMessageType, "ShardingMMNotificationMessageType")
    (ShardingNodeFailureNotificationMessageType, "ShardingNodeFailureNotificationMessageType");

const char * getShardingMessageTypeStr(ShardingMessageType shardingMessageType){
	   if(__shardingMessageTypeEnumNames.find(shardingMessageType) == __shardingMessageTypeEnumNames.end()){
		   ASSERT(false);
		   return "TYPE NOT FOUND";
	   }
	   return __shardingMessageTypeEnumNames.find(shardingMessageType)->second.c_str();
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
