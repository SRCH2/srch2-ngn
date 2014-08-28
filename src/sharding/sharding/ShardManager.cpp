#include "ShardManager.h"


#include "ClusterOperationContainer.h"
#include "metadata_manager/ResourceMetadataManager.h"
#include "metadata_manager/ResourceLocks.h"
#include "metadata_manager/Cluster_Writeview.h"
#include "metadata_manager/Cluster.h"
#include "./ClusterOperationContainer.h"
#include "notifications/Notification.h"
#include "notifications/NewNodeLockNotification.h"
#include "notifications/CommitNotification.h"
#include "notifications/LoadBalancingReport.h"
#include "notifications/LockingNotification.h"
#include "notifications/MetadataReport.h"
#include "notifications/MoveToMeNotification.h"
#include "notifications/CopyToMeNotification.h"
#include "metadata_manager/MetadataInitializer.h"
#include "node_initialization/NewNodeJoinOperation.h"
#include "load_balancer/LoadBalancingStartOperation.h"
#include "load_balancer/ShardMoveOperation.h"

#include "core/util/Assert.h"
#include <pthread.h>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardManager * ShardManager::singleInstance = NULL;

ShardManager * ShardManager::createShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager){
	if(singleInstance != NULL){
		return singleInstance;
	}
	// only shard manager must be singleton. ConfigManager must be accessed from shard manager
	singleInstance = new ShardManager(configManager, metadataManager);
	return singleInstance;
}

void ShardManager::deleteShardManager(){
	if(singleInstance == NULL){
		return ;
	}
	delete singleInstance;
	singleInstance = NULL;
	return;
}

ShardManager * ShardManager::getShardManager(){
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	return singleInstance;
}

NodeId ShardManager::getCurrentNodeId(){
	return ShardManager::getWriteview()->currentNodeId;
}
Cluster_Writeview * ShardManager::getWriteview(){
	return ShardManager::getShardManager()->getMetadataManager()->getClusterWriteview();
}
void ShardManager::getReadview(boost::shared_ptr<const ClusterResourceMetadata_Readview> & readview) {
	ShardManager::getShardManager()->getMetadataManager()->getClusterReadView(readview);
}

ShardManager::ShardManager(ConfigManager * configManager,ResourceMetadataManager * metadataManager){

	this->configManager = configManager;
	this->metadataManager = metadataManager;
	this->lockManager = new ResourceLockManager();
	this->stateMachine = new ClusterOperationStateMachine();
	this->joinedFlag = false;
	this->cancelledFlag = false;

}

void ShardManager::attachToTransportManager(TransportManager * tm){
	this->transportManager = tm;
	this->transportManager->registerCallbackForShardingMessageHandler(this);
}

ShardManager::~ShardManager(){
	boost::unique_lock<boost::mutex> bouncedNotificationsLock(shardManagerGlobalMutex);
	setCancelled();
}


TransportManager * ShardManager::getTransportManager() const{
	return transportManager;
}

ConfigManager * ShardManager::getConfigManager() const{
	return configManager;
}

ResourceMetadataManager * ShardManager::getMetadataManager() const{
	return metadataManager;
}
ResourceLockManager * ShardManager::getLockManager() const{
	return lockManager;
}

ClusterOperationStateMachine * ShardManager::getStateMachine() const{
	return this->stateMachine;
}

void ShardManager::setJoined(){
    joinedFlag = true;
}

bool ShardManager::isJoined() const{
    return joinedFlag;
}

void ShardManager::setCancelled(){
	this->cancelledFlag = true;
}
bool ShardManager::isCancelled() {
	boost::unique_lock<boost::mutex> bouncedNotificationsLock(shardManagerGlobalMutex);
	return this->cancelledFlag;
}

void ShardManager::setLoadBalancing(){
	this->loadBalancingFlag = true;
}
void ShardManager::resetLoadBalancing(){
	this->loadBalancingFlag = false;
}
bool ShardManager::isLoadBalancing() const{
	return this->loadBalancingFlag;
}

void ShardManager::print(){
	metadataManager->print();

	lockManager->print();

	stateMachine->print();

	// bounced notifications
	cout << "****************************" << endl;
	cout << "Bounced notifications' source addresses : " ;
	if(bouncedNotifications.size() == 0 ){
		cout << "empty." << endl;
	}else{
		cout << endl;
	}
	for(unsigned i = 0; i < bouncedNotifications.size(); ++i){
		const ShardingNotification * notif = bouncedNotifications.at(i);
		cout << notif->getDest().toString() << endl;
	}
	cout << "****************************" << endl;

}

