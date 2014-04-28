  /*
  class QueryMessage  : Message {
    QueryMessage(String& s) {
      type = QUERY_MESSAGE;
      totalSize = s.length();
      time = 6;
      memcpy(buffer, s.data(), s.length());
      }
  */
/*
  queryMessage(String s) {
    void* ptr = malloc( sizeof(Message) + s.length()*sizeof(char) );
                             return new (ptr) QueryMessage(s);
  }*/



typedef std::vector<event_base*> EventBases;
typedef std::vector<Node> Nodes;

struct TransportManager {
  struct RouteMap routeMap;
  TransportManger(EventBases&, Nodes&);
  /*
   *  Try to establish a connection with shardId. On failure return false
   *
   *
  bool connect(Node);

  *
   *  Repeatedly try to establish a connection with shardId. On success 
   *  trigger the appropriate callback with the response message. 
   *
   /
  void connect_w_cb(unsigned, void (*func) (Message));

  *
   *  Blocking version of above.
   *
   /
  Message connect_w_response(unsigned);

  *
   *   Same as their namesakes; however, they have an additional timeout feature
   /
  bool connect_w_cb_n_timeout(unsigned, 
      void (*func) (Message), unsigned timeout);

  Message connect_w_response_n_timeout(unsigned, unsigned timeout);
  */
};
