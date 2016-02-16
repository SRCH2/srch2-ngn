/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
		~ShardStatus(){
			if(shardId != NULL){
				delete shardId;
			}
		}
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
    ~CommandStatusNotification(){
		for(vector<CommandStatusNotification::ShardStatus *>::iterator shardStatusItr = shardResults.begin();
				shardStatusItr != shardResults.end(); ++shardStatusItr){
			delete *shardStatusItr;
		}
    }
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
