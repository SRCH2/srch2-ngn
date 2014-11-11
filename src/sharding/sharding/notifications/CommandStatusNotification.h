#ifndef __SHARDING_PROCESSOR_COMMAND_STATUS_NOTIFICATION_H_
#define __SHARDING_PROCESSOR_COMMAND_STATUS_NOTIFICATION_H_


#include "sharding/configuration/ShardingConstants.h"
#include "sharding/transport/MessageAllocator.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
using namespace std;
namespace srch2 {
namespace httpwrapper {



class CommandStatusNotification : public ShardingNotification{
public:

	struct ShardStatus{
	public:
		ShardStatus(const ShardId * shardId = NULL);

	    /*
	     * Contains the identifier of the shard ...
	     */
	    const ShardId * shardId;
	    /*
	     * Success => True
	     * Failure => False
	     */
//	    bool statusValue;
	    vector<bool> statusValues;

	    bool getStatusValue() const;
	    void setStatusValue(bool statusValue);

	    /*
	     * Contains the messages/warnings/errors coming from the shard ...
	     */
	    Json::Value messages;

	    //serializes the object to a byte array and places array into the region
	    //allocated by given allocator
	    void* serialize(void * buffer) const;

	    unsigned getNumberOfBytes() const;

	    //given a byte stream recreate the original object
	    static ShardStatus * deserialize(void* buffer);

		private:
	};

    CommandStatusNotification(ShardCommandCode commandCode);
    CommandStatusNotification();
    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    void* serializeBody(void * buffer) const;

    unsigned getNumberOfBytesBody() const;

    //given a byte stream recreate the original object
    void * deserializeBody(void* buffer);

	bool resolveNotification(SP(ShardingNotification) notif);

    //Returns the type of message which uses this kind of object as transport
    ShardingMessageType messageType() const;

    ShardCommandCode getCommandCode() const ;

    vector<ShardStatus *> getShardsStatus() const ;

    void addShardResult(CommandStatusNotification::ShardStatus * shardResult);

private:
    /*
     * commandCode is the code for the request command corresponding to this CommandStatus
     * like insert or update or ...
     */
    ShardCommandCode commandCode;

    vector<CommandStatusNotification::ShardStatus *> shardResults;
};



}
}

#endif // __SHARDING_PROCESSOR_COMMAND_STATUS_NOTIFICATION_H_
