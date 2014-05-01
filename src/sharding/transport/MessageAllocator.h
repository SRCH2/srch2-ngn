#ifndef __Message_Allocator_H__
#define __Message_Allocator_H__

#include <memory>
#include "Message.h"

namespace srch2 {
namespace httpwrapper {

struct MessageAllocator : public std::allocator<char> {
  char* allocate(size_type n) {
    Message *msg = (Message*) allocator<char>::allocate(n + sizeof(Message));
    msg->bodySize = n;

    return msg->buffer;
  }

  void deallocate(pointer p, size_type n) {
    allocator<char>::deallocate(p - sizeof(Message), n + sizeof(Message));
  }

    Message* allocateMessage(size_type bodyLength) {
    Message *msg = 
      (Message*) allocator<char>::allocate(bodyLength + sizeof(Message));
    msg->bodySize = bodyLength;
    return msg;
  }
  void deallocateMessage(Message *msg) {
    allocator<char>::deallocate((char*) msg, msg->bodySize + sizeof(Message));
  }
  void deallocate(Message *msg) {
    deallocateMessage(msg);
  }
};

}}

#endif /* __MESSAGE_ALLOCATOR_H__ */
