#ifndef __MESSAGE_BUFFER_H__
#define __MESSAGE_BUFFER_H__

#include "Message.h"

namespace srch2 {
namespace httpwrapper {
// TODO : let's please move this class to RouteMap.h (where it is used) and remove this file

class MessageBuffer {
  public:
    Message* msg;
    bool lock;
    int readCount;
    MessageBuffer() : msg(NULL), lock(false), readCount(0) {}
};
}}



#endif
