#include "ShardManager.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardManager * ShardManager::singleInstance = NULL;

ShardManager * ShardManager::createShardManager(TransportManager * transportManager, ConfigManager * configManager){
	if(singleInstance != NULL){
		ASSERT(false);
		return singleInstance;
	}

	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	configManager->getClusterReadView(clusterReadview);
	MetadataManager * metadataInstance = MetadataManager::createMetadataManager(*(clusterReadview.get()));
	LockManager * lockInstance = LockManager::createLockManager();
	NodeInitializer * nodeInitInstance = NodeInitializer::createNodeInitializer();
	LoadBalancer * loadBalancer = LoadBalancer::createLoadBalancer();
	singleInstance = new ShardManager(transportManager, configManager);
	// set shardManager callback handler of transport manager
	transportManager->registerCallbackForShardingMessageHandler(singleInstance);
	return singleInstance;
}

ShardManager * ShardManager::getShardManager(){
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	return singleInstance;
}

ShardManager::ShardManager(TransportManager * transportManager, ConfigManager * configManager){
	/*
	 * 1. start the ShardManager thread so that NodeInitializer can start working very fast.
	 *    This module will first load the existing shards and make them available to readers as soon as
	 *    possible and then waits for messages from host. If no host exists, we continue to normal execution
	 *    by just changing local shards to read distributed shards (only if they come from disk and we know these shards
	 *    have other replicas in other nodes)
	 */
	this->transportManager = transportManager;
	this->configManager = configManager;
	cancelled = false;
	nextChangeId = 1;
	pthread_t shardManagerThread;
    if (pthread_create(&shardManagerThread, NULL, execute , this) != 0){
        perror("Cannot create thread for handling local message");
        return;
    }
}
ShardManager::~ShardManager(){
	cancelLock.lock();
	cancelled = true;
	cancelLock.unlock();
}

void * ShardManager::execute(void * args){
	ShardManager * shardManager = ShardManager::getShardManager();
	while(true){
		shardManager->handleAll();
		//
		cancelLock.lock();
		if(cancelled){
			cancelLock.unlock();
			break;
		}else{
			cancelLock.unlock();
		}

		sleep(2);
	}
	return NULL;
}

TransportManager * ShardManager::getTransportManager() const{
	return transportManager;
}
ConfigManager * ShardManager::getConfigManager() const{
	return configManager;
}


void ShardManager::registerChangeBroadcast(ShardingChange * change){
	if(change == NULL){
		ASSERT(false);
		return;
	}
	change->setChangeId(nextChangeId++);
	ShardingChange * newChange = change->clone();
	broadcastHistoryBuffer.push(change);
}
void ShardManager::flushChangeHistory(unsigned ackedChangeId){
	// move on buffer and remove elements until changeId is larger than ackedChangeId
	while(broadcastHistoryBuffer.size() > 0 && broadcastHistoryBuffer.front()->getChangeId() < ackedChangeId){
		ShardingChange * change = broadcastHistoryBuffer.front();
		broadcastHistoryBuffer.pop();
		delete change;
	}
}

bool ShardManager::send(ShardingNotification * notification){

	if(notification->getDestOperationId().nodeId == MetadataManager::getMetadataManager()->getCurrentNodeId()){
		ASSERT(false);
		return true;
	}
	unsigned notificationByteSize = notification->getNumberOfBytes();
	void * bodyByteArray = transportManager->getMessageAllocator()->allocateMessageReturnBody(notificationByteSize);
	notification->serialize(bodyByteArray);
	Message * notificationMessage = Message::getMessagePointerFromBodyPointer(bodyByteArray);
	notificationMessage->setShardingMask();
	notificationMessage->setMessageId(transportManager->getUniqueMessageIdValue());
	transportManager->sendMessage(notification->getDestOperationId().nodeId , notificationMessage, 0);
	transportManager->getMessageAllocator()->deallocateByMessagePointer(notificationMessage);

	// currently TM always send the message
	return true;
}

