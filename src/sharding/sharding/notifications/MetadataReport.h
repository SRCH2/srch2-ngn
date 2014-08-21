#ifndef __SHARDING_SHARDING_METADATA_REPORT_H__
#define __SHARDING_SHARDING_METADATA_REPORT_H__

#include "Notification.h"
#include "core/util/SerializationHelper.h"
#include "../metadata_manager/Cluster_Writeview.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

// This notification is used to read the metadata of a node
class MetadataReport: public ShardingNotification{
public:
	MetadataReport(Cluster_Writeview * writeview){
		this->writeview = writeview;
	}
	MetadataReport(){};
    ShardingMessageType messageType() const{
    	return ShardingNewNodeReadMetadataReplyMessageType;
    }
	void * serialize(void * buffer) const{
		buffer = ShardingNotification::serialize(buffer);
		buffer = writeview->serialize(buffer, false);
		return buffer;
	}
	unsigned getNumberOfBytes() const{
		unsigned numberOfBytes= 0;
		numberOfBytes += ShardingNotification::getNumberOfBytes();
		numberOfBytes += writeview->getNumberOfBytes(false);
		return numberOfBytes;
	}
	void * deserialize(void * buffer) {
		buffer = ShardingNotification::deserialize(buffer);
		buffer = writeview->deserialize(buffer, false);
		return buffer;
	}
    Cluster_Writeview * getWriteview() const{
    	return writeview;
    };
private:
    // must not be deleted in this class.
    Cluster_Writeview * writeview;

public:
    ///////////// Sub classes :
    class REQUEST : public ShardingNotification{
    public:
        ShardingMessageType messageType() const{
        	return ShardingNewNodeReadMetadataRequestMessageType;
        }
    };
};

}
}


#endif // __SHARDING_SHARDING_METADATA_REPORT_H__
