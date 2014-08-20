#ifndef __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__
#define __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__

#include "Notification.h"
#include "core/util/SerializationHelper.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class LoadBalancingReport : public ShardingNotification{
public:
	LoadBalancingReport(){};
	LoadBalancingReport(double load){
		this->load = load;
	};

    ShardingMessageType messageType() const{
    	return ShardingLoadBalancingReportMessageType;
    }
	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = srch2::util::serializeFixedTypes(load, buffer);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes = 0;
		numberOfBytes += ShardingNotification::getNumberOfBytes();
		numberOfBytes += sizeof(double);
		return numberOfBytes;
	}
	void * deserialize(void * buffer){
		buffer = ShardingNotification::deserialize(buffer);
		buffer = srch2::util::deserializeFixedTypes(buffer, load);
		return buffer;
	}
    double getLoad() const{
    	return this->load;
    }

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
	};
};

}
}


#endif // __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__
