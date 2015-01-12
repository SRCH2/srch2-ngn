#ifndef __SHARDING_ACL_ATTR_READ_NOTIF_H_
#define __SHARDING_ACL_ATTR_READ_NOTIF_H_


#include "sharding/configuration/ShardingConstants.h"
#include "server/HTTPJsonResponse.h"
#include "Notification.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"

using namespace std;
namespace srch2 {
namespace httpwrapper {



class AclAttributeReadNotification : public ShardingNotification{
public:

	AclAttributeReadNotification(const string & roleId,
			const NodeTargetShardInfo & target,
			boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview);
	AclAttributeReadNotification();

	bool resolveNotification(SP(ShardingNotification) _notif);
    bool hasResponse() const {
			return true;
	}
    ShardingMessageType messageType() const{
    	return ShardingAclAttrReadMessageType;
    }
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer) ;
	const NodeTargetShardInfo & getTarget();
	const string & getRoleId() const ;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> getReadview() const;
private:
	NodeTargetShardInfo target;
	string roleId;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;


public:
	class ACK : public ShardingNotification{
	public:
		ACK(const vector<unsigned> & refiningAttrs, const vector<unsigned> & searchableAttrs);
		ACK(){};

		void * serializeBody(void * buffer) const;
		unsigned getNumberOfBytesBody() const;
		void * deserializeBody(void * buffer) ;
		bool resolveNotification(SP(ShardingNotification) _notif);
	    ShardingMessageType messageType() const{
	    	return ShardingAclAttrReadACKMessageType;
	    }
	    vector<unsigned> & getListOfRefiningAttributes();
	    vector<unsigned> & getListOfSearchableAttributes();

	    void setListOfRefiningAttributes(vector<unsigned> & list);
	    void setListOfSearchableAttributes(vector<unsigned> & list);

	private:
		vector<uint32_t> listOfRefiningAttributes;
		vector<uint32_t> listOfSearchableAttributes;
	};
};


}
}


#endif // __SHARDING_ACL_ATTR_READ_NOTIF_H_
