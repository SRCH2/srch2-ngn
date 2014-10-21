#ifndef __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_
#define __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"
namespace srch2 {
namespace httpwrapper {



class CommandNotification : public ShardingNotification{
public:

	CommandNotification(const NodeTargetShardInfo & target, ShardCommandCode commandCode = ShardCommandCode_Merge, const string & filePath = ""){
		this->target = target;
		this->commandCode = ShardCommandCode_Merge;
		if(this->commandCode == ShardCommandCode_Export){
			jsonFilePath = filePath;
		}
		if(this->commandCode == ShardCommandCode_ResetLogger){
			newLogFilePath = filePath;
		}
	}
	~CommandNotification(){};

    ShardingMessageType messageType() const{
    	return ShardingShardCommandMessageType;
    }
	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = target.serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(commandCode, buffer);
		if(commandCode == ShardCommandCode_Export){
			buffer = srch2::util::serializeString(jsonFilePath, buffer);
		}
		if(commandCode == ShardCommandCode_ResetLogger){
			buffer = srch2::util::serializeString(newLogFilePath, buffer);
		}
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += ShardingNotification::getNumberOfBytes();
		numberOfBytes += target.getNumberOfBytes();
		numberOfBytes += sizeof(commandCode);
		if(commandCode == ShardCommandCode_Export){
			numberOfBytes += sizeof(unsigned) + jsonFilePath.size();
		}
		if(commandCode == ShardCommandCode_ResetLogger){
			numberOfBytes += sizeof(unsigned) + newLogFilePath.size();
		}
		return numberOfBytes;
	}
	void * deserialize(void * buffer) {
		buffer = ShardingNotification::deserialize(buffer);
		buffer = target.deserialize(buffer);
		buffer = srch2::util::deserializeFixedTypes(buffer, commandCode);
		if(commandCode == ShardCommandCode_Export){
			buffer = srch2::util::deserializeString(buffer, jsonFilePath);
		}
		if(commandCode == ShardCommandCode_ResetLogger){
			buffer = srch2::util::deserializeString(buffer, newLogFilePath);
		}
		return buffer;
	}

	ShardCommandCode getCommandCode() const{
		return commandCode;
	}

	string getJsonFilePath() const{
		return jsonFilePath;
	}

	string getNewLogFilePath() const{
		return newLogFilePath;
	}
private:
	NodeTargetShardInfo target;
	ShardCommandCode commandCode;

	// in case it's export
	string jsonFilePath;

	// in case it's resetLogger
	string newLogFilePath;
};


}}


#endif // __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_