bool ShardManager::resolveMessage(Message * msg, NodeId node){
	if(msg == NULL){
		return false;
	}
	switch (msg->getType()) {
		case ShardingCommitMessageType:
			CommitNotification * commitNotification = CommitNotification::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(commitNotification);
			delete commitNotification;
			return true;
			break;
		case ShardingCommitACKMessageType:
			CommitNotification::ACK * commitNotificationAck = CommitNotification::ACK::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(commitNotificationAck);
			delete commitNotificationAck;
			return true;
		case ShardingProposalMessageType:
			ProposalNotification * proposalNotification = ProposalNotification::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(proposalNotification);
			delete proposalNotification;
			return true;
		case ShardingProposalOkMessageType:
			ProposalNotification::OK * proposalNotificationOk = ProposalNotification::OK::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(proposalNotificationOk);
			delete proposalNotificationOk;
			return true;
		case ShardingProposalNoMessageType:
			ProposalNotification::NO * proposalNotificationNo = ProposalNotification::NO::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(proposalNotificationNo);
			delete proposalNotificationNo;
			return true;
		case ShardingLockMessageType:
			LockingNotification * lockingNotification = LockingNotification::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(lockingNotification);
			delete lockingNotification;
			return true;
		case ShardingLockGrantedMessageType:
			LockingNotification::GRANTED * lockingNotificationGranted = LockingNotification::GRANTED::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(lockingNotificationGranted);
			delete lockingNotificationGranted;
			return true;
		case ShardingLockRejectedMessageType:
			LockingNotification::REJECTED * lockingNotificationRejected = LockingNotification::REJECTED::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(lockingNotificationRejected);
			delete lockingNotificationRejected;
			return true;
		case ShardingNewNodeWelcomeMessageType:
			NodeInitNotification::WELCOME * initWelcome = NodeInitNotification::WELCOME::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(initWelcome);
			delete initWelcome;
			return true;
		case ShardingNewNodeBusyMessageType:
			NodeInitNotification::BUSY * initBusy = NodeInitNotification::BUSY::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(initBusy);
			delete initBusy;
			return true;
		case ShardingNewNodeNewHostMessageType:
			NodeInitNotification::NEW_HOST * initNewhost = NodeInitNotification::NEW_HOST::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(initNewhost);
			delete initNewhost;
			return true;
		case ShardingNewNodeShardRequestMessageType:
			NodeInitNotification::SHARD_REQUEST * initReq = NodeInitNotification::SHARD_REQUEST::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(initReq);
			delete initReq;
			return true;
		case ShardingNewNodeShardOfferMessageType:
			NodeInitNotification::SHARD_OFFER * initOffer = NodeInitNotification::SHARD_OFFER::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(initOffer);
			delete initOffer;
			return true;
		case ShardingNewNodeShardsReadyMessageType:
			NodeInitNotification::SHARDS_READY * initReady = NodeInitNotification::SHARDS_READY::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(initReady);
			delete initReady;
			return true;
		case ShardingNewNodeJoinPermitMessageType:
			NodeInitNotification::JOIN_PERMIT * initJoinPermit = NodeInitNotification::JOIN_PERMIT::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(initJoinPermit);
			delete initJoinPermit;
			return true;
		case ShardingCopyToMeMessageType:
			CopyToMeNotification * copyToMeNotification = CopyToMeNotification::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(copyToMeNotification);
			delete copyToMeNotification;
			return true;
		case ShardingMoveToMeMessageType:
			MoveToMeNotification * moveToMeNotification = MoveToMeNotification::deserializeAndConstruct(Message::getBodyPointerFromMessagePointer(msg));
			resolve(moveToMeNotification);
			delete moveToMeNotification;
			return true;
			break;
		default:
			ASSERT(false);
			break;
	}
	return false;
}

void ShardManager::resolveReadviewRelease(unsigned metadataVersion){
	LockManager::getLockManager()->resolveReadviewRelease(metadataVersion);
}


