#ifndef __SHARDING_ROUTING_MESSAGE_H_
#define __SHARDING_ROUTING_MESSAGE_H_

#include "configuration/ShardingConstants.h"
#include "configuration/ConfigManager.h"

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
   MessageTime_t inital_time; //used by response type messages
   char buffer[0];
};

}
}

#endif // __SHARDING_ROUTING_MESSAGE_H_