void ShardManager::start(){
	unsigned numberOfNodes = this->metadataManager->getClusterWriteview()->nodes.size();
	if(numberOfNodes == 1){ // we are the first node:
		// assign primary shards to this node :
		MetadataInitializer nodeInitializer(configManager, this->metadataManager);
		nodeInitializer.initializeCluster();
		this->getMetadataManager()->commitClusterMetadata();
		this->setJoined();
	}else{
		// commit the readview to be accessed by readers until we join
		this->getMetadataManager()->commitClusterMetadata();
		// we must join an existing cluster :
		NewNodeJoinOperation * joinOperation = new NewNodeJoinOperation();
		stateMachine->registerOperation(joinOperation);
	}
	pthread_t localLockThread;
    if (pthread_create(&localLockThread, NULL, ShardManager::periodicWork , NULL) != 0){
        //        Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for handling local message");
        return;
    }
}

// sends this sharding notification to destination using TM
bool ShardManager::send(ShardingNotification * notification){
	if(notification == NULL){
		ASSERT(false);
		return false;
	}
	if(notification->getDest().nodeId == getCurrentNodeId()){
		ASSERT(false);
		return true;
	}
	unsigned notificationByteSize = notification->getNumberOfBytes();
	void * bodyByteArray = transportManager->getMessageAllocator()->allocateMessageReturnBody(notificationByteSize);
	notification->serialize(bodyByteArray);
	Message * notificationMessage = Message::getMessagePointerFromBodyPointer(bodyByteArray);
	notificationMessage->setShardingMask();
	notificationMessage->setMessageId(transportManager->getUniqueMessageIdValue());
	notificationMessage->setType(notification->messageType());
	transportManager->sendMessage(notification->getDest().nodeId , notificationMessage, 0);
	transportManager->getMessageAllocator()->deallocateByMessagePointer(notificationMessage);

	// currently TM always send the message
	return true;
}