void ShardManager::resolveMMNotification(ShardMigrationStatus migrationStatus){
	LoadBalancer::getLoadBalancer()->resolveMMNotification(migrationStatus);
}

void ShardManager::resolveSMNodeArrival(const Node & newNode){
	NodeInitializer::getNodeInitializer()->resolveSMNodeArrival(newNode);
}

void resolveSMNodeFailure(const NodeId failedNodeId){
	// TODO : what should we do upon a node failure ?
}


void ShardManager::updateNodeLatestChange(unsigned newChangeId, NodeId senderNodeId){
	if(latestChangesReceived.find(senderNodeId) == latestChangesReceived.end()){
		latestChangesReceived[senderNodeId] = newChangeId;
		return;
	}
	if(latestChangesReceived[senderNodeId] > newChangeId){
		ASSERT(false);
		return;
	}
	latestChangesReceived[senderNodeId] = newChangeId;
}



void ShardManager::resolve(CommitNotification * commitNotification){
	if(LoadBalancer::getLoadBalancer()->doesExpect(commitNotification)){
		LoadBalancer::getLoadBalancer()->resolve(commitNotification);
	}else if(NodeInitializer::getNodeInitializer()->doesExpect(commitNotification)){
		NodeInitializer::getNodeInitializer()->resolve(commitNotification);
	}else{
		MetadataManager::getMetadataManager()->resolve(commitNotification);
		// delete the change in here
		delete commitNotification->getMetadataChange();
	}
}
void ShardManager::resolve(CommitNotification::ACK * commitAckNotification){
	LoadBalancer::getLoadBalancer()->resolve(commitAckNotification);
	NodeInitializer::getNodeInitializer()->resolve(commitAckNotification);
}
void ShardManager::resolve(LockingNotification * lockingNotification){
	LockManager::getLockManager()->resolve(lockingNotification);
}
void ShardManager::resolve(LockingNotification::GRANTED * granted){
	LockManager::getLockManager()->resolve(granted);
}
void ShardManager::resolve(LockingNotification::REJECTED * rejected){
	LockManager::getLockManager()->resolve(rejected);
}
void ShardManager::resolve(NodeInitNotification::WELCOME * welcome){
	NodeInitializer::getNodeInitializer()->resolve(welcome);
}
void ShardManager::resolve(NodeInitNotification::BUSY * busy){
	NodeInitializer::getNodeInitializer()->resolve(busy);
}
void ShardManager::resolve(NodeInitNotification::NEW_HOST * newHost){
	NodeInitializer::getNodeInitializer()->resolve(newHost);
}
void ShardManager::resolve(NodeInitNotification::SHARD_REQUEST * shardRequest){
	NodeInitializer::getNodeInitializer()->resolve(shardRequest);
}
void ShardManager::resolve(NodeInitNotification::SHARD_OFFER * shardOffer){
	NodeInitializer::getNodeInitializer()->resolve(shardOffer);
}
void ShardManager::resolve(NodeInitNotification::SHARDS_READY * shardsReady){
	NodeInitializer::getNodeInitializer()->resolve(shardsReady);
}
void ShardManager::resolve(NodeInitNotification::JOIN_PERMIT * joinPermit){
	NodeInitializer::getNodeInitializer()->resolve(joinPermit);
}
void ShardManager::resolve(CopyToMeNotification * copyToMeNotification){
	LoadBalancer::getLoadBalancer()->resolve(copyToMeNotification);
}
void ShardManager::resolve(MoveToMeNotification * moveToMeNotification){
	LoadBalancer::getLoadBalancer()->resolve(moveToMeNotification);
}
void ShardManager::resolve(ProposalNotification * proposal){
	LoadBalancer::getLoadBalancer()->resolve(proposal);
}
void ShardManager::resolve(ProposalNotification::OK * proposalAck){
	LoadBalancer::getLoadBalancer()->resolve(proposalAck);
}
void ShardManager::resolve(ProposalNotification::NO * proposalNo){
	LoadBalancer::getLoadBalancer()->resolve(proposalNo);
}


}
}


