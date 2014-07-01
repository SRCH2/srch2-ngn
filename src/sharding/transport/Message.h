#ifndef __SHARDING_TRANSPORT_MESSAGE_H__
#define __SHARDING_TRANSPORT_MESSAGE_H__

#include "src/sharding/configuration/ShardingConstants.h"
#include "src/sharding/configuration/ConfigManager.h"

namespace srch2 {
namespace httpwrapper {

typedef unsigned MessageID_t;

const char MSG_LOCAL_MASK = 0x1;        // 00000001
const char MSG_DISCOVERY_MASK = 0x2;    // 00000010
const char MSG_DP_INTERNAL_MASK = 0x4;  // 00000100
const char MSG_SHM_INTERNAL_MASK = 0x4; // 00001000
const char MSG_DP_REPLY_MASK = 0x10;// 00010000
const char MSG_SHM_REPLY_MASK = 0x20;   // 00100000

class Message {

public:

   //helper Functions

   bool isSMRelated(){
	 return (mask == 0);
   }

   bool isLocal(){
     return mask & MSG_LOCAL_MASK;
   }
   bool isReply() {
     return isDPReply() || isSHMReply();
   }

   bool isDPReply() {
     return mask & MSG_DP_REPLY_MASK;
   }

   bool isSHMReply() {
     return mask & MSG_SHM_REPLY_MASK;
   }
   bool isInternal() {
     return isDPInternal() || isSHMInternal();
   }
   bool isDPInternal() {
     return mask & MSG_DP_INTERNAL_MASK;
   }
   bool isSHMInternal() {
     return mask & MSG_SHM_INTERNAL_MASK;
   }
   bool isDiscovery() {
     return mask & MSG_DISCOVERY_MASK;
   }

   Message * setLocal(){
	   mask |= MSG_LOCAL_MASK;
	   return this;
   }
   Message * setDPReply(){
	   mask |= MSG_DP_REPLY_MASK;
	   return this;
   }
   Message * setSHMReply(){
	   mask |= MSG_SHM_REPLY_MASK;
	   return this;
   }
   Message * setDPInternal(){
	   mask |= MSG_DP_INTERNAL_MASK;
	   return this;
   }
   Message * setSHMInternal(){
	   mask |= MSG_SHM_INTERNAL_MASK;
	   return this;
   }
   Message * setDiscoveryMask(){
	   mask |= MSG_DISCOVERY_MASK;
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
   ShardId getDestinationShardId(){
	   return destinationShardId;
   }
   void setDestinationShardId(const ShardId& destShardId){
	   this->destinationShardId = destShardId;
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



private:
   ShardingMessageType shardingMessageType;
   char mask;
   ShardId destinationShardId;
   unsigned bodySize;
   MessageID_t id;
   MessageID_t requestMessageId; //used by response type messages
   char body[0];
};

}
}

#endif // __SHARDING_ROUTING_MESSAGE_H__