bool ShardManager::resolveMessage(Message * msg, NodeId senderNode){
	if(msg == NULL){
		return false;
	}

	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
	Cluster_Writeview * writeview = ShardManager::getWriteview();

	// if a node fails, we don't accept any more messages from it.
	if(writeview->nodes.find(senderNode) != writeview->nodes.end() &&
			writeview->nodes[senderNode].first == ShardingNodeStateFailed){
		return true;
	}
	cout << ">>>>>>>>>>>>>>>>>>>>>>>>> Resolve Message : " ;
	switch (msg->getType()) {
		case ShardingNewNodeLockMessageType:
		{

			NewNodeLockNotification * newNodeLockNotification =
					ShardingNotification::deserializeAndConstruct<NewNodeLockNotification>(Message::getBodyPointerFromMessagePointer(msg));
			if(newNodeLockNotification->isBounced()){
				saveBouncedNotification(newNodeLockNotification);
				break;
			}
			if(! isJoined()){
				bounceNotification(newNodeLockNotification);
				break;
			}
			cout << "New node lock msg received from node " << senderNode << endl;
			this->lockManager->resolve(newNodeLockNotification);
			delete newNodeLockNotification;
			break;
		}
		case ShardingNewNodeLockACKMessageType:
		{
			NewNodeLockNotification::ACK * newNodeLockAck =
					ShardingNotification::deserializeAndConstruct<NewNodeLockNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
			if(newNodeLockAck->isBounced()){
				ASSERT(false);
				delete newNodeLockAck;
				break;
			}
			cout << "New node lock ack msg received from node " << senderNode << endl;
			stateMachine->handle(newNodeLockAck);
			delete newNodeLockAck;
			break;
		}
		case ShardingMoveToMeMessageType:
		{
			MoveToMeNotification * moveNotif  =
					ShardingNotification::deserializeAndConstruct<MoveToMeNotification>(Message::getBodyPointerFromMessagePointer(msg));
			if(moveNotif->isBounced()){
				ASSERT(false);
				delete moveNotif;
				break;
			}

			cout << "Migration manager requested to move to node " << senderNode << endl;
			/////////////////////////// TODO : this code is temporarily faking Migration Manager
			ShardMigrationStatus mmStatus;
			mmStatus.sourceNodeId = ShardManager::getCurrentNodeId();
			mmStatus.srcOperationId = 0;
			mmStatus.destinationNodeId = moveNotif->getSrc().nodeId;
			mmStatus.dstOperationId = moveNotif->getSrc().operationId;
			mmStatus.status = MIGRATION_STATUS_FINISH;
			MMNotification * mmNotif = new MMNotification(mmStatus, moveNotif->getShardId());
			send(mmNotif);
			delete mmNotif;
			delete moveNotif;
			break;
		}
		case ShardingMoveToMeStartMessageType:
		{
			MoveToMeNotification::START * moveNotif  =
					ShardingNotification::deserializeAndConstruct<MoveToMeNotification::START>(Message::getBodyPointerFromMessagePointer(msg));
			if(moveNotif->isBounced()){
				saveBouncedNotification(moveNotif);
				break;
			}
			if(! isJoined()){
				bounceNotification(moveNotif);
				break;
			}
			cout << "Shard Move start from node " << senderNode << endl;
			this->stateMachine->registerOperation(new ShardMoveSrcOperation(moveNotif->getSrc(), moveNotif->getShardId()));
			delete moveNotif;
			break;
		}
		case ShardingMoveToMeACKMessageType:
		{
			MoveToMeNotification::ACK * moveAckNotif  =
					ShardingNotification::deserializeAndConstruct<MoveToMeNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
			if(moveAckNotif->isBounced()){
				ASSERT(false);
				delete moveAckNotif;
				break;
			}

			cout << "Shard Move ack from node " << senderNode << endl;
			this->stateMachine->handle(moveAckNotif);
			delete moveAckNotif;
			return true;
		}
		case ShardingMoveToMeAbortMessageType:
		{
			MoveToMeNotification::ABORT * moveAbortNotif  =
					ShardingNotification::deserializeAndConstruct<MoveToMeNotification::ABORT>(Message::getBodyPointerFromMessagePointer(msg));
			if(moveAbortNotif->isBounced()){
				ASSERT(false);
				delete moveAbortNotif;
				break;
			}

			cout << "Shard Move abort from node " << senderNode << endl;
			this->stateMachine->handle(moveAbortNotif);
			delete moveAbortNotif;
			break;
		}
		case ShardingMoveToMeFinishMessageType:
		{
			MoveToMeNotification::FINISH * moveFinishNotif  =
					ShardingNotification::deserializeAndConstruct<MoveToMeNotification::FINISH>(Message::getBodyPointerFromMessagePointer(msg));
			if(moveFinishNotif->isBounced()){
				ASSERT(false);
				delete moveFinishNotif;
				break;
			}

			cout << "Shard Move finish from node " << senderNode << endl;
			this->stateMachine->handle(moveFinishNotif);
			delete moveFinishNotif;
			break;
		}
		case ShardingNewNodeReadMetadataRequestMessageType:
		{
			MetadataReport::REQUEST * readNotif =
					ShardingNotification::deserializeAndConstruct<MetadataReport::REQUEST>(Message::getBodyPointerFromMessagePointer(msg));
			if(readNotif->isBounced()){
				saveBouncedNotification(readNotif);
				break;
			}
			if(! isJoined()){
				bounceNotification(readNotif);
				break;
			}

			cout << "Metadata Report Request from node " << senderNode << endl;
			metadataManager->resolve(readNotif);
			delete readNotif;
			break;
		}
		case ShardingNewNodeReadMetadataReplyMessageType:
		{
			MetadataReport * readAckNotif =
					ShardingNotification::deserializeAndConstruct<MetadataReport>(Message::getBodyPointerFromMessagePointer(msg));
			if(readAckNotif->isBounced()){
				ASSERT(false);
				delete readAckNotif;
				break;
			}

			cout << "Metadata Report, read from node " << senderNode << endl;
			stateMachine->handle(readAckNotif);
			delete readAckNotif;
			break;
		}
		case ShardingLockMessageType:
		{
			LockingNotification * lockNotif =
					ShardingNotification::deserializeAndConstruct<LockingNotification>(Message::getBodyPointerFromMessagePointer(msg));
			if(lockNotif->isBounced()){
				saveBouncedNotification(lockNotif);
				break;
			}
			if(! isJoined()){
				bounceNotification(lockNotif);
				break;
			}

			cout << "Locking request from node " << senderNode << endl;
			lockManager->resolve(lockNotif);
			delete lockNotif;
			break;
		}
		case ShardingLockACKMessageType:
		{
			LockingNotification::ACK * lockAckNotif =
					ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
			if(lockAckNotif->isBounced()){
				ASSERT(false);
				delete lockAckNotif;
				break;
			}

			cout << "Locking request result from node " << senderNode <<
					" which is " << (lockAckNotif->isGranted() ? "granted.":"rejected.") << endl;
			stateMachine->handle(lockAckNotif);
			delete lockAckNotif;
			break;
		}
		case ShardingLockRVReleasedMessageType:
		{
			ASSERT(false);
			break;
		}
		case ShardingLoadBalancingReportMessageType:
		{
			LoadBalancingReport * loadBalancingReportNotif =
					ShardingNotification::deserializeAndConstruct<LoadBalancingReport>(Message::getBodyPointerFromMessagePointer(msg));
			if(loadBalancingReportNotif->isBounced()){
				ASSERT(false);
				delete loadBalancingReportNotif;
				break;
			}

			cout << "Loadbalancing report from node " << senderNode << " which is load = " << loadBalancingReportNotif->getLoad() << endl;
			stateMachine->handle(loadBalancingReportNotif);
			delete loadBalancingReportNotif;
			break;
		}
		case ShardingLoadBalancingReportRequestMessageType:
		{
			LoadBalancingReport::REQUEST * loadBalancingReqNotif =
					ShardingNotification::deserializeAndConstruct<LoadBalancingReport::REQUEST>(Message::getBodyPointerFromMessagePointer(msg));
			if(loadBalancingReqNotif->isBounced()){
				saveBouncedNotification(loadBalancingReqNotif);
				break;
			}
			if(! isJoined()){
				bounceNotification(loadBalancingReqNotif);
				break;
			}

			LoadBalancingReport * report = new LoadBalancingReport(writeview->getLocalNodeTotalLoad());
			report->setDest(loadBalancingReqNotif->getSrc());
			report->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
			cout << "Load balancing report request from node " << senderNode << ", replying with : " << report->getLoad() << endl;
			send(report);
			delete report;
			delete loadBalancingReqNotif;
			break;
		}
		case ShardingCopyToMeMessageType:
		{

			CopyToMeNotification * copyNotif =
					ShardingNotification::deserializeAndConstruct<CopyToMeNotification>(Message::getBodyPointerFromMessagePointer(msg));
			if(copyNotif->isBounced()){
				saveBouncedNotification(copyNotif);
				break;
			}
			if(! isJoined()){
				bounceNotification(copyNotif);
				break;
			}

			cout << "Migration manager requested to copy to node " << senderNode << endl;
			ShardMigrationStatus mmStatus;
			mmStatus.sourceNodeId = ShardManager::getCurrentNodeId();
			mmStatus.srcOperationId = 0;
			mmStatus.destinationNodeId = copyNotif->getSrc().nodeId;
			mmStatus.dstOperationId = copyNotif->getSrc().operationId;
			mmStatus.status = MIGRATION_STATUS_FINISH;
			MMNotification * mmNotif = new MMNotification(mmStatus, copyNotif->getDestShardId());
			send(mmNotif);
			delete mmNotif;
			delete copyNotif;
			break;
		}
		case ShardingMMNotificationMessageType:
		{
			// TODO : Only happens in test because CopyToMeNotification and its ack work instead of migration manager
			MMNotification * mmNotif =
					ShardingNotification::deserializeAndConstruct<MMNotification>(Message::getBodyPointerFromMessagePointer(msg));
			if(mmNotif->isBounced()){
				ASSERT(false);
				delete mmNotif;
				break;
			}

			cout << "Migration manager local status notification " << senderNode << endl;
			ShardMigrationStatus mmStatus = mmNotif->getStatus();
			// add empty shard to it...
			ClusterShardId * destShardId = new ClusterShardId(mmNotif->getDestShardId());
			// prepare indexDirectory
	        string indexDirectory = configManager->getShardDir(writeview->clusterName,
	                writeview->nodes[ShardManager::getCurrentNodeId()].second->getName(), writeview->cores[destShardId->coreId]->getName(), destShardId);
	        if(indexDirectory.compare("") == 0){
	            indexDirectory = configManager->createShardDir(writeview->clusterName,
	                    writeview->nodes[ShardManager::getCurrentNodeId()].second->getName(), writeview->cores[destShardId->coreId]->getName(), destShardId);
	        }
			EmptyShardBuilder emptyShard(destShardId, indexDirectory);
			emptyShard.prepare();
			mmStatus.shard = emptyShard.getShardServer();
			mmNotif->setStatus(mmStatus);
			this->stateMachine->handle(mmNotif);
			delete mmNotif;
			break;
		}
		case ShardingCommitMessageType:
		{
			CommitNotification * commitNotif =
					ShardingNotification::deserializeAndConstruct<CommitNotification>(Message::getBodyPointerFromMessagePointer(msg));
			if(commitNotif->isBounced()){
				saveBouncedNotification(commitNotif);
				break;
			}
			if(! isJoined()){
				bounceNotification(commitNotif);
				break;
			}

			cout << "Commit request received from node " << senderNode << endl;

			metadataManager->resolve(commitNotif);
			delete commitNotif;
			break;
		}
		case ShardingCommitACKMessageType:
		{
			CommitNotification::ACK * commitAckNotif =
					ShardingNotification::deserializeAndConstruct<CommitNotification::ACK>(Message::getBodyPointerFromMessagePointer(msg));
			if(commitAckNotif->isBounced()){
				ASSERT(false);
				delete commitAckNotif;
				break;
			}

			cout << "Commit request ACK received from node " << senderNode << endl;
			stateMachine->handle(commitAckNotif);
			delete commitAckNotif;
			break;
		}
		default:
			break;
	}

//    cout << "Shard Manager status after handling message:" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;

	return true;
}

