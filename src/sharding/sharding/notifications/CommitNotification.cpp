#include "CommitNotification.h"
#include "../ShardManager.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../ClusterOperationContainer.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


CommitNotification::CommitNotification(MetadataChange * metadataChange){
	ASSERT(metadataChange != NULL);
	this->metadataChange = metadataChange;
}

bool CommitNotification::resolveMessage(Message * msg, NodeId sendeNode){
	CommitNotification * commitNotif =
			ShardingNotification::deserializeAndConstruct<CommitNotification>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | Change : %s", commitNotif->getDescription().c_str(),
			commitNotif->getMetadataChange()->toNameString().c_str());
	if(ShardManager::getShardManager()->handleBouncing(commitNotif)){
		return true;
	}

	ShardManager::getShardManager()->getMetadataManager()->resolve(commitNotif);
	delete commitNotif;
	return false;
}

MetadataChange * CommitNotification::getMetadataChange() const{
	return this->metadataChange;
}


//Returns the type of message which uses this kind of object as transport
ShardingMessageType CommitNotification::messageType() const{
	return ShardingCommitMessageType;
}

bool CommitNotification::operator==(const CommitNotification & right){
	return *metadataChange == *(right.metadataChange);
}

bool CommitNotification::ACK::operator==(const CommitNotification::ACK & right){
	return true;
}

bool CommitNotification::ACK::resolveMessage(Message * msg, NodeId sendeNode){
	CommitNotification::ACK * commitAckNotif =
			ShardingNotification::deserializeAndConstruct<CommitNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", commitAckNotif->getDescription().c_str());
	if(commitAckNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete commitAckNotif;
		return true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(commitAckNotif);
	delete commitAckNotif;
	return false;
}

}
}
