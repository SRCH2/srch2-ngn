#ifndef __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__
#define __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__

#include "Notification.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class LoadBalancingReport : public ShardingNotification{
public:
	LoadBalancingReport(){
		this->load = 0;
	};
	LoadBalancingReport(double load){
		this->load = load;
	};

	bool resolveNotification(SP(ShardingNotification) _notif);

    ShardingMessageType messageType() const;
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer);
    double getLoad() const;

private:
	// this load might not be consistent with other loads
	// but this is the most up to date view of that node.
	double load;


public:
	//////// Sub classes
	class REQUEST : public ShardingNotification{
	public:
	    ShardingMessageType messageType() const{
	    	return ShardingLoadBalancingReportRequestMessageType;
	    }
		bool resolveNotification(SP(ShardingNotification) _notif);
	    bool hasResponse() const {
				return true;
		}
	};
};

}
}


#endif // __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__