void ShardManager::resolve(LockingNotification::ACK * lockAckNotif){
	stateMachine->handle(lockAckNotif);
	cout << ">>>>>>>>>>>>>>>>>>>>>>>>> Resolve Message : " ;
	cout << "Internal lock ack received." << endl;
}

void ShardManager::resolveReadviewRelease(unsigned metadataVersion){
	LockingNotification::RV_RELEASED * rvReleased = new LockingNotification::RV_RELEASED(metadataVersion);
	this->getLockManager()->resolve(rvReleased);
	delete rvReleased;

//    cout << "Shard Manager status after receiving RV release, vid = " << metadataVersion << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::resolveMMNotification(const ShardMigrationStatus & migrationStatus){
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
	// TODO : second argument must be deleted when migration manager is merged with this code.
	// TODO :  Migration manager must return the ClusterShardId value
	MMNotification * mmNotif = new MMNotification(migrationStatus, ClusterShardId());
	this->stateMachine->handle(mmNotif);

//    cout << "Shard Manager status after receiving migration manager notification:" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::resolveSMNodeArrival(const Node & newNode){
    boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
    Cluster_Writeview * writeview = this->getWriteview();
    writeview->addNode(newNode);

//    cout << "Shard Manager status after arrival of node " << newNode.getId() << ":" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
};

