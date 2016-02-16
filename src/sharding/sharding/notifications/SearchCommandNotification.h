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
#ifndef __SHARDING_PROCESSOR_SEARCH_COMMAND_NOTIFICATOIN_H_
#define __SHARDING_PROCESSOR_SEARCH_COMMAND_NOTIFICATOIN_H_

#include "core/util/Assert.h"
#include "core/util/Logger.h"

#include "sharding/configuration/ShardingConstants.h"
#include "sharding/sharding/notifications/Notification.h"
#include "../metadata_manager/Shard.h"
#include "include/instantsearch/LogicalPlan.h"
#include "include/instantsearch/Schema.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {

class ClusterResourceMetadata_Readview;

class SearchCommand : public ShardingNotification{
public:

    SearchCommand(unsigned coreId, LogicalPlan * logicalPlan, NodeTargetShardInfo target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview);
    SearchCommand();
    ~SearchCommand();

    //serializes the object to a byte array and places array into the region
    //allocated by given allocator
    /*
     * Serialization scheme:
     * | isLogicalPlanNULL | LogicalPlan(only is isLogicalPlanNULL is true) |
     */
    void* serializeBody(void * buffer) const;


    unsigned getNumberOfBytesBody() const;

    //NOTE : schema must be set before this method is called, otherwise the logical plan
    //       wont be deserialized
    void * deserializeBody(void* buffer);

    //Returns the type of message which uses this kind of object as transport
    ShardingMessageType messageType() const ;


    bool resolveNotification(SP(ShardingNotification) _notif);

    boost::shared_ptr<const ClusterResourceMetadata_Readview> getReadview() const;
    const NodeTargetShardInfo & getTarget() const;
    LogicalPlan * getLogicalPlan() const;
    void setSchema(const Schema * schema);
private:
    uint32_t coreId ;
    srch2::instantsearch::LogicalPlan * logicalPlan;
    NodeTargetShardInfo target;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
    const srch2::instantsearch::Schema * schema;

    bool isCreatedByDeserialization();
};


}
}

#endif // __SHARDING_PROCESSOR_SEARCH_COMMAND_NOTIFICATOIN_H_


