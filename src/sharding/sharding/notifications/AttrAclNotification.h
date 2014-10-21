#ifndef __SHARDING_SHARDING_SHARDING_ATTR_ACL_NOTIF_H_
#define __SHARDING_SHARDING_SHARDING_ATTR_ACL_NOTIF_H_

namespace srch2is = srch2::instantsearch;
using namespace std;

#include "sharding/configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/MessageAllocator.h"
#include "Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"
namespace srch2 {
namespace httpwrapper {



class AttrAclNotification : public ShardingNotification{
public:
	AttrAclNotification(const vector<string> & attributesToCheck, const unsigned coreId){
	}
private:

};
}
}


#endif // __SHARDING_SHARDING_SHARDING_ATTR_ACL_NOTIF_H_