void ShardManager::resolveSMNodeFailure(const NodeId failedNodeId){
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
	NodeFailureNotification * nodeFailureNotif = new NodeFailureNotification(failedNodeId);
	// 1. metadata manager
	this->metadataManager->resolve(nodeFailureNotif);
	// 2. lock manager
	this->lockManager->resolve(nodeFailureNotif);
	// 3. state machine
	this->stateMachine->handle(nodeFailureNotif);


	cout << "XXXXXXXXXXXXX Shard Manager status after handling node " << failedNodeId << " failure:" << endl;
    ShardManager::getShardManager()->print();
    cout << "======================================================================" << endl;
}


void * ShardManager::periodicWork(void *args) {

	while(! ShardManager::getShardManager()->isCancelled()){

		/*
		 * 1. Resend bounced notifications.
		 * 2. is we are joined, start load balancing.
		 */
		//
		sleep(10);

		boost::unique_lock<boost::mutex> shardManagerGlobalLock(ShardManager::getShardManager()->shardManagerGlobalMutex);

		// 1. Resend bounced notifications.
		for(unsigned i = 0 ; i < ShardManager::getShardManager()->bouncedNotifications.size() ; ++i){
			ShardingNotification * notif = ShardManager::getShardManager()->bouncedNotifications.at(i);
			ShardManager::getShardManager()->send(notif);
		}
		ShardManager::getShardManager()->bouncedNotifications.clear();


		// 2. if we are joined, start load balancing.
		if(ShardManager::getShardManager()->isJoined() && ! ShardManager::getShardManager()->isLoadBalancing()){
			ShardManager::getShardManager()->setLoadBalancing();
			ShardManager::getShardManager()->stateMachine->registerOperation(new LoadBalancingStartOperation());
		}
	    ShardManager::getShardManager()->print();
	    cout << "======================================================================" << endl;
	}
	return NULL;
}

void ShardManager::saveBouncedNotification(ShardingNotification * notif){
	notif->resetBounced();
	notif->swapSrcDest();
	bouncedNotifications.push_back(notif);
}

void ShardManager::bounceNotification(ShardingNotification * notif){
	notif->setBounced();
	notif->swapSrcDest();
	send(notif);
	delete notif;
}

}
}


