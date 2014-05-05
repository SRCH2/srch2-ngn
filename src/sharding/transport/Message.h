#ifndef __SHARDING_ROUTING_MESSAGE_H_
#define __SHARDING_ROUTING_MESSAGE_H_

#include "sharding/configuration/ShardingConstants.h"
#include "sharding/configuration/ConfigManager.h"

namespace srch2 {
namespace httpwrapper {

typedef unsigned MessageTime_t;

struct Message {
  //TODO: magic number
   ShardingMessageType type;
   char mask;
   ShardId shard;
   unsigned bodySize; //size of buffer -> change name to bodySize?
   MessageTime_t time;
   MessageTime_t initial_time; //used by response type messages
   char buffer[0];

   //helper Functions
   bool isLocal(){
     return mask & 0x2;
   }

   bool isReply() {
     return mask & 0x1;
   }
};

}
}

#endif // __SHARDING_ROUTING_MESSAGE_H_
