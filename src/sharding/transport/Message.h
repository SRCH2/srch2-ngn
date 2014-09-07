#ifndef __SHARDING_TRANSPORT_MESSAGE_H__
#define __SHARDING_TRANSPORT_MESSAGE_H__

#include "src/sharding/configuration/ShardingConstants.h"
#include "src/sharding/configuration/ConfigManager.h"

namespace srch2 {
namespace httpwrapper {

typedef unsigned MessageID_t;

const char MSG_LOCAL_MASK = 0x1;        // 00000001
const char MSG_DISCOVERY_MASK = 0x2;    // 00000010
const char MSG_DP_REQUEST_MASK = 0x4;   // 00000100
const char MSG_DP_REPLY_MASK = 0x8;     // 00001000
const char MSG_SHARDING_MASK = 0x10;     // 00010000

class Message {

public:

   //helper Functions

   bool isSMRelated(){
	 return (mask == 0);
   }

   bool isLocal(){
     return mask & MSG_LOCAL_MASK;
   }
   bool isDPRequest() {
     return mask & MSG_DP_REQUEST_MASK;
   }
   bool isDPReply() {
     return mask & MSG_DP_REPLY_MASK;
   }
   bool isDiscovery() {
     return mask & MSG_DISCOVERY_MASK;
   }
   bool isSharding() {
     return mask & MSG_SHARDING_MASK;
   }
   Message * setLocal(){
	   mask |= MSG_LOCAL_MASK;
	   return this;
   }
   Message * setDPRequestMask(){
	   mask |= MSG_DP_REQUEST_MASK;
	   return this;
   }
   Message * setDPReplyMask(){
	   mask |= MSG_DP_REPLY_MASK;
	   return this;
   }
   Message * setDiscoveryMask(){
	   mask |= MSG_DISCOVERY_MASK;
	   return this;
   }
   Message * setShardingMask(){
	   mask |= MSG_SHARDING_MASK;
	   return this;
   }
   unsigned getBodySize(){
	   return this->bodySize;
   }
   void setMessageId(MessageID_t id){
	   this->id = id;
   }
   MessageID_t getMessageId(){
	   return this->id;
   }
   void setRequestMessageId(MessageID_t requestMessageId){
	   this->requestMessageId = requestMessageId;
   }
   MessageID_t getRequestMessageId(){
	   return this->requestMessageId;
   }
   void setBodyAndBodySize(void * src, unsigned bodySize){
	   setBodySize(bodySize);
	   memcpy(this->body, src, this->getBodySize());
   }
   ShardingMessageType getType(){
	   return this->shardingMessageType;
   }
   void setType(ShardingMessageType type){
	   this->shardingMessageType = type;
   }
   void setBodySize(unsigned bodySize){
	   this->bodySize = bodySize;
   }

   void setMask(char mask) { this->mask =  mask; }
   char * getMessageBody() { return this->body; }

   static Message * getMessagePointerFromBodyPointer( void * bodyPointer){
	   return (Message *)((char *)bodyPointer - sizeof(Message));
   }
   static char * getBodyPointerFromMessagePointer(Message * messagePointer){
	   return messagePointer->body;
   }

   string getDescription(){
	   switch (shardingMessageType) {
	   case ShardingNewNodeLockMessageType:
		   return "ShardingNewNodeLockMessageType";
	   case ShardingNewNodeLockACKMessageType:
		   return "ShardingNewNodeLockACKMessageType";
	   case ShardingMoveToMeMessageType:
		   return "ShardingMoveToMeMessageType";
	   case ShardingMoveToMeStartMessageType:
		   return "ShardingMoveToMeStartMessageType";
	   case ShardingMoveToMeACKMessageType:
		   return "ShardingMoveToMeACKMessageType";
	   case ShardingMoveToMeFinishMessageType:
		   return "ShardingMoveToMeFinishMessageType";
	   case ShardingMoveToMeAbortMessageType:
		   return "ShardingMoveToMeAbortMessageType";
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
	   default:
		   return "Message";
		   break;
	   }
	   ASSERT(false);
	   return "";
   }


private:
   ShardingMessageType shardingMessageType;
   char mask;
   unsigned bodySize;
   MessageID_t id;
   MessageID_t requestMessageId; //used by response type messages
   char body[0];
};

}
}

#endif // __SHARDING_ROUTING_MESSAGE_H__
