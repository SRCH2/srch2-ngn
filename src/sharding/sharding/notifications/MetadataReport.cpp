#include "MetadataReport.h"
#include "../ShardManager.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../state_machine/StateMachine.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

bool MetadataReport::resolveNotification(SP(ShardingNotification) readAckNotif){
	ShardManager::getShardManager()->getStateMachine()->handle(readAckNotif);
	return true;
}
ShardingMessageType MetadataReport::messageType() const{
	return ShardingNewNodeReadMetadataReplyMessageType;
}
void * MetadataReport::serializeBody(void * buffer) const{
	buffer = writeview->serialize(buffer, false);
	return buffer;
}
unsigned MetadataReport::getNumberOfBytesBody() const{
	unsigned numberOfBytes= 0;
	numberOfBytes += writeview->getNumberOfBytes(false);
	return numberOfBytes;
}
void * MetadataReport::deserializeBody(void * buffer) {
	writeview = new Cluster_Writeview();
	buffer = writeview->deserialize(buffer, false);
	return buffer;
}
Cluster_Writeview * MetadataReport::getWriteview() const{
	return writeview;
}

bool MetadataReport::operator==(const MetadataReport & report){
	return *writeview == *(report.writeview);
}


ShardingMessageType MetadataReport::REQUEST::messageType() const{
	return ShardingNewNodeReadMetadataRequestMessageType;
}

bool MetadataReport::REQUEST::resolveNotification(SP(ShardingNotification) readNotif){
	ShardManager::getShardManager()->getMetadataManager()->resolve(boost::dynamic_pointer_cast<MetadataReport::REQUEST>(readNotif));
	return true;
}


}
}
