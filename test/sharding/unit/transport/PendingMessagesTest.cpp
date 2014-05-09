#include "transport/Message.h"
#include "transport/PendingMessages.h"
#include "transport/MessageAllocator.h"
#include "configuration/ShardingConstants.h"
#include <assert.h>
#include <cstdlib>
#include "util/Assert.h"

using namespace srch2::httpwrapper;
using namespace srch2::instantsearch;

static const char REPLY_MESSAGE_DETAILS[] = "THIS IS A REPLY MESSAGE";

struct x : Callback {
  void timeout(void*);
  void callback(Message*);
};

void x::timeout(void*) {}
void x::callback(Message *reply) {
  ASSERT( reply->bodySize == sizeof(REPLY_MESSAGE_DETAILS) );
  ASSERT( !memcmp(reply->buffer, REPLY_MESSAGE_DETAILS, 
        sizeof(REPLY_MESSAGE_DETAILS) ));
}

int main() {
  PendingMessages msg;
  MessageAllocator alloc;

  CallbackReference cbRef =
  msg.prepareCallback(NULL, new x, SearchCommandMessageType, 
    false, 4);

  msg.addMessage(0, 3, cbRef);

  for(int i=0; i < 3; ++i) {
    Message *reply = alloc.allocateMessage(sizeof(REPLY_MESSAGE_DETAILS));
    reply->initial_time = 3;
    memcpy(reply->buffer, REPLY_MESSAGE_DETAILS, 
      sizeof(REPLY_MESSAGE_DETAILS));
    msg.resolve(reply);
  }

  Message *reply = alloc.allocateMessage(sizeof(REPLY_MESSAGE_DETAILS));
  reply->initial_time = 3;
  memcpy(reply->buffer, REPLY_MESSAGE_DETAILS, sizeof(REPLY_MESSAGE_DETAILS));
  msg.resolve(reply);
}
