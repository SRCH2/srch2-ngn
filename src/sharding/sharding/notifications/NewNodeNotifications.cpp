#include "MetadataReport.h"
#include "NewNodeLockNotification.h"
#include "../ShardManager.h"
#include "../ClusterOperationContainer.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


bool NewNodeLockNotification::resolveMessage(Message * msg, NodeId senderNode){
	NewNodeLockNotification * newNodeLockNotification =
			ShardingNotification::deserializeAndConstruct<NewNodeLockNotification>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | Batch of size %d, %s",
			newNodeLockNotification->getDescription().c_str(),
			newNodeLockNotification->getLockRequest()->requestBatch.size(),
			newNodeLockNotification->getLockRequest()->isBlocking ? "blocking." : "non-blocking.");
	if(ShardManager::getShardManager()->handleBouncing(newNodeLockNotification)){
		return true;
	}
	ShardManager::getShardManager()->getLockManager()->resolve(newNodeLockNotification);
	delete newNodeLockNotification;
	return false;
}

bool NewNodeLockNotification::ACK::resolveMessage(Message * msg, NodeId sendeNode){
	NewNodeLockNotification::ACK * newNodeLockAck =
			ShardingNotification::deserializeAndConstruct<NewNodeLockNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", newNodeLockAck->getDescription().c_str());
	if(newNodeLockAck->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete newNodeLockAck;
		return true;
	}
	ShardManager::getShardManager()->getStateMachine()->handle(newNodeLockAck);
	delete newNodeLockAck;
	return false;
}


bool MetadataReport::resolveMessage(Message * msg, NodeId sendeNode){
	MetadataReport * readAckNotif =
			ShardingNotification::deserializeAndConstruct<MetadataReport>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", readAckNotif->getDescription().c_str());
	if(readAckNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete readAckNotif;
		return true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(readAckNotif);
	delete readAckNotif;
	return false;
}

bool MetadataReport::REQUEST::resolveMessage(Message * msg, NodeId sendeNode){
	MetadataReport::REQUEST * readNotif =
			ShardingNotification::deserializeAndConstruct<MetadataReport::REQUEST>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", readNotif->getDescription().c_str());
	if( ShardManager::getShardManager()->handleBouncing(readNotif) ){
		return true;
	}
	ShardManager::getShardManager()->getMetadataManager()->resolve(readNotif);
	delete readNotif;
	return false;
}


}
}
