struct Message {
  enum Type;
  char mask;
  Shard::Id shard;
  unsigned totalSize;
  unsigned time;
  char buffer[0];
};

class MessageAllocator; //: std::allocator<char>;
