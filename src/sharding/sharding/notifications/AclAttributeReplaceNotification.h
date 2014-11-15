#ifndef __SHARDING_ACL_ATTR_REPLACE_NOTIF_H_
#define __SHARDING_ACL_ATTR_REPLACE_NOTIF_H_


#include "sharding/configuration/ShardingConstants.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {



class AclAttributeReplaceNotification : public ShardingNotification{
public:

	AclAttributeReplaceNotification(const vector<string> & attributes, unsigned coreId);
	AclAttributeReplaceNotification();
	~AclAttributeReplaceNotification(){};

	bool resolveNotification(SP(ShardingNotification) _notif);
    bool hasResponse() const ;
    ShardingMessageType messageType() const;
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer) ;


	const vector<string> & getAttributes() const ;

	const CoreInfo_t * getCoreInfo() const;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> getReadview() const;
private:
	vector<string> attributes;
	unsigned coreId; // the coreId the attribute core
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;


public:
	class ACK : public ShardingNotification{
	public:
		ACK(){};
		~ACK(){};
		bool resolveNotification(SP(ShardingNotification) _notif);
	    ShardingMessageType messageType() const;

	private:
	};
};


}
}


#endif // __SHARDING_ACL_ATTR_REPLACE_NOTIF_H_
