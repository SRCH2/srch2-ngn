#include "transport/TransportManager.h"
#include <event.h>
#include <cstdlib>

static const char MESSAGE_CONTENTS[] = "MESSAGE FROM SRCH2";

using namespace srch2::httpwrapper;
/*
void SMCallBackHandler::notify(Message *msg) {
  assert( msg->bodySize == sizeof(MESSAGE_CONTENTS));
  assert( memcmp(msg->buffer, MESSAGE_CONTENTS, sizeof(MESSAGE_CONTENTS)));
}*/

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

  int cid;
  while(true) {
    if((cid = fork()) == 0) {
      //in child
      ++n;
      if(n == nodes->end()) std::exit(0);
      continue;
    } else {
      //in parent
      n->thisIsMe = true;
      break;
   }
  }

  EventBases eventbases;
  eventbases.push_back(event_base_new());

  TransportManager *tm =  new TransportManager(eventbases, *nodes);

  while( !tm->getRouteMap()->isTotallyConnected())
    sleep(2);
}

