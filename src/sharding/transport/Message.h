#ifndef __SHARDING_TRANSPORT_MESSAGE_H__
#define __SHARDING_TRANSPORT_MESSAGE_H__

#include "configuration/ShardingConstants.h"
#include "configuration/ConfigManager.h"

namespace srch2 {
namespace httpwrapper {

typedef unsigned MessageTime_t;

const char LOCAL_MASK = 0x2;
const char REPLY_MASK = 0x1;
const char INTERNAL_MASK = 0x4;
const char NOREPLY_MASK = 0x8;

class Message {
  //TODO: magic number
public:

   //helper Functions
   bool isLocal(){
     return mask & LOCAL_MASK;
   }
   bool isReply() {
     return mask & REPLY_MASK;
   }
   bool isInternal() {
     return mask & INTERNAL_MASK;
   }
   bool isNoReply() {
	   return mask & NOREPLY_MASK;
   }
   Message * setLocal(){
	   mask |= LOCAL_MASK;
	   return this;
   }
   Message * setReply(){
	   mask |= REPLY_MASK;
	   return this;
   }
   Message * setInternal(){
	   mask |= INTERNAL_MASK;
	   return this;
   }
   Message * setNoReply() {
	   mask |= NOREPLY_MASK;
	   return this;
   }
   unsigned getBodySize(){
	   return this->bodySize;
   }
   void setTime(MessageTime_t time){
	   this->time = time;
   }
   MessageTime_t getTime(){
	   return this->time;
   }
   void setInitialTime(MessageTime_t time){
	   this->initialTime = time;
   }
   MessageTime_t getInitialTime(){
	   return this->initialTime;
   }
   void setBodyAndBodySize(void * src, unsigned bodySize){
	   setBodySize(bodySize);
	   memcpy(this->body, (char *)src, this->getBodySize());
   }
   ShardingMessageType getType(){
	   return this->type;
   }
   void setType(ShardingMessageType type){
	   this->type = type;
   }
   ShardId getDestinationShardId(){
	   return destinationShard;
   }
   void setDestinationShardId(ShardId destShardId){
	   this->destinationShard = destShardId;
   }
   void setBodySize(unsigned bodySize){
	   this->bodySize = bodySize;
   }
   static Message * getMessagePointerFromBodyPointer( void * bodyPointer){
	   return (Message *)((char *)bodyPointer - sizeof(Message));
   }
   static char * getBodyPointerFromMessagePointer(Message * messagePointer){
	   return messagePointer->body;
   }



private:
   ShardingMessageType type;
   char mask;
   ShardId destinationShard;
   unsigned bodySize; //size of buffer -> change name to bodySize?
   MessageTime_t time;
   MessageTime_t initialTime; //used by response type messages
   char body[0];
};

}
}

#endif // __SHARDING_ROUTING_MESSAGE_H_
