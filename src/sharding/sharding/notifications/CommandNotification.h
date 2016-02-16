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
#ifndef __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_
#define __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_


#include "sharding/configuration/ShardingConstants.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {



class CommandNotification : public ShardingNotification{
public:

	CommandNotification(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			const NodeTargetShardInfo & target,
			ShardCommandCode commandCode = ShardCommandCode_Merge, const string & filePath = "");
	CommandNotification();
	~CommandNotification();


	bool resolveNotification(SP(ShardingNotification) _notif);
    bool hasResponse() const {
			return true;
	}
    ShardingMessageType messageType() const;
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer) ;

	ShardCommandCode getCommandCode() const;

	string getJsonFilePath() const;

	string getNewLogFilePath() const;

	NodeTargetShardInfo getTarget();

	boost::shared_ptr<const ClusterResourceMetadata_Readview> getReadview() const;

private:
	NodeTargetShardInfo target;
	ShardCommandCode commandCode;

	// in case it's export
	string jsonFilePath;

	// in case it's resetLogger
	string newLogFilePath;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
};


}}


#endif // __SHARDING_SHARDING_SHARDING_COMMAND_NOTIF_H_
