struct CallBack {
  bool isSearchCallBack;
  void* originalSerializableObject;
  void* callBackObject;
};

struct PendingRequest {
  time_t timeout;
  MessageTime_t msg_id;
  short callbackAndTypeMask;
};

struct RegisteredCallBack {
  void* callBackObject;
  std::vector<Message*> reply;
};

class PendingMessages {
  static final unsigned NUMBER_OF_BYTE_IN_PENDING_BITMASK = 128;

  char pendingMask[NUMBER_OF_BYTE_IN_PENDING_BITMASK];
  AtomicBitmask pendingRequests(pendingMask);
  char cbMask[NUMBER_OF_BYTE_IN_PENDING_BITMASK];
  AtomicBitmask pendingCallBacks(cbMask);

  struct PendingRequest 
    pendingRequestTable[NUMBER_OF_BYTE_IN_PENDING_BITMASK*8];
  struct RegisteredCallBack callback[NUMBER_OF_BYTE_IN_PENDING_BITMASK*8];

public:
  void addMessage(time_t, MessageTime_t, unsigned CallBack);
  void trigger_timeouts(time_t);
  void resolve(Message*);
};
