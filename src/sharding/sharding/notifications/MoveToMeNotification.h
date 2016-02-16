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
#ifndef __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__
#define __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__

#include "Notification.h"
#include "../metadata_manager/Shard.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class MoveToMeNotification : public ShardingNotification {
public:
	ShardingMessageType messageType() const{
		return ShardingMoveToMeMessageType;
	}
	MoveToMeNotification(const ClusterShardId & shardId){
        this->shardId = shardId;
    }
	MoveToMeNotification(){};


	bool resolveNotification(SP(ShardingNotification) _notif);

	bool hasResponse() const {
		return true;
	}

	void * serializeBody(void * buffer) const{
        buffer = shardId.serialize(buffer);
        return buffer;
    }
    unsigned getNumberOfBytesBody() const{
        unsigned numberOfBytes = 0;
        numberOfBytes += shardId.getNumberOfBytes();
        return numberOfBytes;
    }
    void * deserializeBody(void * buffer){
        buffer = shardId.deserialize(buffer);
        return buffer;
    }

    bool operator==(const MoveToMeNotification & right){
        return shardId == right.shardId;
    }

    ClusterShardId getShardId() const{
        return this->shardId;
    }
private:
    ClusterShardId shardId;

public:
	class CleanUp : public ShardingNotification {
	public:
		CleanUp(const ClusterShardId & shardId){
			this->shardId = shardId;
		}
		CleanUp(){};

		bool resolveNotification(SP(ShardingNotification) _notif);

		ShardingMessageType messageType() const{
			return ShardingMoveToMeCleanupMessageType;
		}
		void * serializeBody(void * buffer) const{
			buffer = shardId.serialize(buffer);
			return buffer;
		}
		unsigned getNumberOfBytesBody() const{
			unsigned numberOfBytes = 0;
			numberOfBytes += shardId.getNumberOfBytes();
			return numberOfBytes;
		}
		void * deserializeBody(void * buffer){
			buffer = shardId.deserialize(buffer);
			return buffer;
		}

		bool operator==(const MoveToMeNotification::CleanUp & right){
			return shardId == right.shardId;
		}

		ClusterShardId getShardId() const{
			return this->shardId;
		}
	private:
		ClusterShardId shardId;
	};
	class ACK : public ShardingNotification {
	public:
		ShardingMessageType messageType() const{
			return ShardingMoveToMeACKMessageType;
		}

		bool resolveNotification(SP(ShardingNotification) _notif);
	};
};


}
}


#endif // __SHARDING_SHARDING_MOVE_TO_ME_NOTIFICATION_H__
