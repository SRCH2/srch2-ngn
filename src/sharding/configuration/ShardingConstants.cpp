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

#include "ShardingConstants.h"
#include "boost/assign/list_of.hpp"
#include "core/util/Assert.h"
using namespace srch2::instantsearch;
namespace srch2 {
namespace httpwrapper {

std::map<ShardingMessageType,std::string> __shardingMessageTypeEnumNames = boost::assign::map_list_of
	(NULLType, "NULLType")
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
		case ShardingTransactionType_DebugCollectInfo:
			return "debug-info-collector";
	}
	return "unknown-command";
}


}
}
