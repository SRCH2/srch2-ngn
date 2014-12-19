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


