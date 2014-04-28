#ifndef __SHARDING_ROUTING_MESSAGE_H_
#define __SHARDING_ROUTING_MESSAGE_H_

#include "sharding/configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {



struct Message {
   ShardingMessageType type;
   char mask;
   ShardId shard;
   unsigned totalSize;
   unsigned time;
   char buffer[0];
};

class MessageAllocator; //: std::allocator<char>;


}
}

#endif // __SHARDING_ROUTING_MESSAGE_H_
