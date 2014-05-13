#include "transport/TransportManager.h"
#include <event.h>
#include <cstdlib>

static const char *const MESSAGE_CONTENTS[] =
{ "Well Costello, I'm going to New York with you. You know Bucky Harris, the Yankee's manager, gave me a job as coach for as long as you're on the team",
  " Look Abbott, if you're the coach, you must know all the players.",
  " I certainly do.",
  "Well you know I've never met the guys. So you'll have to tell me their names, and then I'll know who's playing on the team.",
  "Oh, I'll tell you their names, but you know it seems to me they give these ball players now-a-days very peculiar names.",
  "You mean funny names?",
  "Strange names, pet names...like Dizzy Dean...",
  " His brother Daffy.",
  "Daffy Dean...",
  " And their French cousin.",
  "French?",
  "Goofe",
  "Goofe Dean. Well, let's see, we have on the bags, Who's on first, What's on second, I Don't Know is on third...",
  "That's what I want to find out.",
  "I say Who's on first, What's on second, I Don't Know's on third.",
  "Are you the manager?",
  "Yes",
  "You gonna be the coach to?",
  "Yes",
  "  And you don't know the fellows' names?",
  "Well I should.",
  "Well then who's on first?",
  "Yes.",
  "I mean the fellow's name.",
  "Who",
  "The guy on first",
  "Who",
  "The first baseman",
  "Who",
  " The guy playing...",
  "Who is on first!",
  "I'm asking YOU who's on first.",
  "That's the man's name.",
  "That's who's name?",
  "Yes",
  "Well go ahead and tell me",
  "That's it",
  "That's who?",
  " Yes.",
  "Look, you gotta first baseman?",
  "Certainly.",
  "Who's playing first?",
  "That's right.",
  "When you pay off the first baseman every month, who gets the money?",
  "Every dollar of it.",
   "All I'm trying to find out is the fellow's name on first base.",
   "Who.",
   "The guy that gets...",
   " That's it.",
   "Who gets the money...",
   "He does, every dollar. Sometimes his wife comes down and collects it.",
   "Who's wife?",
   "Yes"
};


using namespace srch2::httpwrapper;

#include <cstdio>

struct TestHandler : public CallBackHandler {
  int messageRecieved;
  Message* notify(Message *msg) {
   // assert(!strcmp(msg->buffer, MESSAGE_CONTENTS[messageRecieved]));
    printf("%d: \t %s\n", msg->shardId.coreId, msg->body);
    fflush(stdout);
    if(++messageRecieved == 52) pthread_exit(0);
    return NULL;
  }
  TestHandler() : messageRecieved(0) {}
};

void* dispatch(void *arg) {
  event_base_dispatch((struct event_base*) arg);
  return NULL;
}

int main() {
  std::vector<Node>* nodes = new std::vector<Node>();
  nodes->push_back(
      Node(std::string("apple"), std::string("127.0.0.1"), 9552, false));
  nodes->push_back(
      Node(std::string("apple"), std::string("127.0.0.1"), 9551, false));

  int i=0;
  for(std::vector<Node>::iterator node = nodes->begin(); 
      node != nodes->end(); ++node) {
    node->setId(i++);
  }

  std::vector<Node>::iterator n = nodes->begin(); 

  int cid; i = 0;
  while(true) {
    if((cid = fork()) == 0) {
      //in child
      ++n, ++i;
      if(n == nodes->end()) std::exit(0);
      continue;
    } else {
      //in parent
      n->thisIsMe = true;
      break;
   }
  }

  const int NUM_THREADS = 3;
  EventBases eventbases;
  for(int t=0; t<NUM_THREADS; ++t) 
    eventbases.push_back(event_base_new());

  TransportManager *tm =  new TransportManager(eventbases, *nodes);
  tm->setInternalTrampoline(new TestHandler());

  pthread_t tmp[NUM_THREADS];
  for(int t=0;  t < NUM_THREADS; ++t) {
    pthread_create(tmp + t, NULL, dispatch, eventbases[t]);
  }

  for(std::vector<Node>::iterator node = nodes->begin(); 
      node != nodes->end(); ++node) {
    if(node == n) continue;
    for(int m=0; m < 52; ++m) {

      int messageLength = strlen(MESSAGE_CONTENTS[m]);
      Message* msg = tm->getMessageAllocator()->allocateMessage(messageLength+1);
      msg->type = StatusMessageType;
      msg->mask |= INTERNAL_MASK;
      msg->bodySize = messageLength+1;
      msg->shardId.coreId = n->getId();
      memcpy(msg->body, MESSAGE_CONTENTS[m], messageLength);

      tm->route(node->getId(), msg);
      tm->getMessageAllocator()->deallocateByMessagePointer(msg);
    }
  }
}
