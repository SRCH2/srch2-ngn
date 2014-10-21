#include "CopyToMeNotification.h"
#include "LoadBalancingReport.h"
#include "MoveToMeNotification.h"
#include "../ShardManager.h"
#include "../ClusterOperationContainer.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

bool CopyToMeNotification::resolveMessage(Message * msg, NodeId sendeNode){
	CopyToMeNotification * copyNotif =
			ShardingNotification::deserializeAndConstruct<CopyToMeNotification>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | cp %s %s", copyNotif->getDescription().c_str(),
			copyNotif->getReplicaShardId().toString().c_str(),
			copyNotif->getUnassignedShardId().toString().c_str());
	if(ShardManager::getShardManager()->handleBouncing(copyNotif)){
		return true;
	}

	// call migration manager to start transfering this shard.
	ClusterShardId replicaShardId = copyNotif->getReplicaShardId();
	ClusterShardId unassignedShardId = copyNotif->getUnassignedShardId();
	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();
	if(writeview->localClusterDataShards.find(replicaShardId) == writeview->localClusterDataShards.end()){
		// NOTE: requested shard does not exist on this node.
		ASSERT(false);
		return false;
	}

	ShardManager::getShardManager()->getMigrationManager()->migrateShard(replicaShardId, writeview->localClusterDataShards.at(replicaShardId).server, unassignedShardId,
			copyNotif->getDest(), copyNotif->getSrc());

	delete copyNotif;
	return false;
}


bool LoadBalancingReport::resolveMessage(Message * msg, NodeId sendeNode){
	LoadBalancingReport * loadBalancingReportNotif =
			ShardingNotification::deserializeAndConstruct<LoadBalancingReport>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | Load : %f", loadBalancingReportNotif->getDescription().c_str(), loadBalancingReportNotif->getLoad());
	if(loadBalancingReportNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete loadBalancingReportNotif;
		return true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(loadBalancingReportNotif);
	delete loadBalancingReportNotif;
	return false;
}


bool LoadBalancingReport::REQUEST::resolveMessage(Message * msg, NodeId sendeNode){
	LoadBalancingReport::REQUEST * loadBalancingReqNotif =
			ShardingNotification::deserializeAndConstruct<LoadBalancingReport::REQUEST>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", loadBalancingReqNotif->getDescription().c_str());
	if ( ShardManager::getShardManager()->handleBouncing(loadBalancingReqNotif) ){
		return true;
	}

	LoadBalancingReport * report = new LoadBalancingReport(ShardManager::getShardManager()->getWriteview()->getLocalNodeTotalLoad());
	report->setDest(loadBalancingReqNotif->getSrc());
	report->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
	ShardManager::getShardManager()->send(report);
	delete report;
	delete loadBalancingReqNotif;
	return false;
}



bool MoveToMeNotification::resolveMessage(Message * msg, NodeId sendeNode){
	MoveToMeNotification * moveNotif  =
			ShardingNotification::deserializeAndConstruct<MoveToMeNotification>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | Shard : %s", moveNotif->getDescription().c_str(),
			moveNotif->getDescription().c_str(), moveNotif->getShardId().toString().c_str());
	if(moveNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete moveNotif;
		return true;
	}

	Cluster_Writeview * writeview = ShardManager::getShardManager()->getWriteview();

	if(writeview->localClusterDataShards.find(moveNotif->getShardId()) == writeview->localClusterDataShards.end()){
		// NOTE : requested shard does not exist on this node
		ASSERT(false);
		return true;
	}
	// call migration manager to transfer this shard
	ShardManager::getShardManager()->getMigrationManager()->migrateShard(moveNotif->getShardId(),
			writeview->localClusterDataShards.at(moveNotif->getShardId()).server, moveNotif->getShardId(),moveNotif->getDest(), moveNotif->getSrc());

	delete moveNotif;
	return false;
}

bool MoveToMeNotification::START::resolveMessage(Message * msg, NodeId sendeNode){
	MoveToMeNotification::START * moveNotif  =
			ShardingNotification::deserializeAndConstruct<MoveToMeNotification::START>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | Shard : %s", moveNotif->getDescription().c_str() ,
			moveNotif->getShardId().toString().c_str());
	if(moveNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ShardManager::getShardManager()->saveBouncedNotification(moveNotif);
		return true;
	}
	if(! ShardManager::getShardManager()->isJoined()){
		ShardManager::getShardManager()->bounceNotification(moveNotif);
		Logger::debug("==> Not joined yet. Bounced.");
		return true;
	}
	ShardManager::getShardManager()->getStateMachine()->registerOperation(new ShardMoveSrcOperation(moveNotif->getSrc(), moveNotif->getShardId()));
	delete moveNotif;
	return false;
}


bool MoveToMeNotification::ACK::resolveMessage(Message * msg, NodeId sendeNode){
	MoveToMeNotification::ACK * moveAckNotif  =
			ShardingNotification::deserializeAndConstruct<MoveToMeNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", moveAckNotif->getDescription().c_str());
	if(moveAckNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete moveAckNotif;
		return true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(moveAckNotif);
	delete moveAckNotif;
	return false;
}


bool MoveToMeNotification::FINISH::resolveMessage(Message * msg, NodeId sendeNode){
	MoveToMeNotification::FINISH * moveFinishNotif  =
			ShardingNotification::deserializeAndConstruct<MoveToMeNotification::FINISH>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", moveFinishNotif->getDescription().c_str());
	if(moveFinishNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete moveFinishNotif;
		return false;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(moveFinishNotif);
	delete moveFinishNotif;
	return false;
}


bool MoveToMeNotification::ABORT::resolveMessage(Message * msg, NodeId sendeNode){
	MoveToMeNotification::ABORT * moveAbortNotif  =
			ShardingNotification::deserializeAndConstruct<MoveToMeNotification::ABORT>(Message::getBodyPointerFromMessagePointer(msg));
	Logger::debug("%s | .", moveAbortNotif->getDescription().c_str());
	if(moveAbortNotif->isBounced()){
		Logger::debug("==> Bounced.");
		ASSERT(false);
		delete moveAbortNotif;
		return true;
	}

	ShardManager::getShardManager()->getStateMachine()->handle(moveAbortNotif);
	delete moveAbortNotif;
	return false;
}


}
}
