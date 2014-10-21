#include "ShardManager.h"


#include "./state_machine/StateMachine.h"
#include "./metadata_manager/ResourceMetadataManager.h"
#include "./metadata_manager/ResourceLocks.h"
#include "./lock_manager/LockManager.h"
#include "./metadata_manager/Cluster_Writeview.h"
#include "./metadata_manager/Cluster.h"
#include "./metadata_manager/MetadataInitializer.h"
#include "./cluster_operations/ClusterSaveOperation.h"
#include "./cluster_operations/ClusterShutdownOperation.h"
#include "./notifications/Notification.h"
#include "./notifications/NewNodeLockNotification.h"
#include "./notifications/CommitNotification.h"
#include "./notifications/LoadBalancingReport.h"
#include "./notifications/LockingNotification.h"
#include "./notifications/MetadataReport.h"
#include "./notifications/MoveToMeNotification.h"
#include "./notifications/CopyToMeNotification.h"
#include "./transactions/cluster_transactions/LoadBalancer.h"
#include "./transactions/ShardMoveOperation.h"
#include "sharding/migration/MigrationManager.h"

#include "core/util/Assert.h"
#include <pthread.h>
#include <signal.h>

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

StateMachine * ShardManager::getStateMachine(){
	return  ShardManager::getShardManager()->_getStateMachine();
}

ShardManager::ShardManager(ConfigManager * configManager,ResourceMetadataManager * metadataManager){

	this->configManager = configManager;
	this->metadataManager = metadataManager;
	this->lockManager = new ResourceLockManager();
	this->_lockManager = new LockManagerExternalInterface;
	this->stateMachine = new StateMachine();
	this->recordOperationsStateMachine = new StateMachine();
	this->joinedFlag = false;
	this->cancelledFlag = false;
	this->loadBalancingThread = new pthread_t;

}

void ShardManager::attachToTransportManager(TransportManager * tm){
	this->transportManager = tm;
	this->transportManager->registerCallbackForShardingMessageHandler(this);
}

void ShardManager::initFirstNode(){
	// assign primary shards to this node :
	MetadataInitializer nodeInitializer(configManager, this->metadataManager);
	nodeInitializer.initializeCluster();
	this->getMetadataManager()->commitClusterMetadata();
	this->setJoined();
}

ShardManager::~ShardManager(){
	setCancelled();
	delete this->loadBalancingThread;
	delete this->_lockManager;
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
LockManagerExternalInterface * ShardManager::_getLockManager() const{
	return _lockManager;
}

StateMachine * ShardManager::_getStateMachine() const{
	return this->stateMachine;
}

StateMachine * ShardManager::getRecordOperationsStateMachine() const{
	return this->recordOperationsStateMachine;
}

MigrationManager * ShardManager::getMigrationManager() const{
	return this->migrationManager;
}

void ShardManager::initMigrationManager(){
	this->migrationManager = new MigrationManager(transportManager, configManager);
}

void ShardManager::setJoined(){
    joinedFlag = true;
}

bool ShardManager::isJoined() const{
    return joinedFlag;
}

void ShardManager::setCancelled(){
	boost::unique_lock<boost::mutex> bouncedNotificationsLock(shardManagerGlobalMutex);
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

pthread_t * ShardManager::getLoadbalancingThread() {
	return this->loadBalancingThread;
}

void ShardManager::print(){
	metadataManager->print();

	lockManager->print();

	stateMachine->print();

	recordOperationsStateMachine->print();

	// bounced notifications
	if(bouncedNotifications.size() == 0){
		return;
	}
	cout << "**************************************************************************************************" << endl;
	cout << "Bounced notifications' source addresses : " ;
	if(bouncedNotifications.size() == 0 ){
		cout << "empty." << endl;
	}else{
		cout << endl;
	}
	cout << "**************************************************************************************************" << endl;
	for(unsigned i = 0; i < bouncedNotifications.size(); ++i){
		const ShardingNotification * notif = bouncedNotifications.at(i);
		cout << notif->getDest().toString() << endl;
	}

}

void ShardManager::start(){
	Logger::info("Starting data processor ...");
	unsigned numberOfNodes = this->metadataManager->getClusterWriteview()->nodes.size();
	if(numberOfNodes == 1){ // we are the first node:
		// assign primary shards to this node :
		initFirstNode();

		Logger::info("Cluster is ready to accept new nodes. Current node ID : %d", ShardManager::getCurrentNodeId());
		//TODO remove
		print();
	}else{
		// commit the readview to be accessed by readers until we join
		this->getMetadataManager()->commitClusterMetadata();
		Logger::info("Joining the existing cluster ...");
		//TODO remove
		Logger::info("Printing node information before join ...");
		print();

		// we must join an existing cluster :
		NodeJoiner::run();
	}
    if (pthread_create(loadBalancingThread, NULL, ShardManager::periodicWork , NULL) != 0){
        //        Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for load balancing.");
        return;
    }
}

/*
 * Saves the cluster metadata and indices ...
 */
void ShardManager::save(evhttp_request *req){
	boost::shared_ptr<HTTPJsonShardOperationResponse > brokerSideInformationJson =
			boost::shared_ptr<HTTPJsonShardOperationResponse > (new HTTPJsonShardOperationResponse(req));
	if(this->metadataManager->getClusterWriteview() == NULL){
		brokerSideInformationJson->finalizeOK();
		brokerSideInformationJson->addError(HTTPJsonResponse::getJsonSingleMessage(HTTP_JSON_Cluster_Not_Ready_Error));
		brokerSideInformationJson->addShardResponse(c_action_save, false, nullJsonValue);
		return;
	}


    /* Yes, we are expecting a post request */
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
    	ClusterSaveOperation * saveOperation = new ClusterSaveOperation(brokerSideInformationJson);
    	this->stateMachine->registerOperation(saveOperation);
        break;
    }
    default: {
        brokerSideInformationJson->finalizeInvalid();
        break;
    }
    };
}

/*
 * Shuts the cluster down.
 * In this process, we first save all the indices and the cluster metadata and then shut the entire cluster down.
 */
void ShardManager::shutdown(evhttp_request *req){
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
    switch (req->type) {
    case EVHTTP_REQ_PUT: {
    	ClusterShutdownOperation * shutdownOperation = new ClusterShutdownOperation();
    	this->stateMachine->registerOperation(shutdownOperation);
		bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
				"{\"message\":\"Shutting down the cluster...\"}\n");
        break;
    }
    default: {
        bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST",
                "{\"error\":\"The request has an invalid or missing argument. See Srch2 API documentation for details.\"}");
        Logger::error(
                "The request has an invalid or missing argument. See Srch2 API documentation for details");
        break;
    }
    };
}

void ShardManager::insert(const unsigned coreId , evhttp_request *req){
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
	ClusterRecordOperation * insertOperation = new ClusterRecordOperation(Insert_ClusterRecordOperation_Type, coreId, req);
	this->recordOperationsStateMachine->registerOperation(insertOperation);
	return;
}

void ShardManager::_shutdown(){
	Logger::console("Shutting down the instance ...");
	raise(SIGTERM);
//    kill(getpid(),SIGTERM);
//	exit(0);
	//TODO
}


/// TODO : not needed now, later when we refactor DP, this function will
///        evolve to the info() request.
void ShardManager::nodesInfo(evhttp_request *req){
	boost::shared_ptr<const ClusterResourceMetadata_Readview> readview;
	this->getReadview(readview);

	HTTPJsonResponse httpResponse(req);

	httpResponse.setResponseAttribute(c_cluster_name, Json::Value(readview->getClusterName()));
	Json::Value nodes(Json::arrayValue);
	this->getNodeInfoJson(nodes);
	httpResponse.setResponseAttribute(c_nodes, nodes);

	httpResponse.finalizeOK();

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

	Logger::debug("%s | Sending [Type: %s, ID: %d] ... ", notification->getDescription().c_str(),
			notificationMessage->getDescription().c_str(), notificationMessage->getMessageId());

	transportManager->sendMessage(notification->getDest().nodeId , notificationMessage, 0);
	transportManager->getMessageAllocator()->deallocateByMessagePointer(notificationMessage);

	// currently TM always send the message
	return true;
}

bool ShardManager::handleBouncing(ShardingNotification * notif){
	if(notif->isBounced()){
		saveBouncedNotification(notif);
		Logger::debug("==> Bounced.");
		return true;
	}
	if(! isJoined()){
		bounceNotification(notif);
		Logger::debug("==> Not joined yet. Bounced.");
		return true;
	}
	return false;
}

bool ShardManager::resolveMessage(Message * msg, NodeId senderNode){
	if(msg == NULL){
		return false;
	}

	// Global mutex lock which will last as long as we are in this scope.
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);


	Cluster_Writeview * writeview = ShardManager::getWriteview();

	// if a node fails, we don't accept any more messages from it.
	if(writeview->nodes.find(senderNode) != writeview->nodes.end() &&
			writeview->nodes[senderNode].first == ShardingNodeStateFailed){
		Logger::debug("!!! Warning: Message with type %s was", msg->getDescription().c_str());
		Logger::debug("ignored because source node had failed before.!!!");
		return true;
	}
	Logger::debug("SHM | Type :  %s , MsgID : %d . Going to be processed ...", msg->getDescription().c_str() , msg->getMessageId());
	bool isBouncingActive = false;
	switch (msg->getType()) {
		case ShardingNewNodeReadMetadataRequestMessageType:
			isBouncingActive = MetadataReport::REQUEST::resolveMessage(msg, senderNode);
			break;
		case ShardingNewNodeReadMetadataReplyMessageType:
			isBouncingActive = MetadataReport::resolveMessage(msg, senderNode);
			break;
		case ShardingNewNodeLockMessageType:
			isBouncingActive = NewNodeLockNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingNewNodeLockACKMessageType:
			isBouncingActive = NewNodeLockNotification::ACK::resolveMessage(msg, senderNode);
			break;
		case ShardingLockMessageType:
			isBouncingActive = LockingNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingLockACKMessageType:
			isBouncingActive = LockingNotification::ACK::resolveMessage(msg, senderNode);
			break;
		case ShardingLockRVReleasedMessageType:
			ASSERT(false);
			break;
		case ShardingLoadBalancingReportMessageType:
			isBouncingActive = LoadBalancingReport::resolveMessage(msg, senderNode);
			break;
		case ShardingLoadBalancingReportRequestMessageType:
			isBouncingActive = LoadBalancingReport::REQUEST::resolveMessage(msg,senderNode);
			break;
		case ShardingMoveToMeMessageType:
			isBouncingActive = MoveToMeNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingMoveToMeStartMessageType:
			isBouncingActive = MoveToMeNotification::START::resolveMessage(msg, senderNode);
			break;
		case ShardingMoveToMeACKMessageType:
			isBouncingActive = MoveToMeNotification::ACK::resolveMessage(msg, senderNode);
			break;
		case ShardingMoveToMeAbortMessageType:
			isBouncingActive = MoveToMeNotification::ABORT::resolveMessage(msg, senderNode);
			break;
		case ShardingMoveToMeFinishMessageType:
			isBouncingActive = MoveToMeNotification::FINISH::resolveMessage(msg, senderNode);
			break;
		case ShardingCopyToMeMessageType:
			isBouncingActive = CopyToMeNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingCommitMessageType:
			isBouncingActive = CommitNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingCommitACKMessageType:
			isBouncingActive = CommitNotification::ACK::resolveMessage(msg, senderNode);
			break;
		case ShardingSaveDataMessageType:
			isBouncingActive = SaveDataNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingSaveDataACKMessageType:
			isBouncingActive = SaveDataNotification::ACK::resolveMessage(msg, senderNode);
			break;
		case ShardingSaveMetadataMessageType:
			isBouncingActive = SaveMetadataNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingSaveMetadataACKMessageType:
			isBouncingActive = SaveMetadataNotification::ACK::resolveMessage(msg, senderNode);
			break;
		case ShardingMergeMessageType:
			isBouncingActive = MergeNotification::resolveMessage(msg, senderNode);
			break;
		case ShardingMergeACKMessageType:
			isBouncingActive = MergeNotification::ACK::resolveMessage(msg, senderNode);
			break;
		case ShardingShutdownMessageType:
			isBouncingActive = ShutdownNotification::resolveMessage(msg, senderNode);
			break;
		default:
			ASSERT(false);
			break;
	}

	Logger::debug("SHM | Type :  %s , MsgID : %d . Processed.", msg->getDescription().c_str() , msg->getMessageId());
//    cout << "Shard Manager status after handling message:" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;

	return true;
}

void * ShardManager::resolveReadviewRelease_ThreadChange(void * vidPtr){
	unsigned metadataVersion = *(unsigned *)vidPtr;
	delete (unsigned *)vidPtr;
	Logger::debug("DP | Metadata release VID=%d", metadataVersion);
	ShardManager::getShardManager()->_getLockManager()->resolveReadviewRelease(metadataVersion);
	Logger::debug("DP | Metadata release VID=%d processed.", metadataVersion);

//    cout << "Shard Manager status after receiving RV release, vid = " << metadataVersion << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;

	return NULL;
}

void ShardManager::resolveMMNotification(const ShardMigrationStatus & migrationStatus){

	Logger::debug("MM | (%d => %d) was %s", migrationStatus.sourceNodeId, migrationStatus.destinationNodeId,
			(migrationStatus.status == MM_STATUS_SUCCESS)? "Done."  : "Failed.");
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);

	if(mmSessionListeners.find(migrationStatus.srcOperationId) == mmSessionListeners.end()){
		ASSERT(false);
		return;
	}
	mmSessionListeners.find(migrationStatus.srcOperationId)->second->receiveStatus(migrationStatus);
	this->stateMachine->removeTransaction(mmSessionListeners.find(migrationStatus.srcOperationId)->second->getTransIdToDelete());
	Logger::debug("MM | (%d => %d) was %s Processed.", migrationStatus.sourceNodeId, migrationStatus.destinationNodeId,
			(migrationStatus.status == MM_STATUS_SUCCESS)? "Done."  : "Failed.");
//    cout << "Shard Manager status after receiving migration manager notification:" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::registerMMSessionListener(const unsigned operationId, ProducerInterface * listener){
	if(listener == NULL){
		ASSERT(false);
		return;
	}
	if(mmSessionListeners.find(operationId) != mmSessionListeners.end()){
		ASSERT(false);
		return;
	}
	mmSessionListeners[operationId] = listener;
}

void ShardManager::resolveSMNodeArrival(const Node & newNode){
	Logger::debug("SM | Node %d arrival.", newNode.getId());
    boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
    Cluster_Writeview * writeview = this->getWriteview();
    writeview->addNode(newNode);
	Logger::debug("SM | Node %d arrival. Processed.", newNode.getId());
//    cout << "Shard Manager status after arrival of node " << newNode.getId() << ":" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::resolveSMNodeFailure(const NodeId failedNodeId){
	Logger::debug("Node %d failure.", failedNodeId);
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
	NodeFailureNotification * nodeFailureNotif = new NodeFailureNotification(failedNodeId);
	// 1. metadata manager
	this->metadataManager->resolve(nodeFailureNotif);
	// 2. lock manager
	this->lockManager->resolve(nodeFailureNotif);
	// 3. state machine
	this->stateMachine->handle(nodeFailureNotif);
	Logger::debug("Node %d failure. Processed.", failedNodeId);

}

void ShardManager::resolve(SaveDataNotification * saveDataNotif){
	// Move on all local shards and save them on the disk.
	Cluster_Writeview * writeview = this->getWriteview();
	for(map<ClusterShardId, LocalPhysicalShard >::iterator localClusterShardItr = writeview->localClusterDataShards.begin();
			localClusterShardItr != writeview->localClusterDataShards.end(); ++localClusterShardItr){
		localClusterShardItr->second.server->getIndexer()->save();
	}

	for(map<unsigned,  LocalPhysicalShard >::iterator localNodeShardItr = writeview->localNodeDataShards.begin();
			localNodeShardItr != writeview->localNodeDataShards.end(); ++localNodeShardItr){
		localNodeShardItr->second.server->getIndexer()->save();
	}

	// reply ack
	SaveDataNotification::ACK * ack = new SaveDataNotification::ACK();
	ack->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
	ack->setDest(saveDataNotif->getSrc());
	if(ack->getDest().nodeId == ShardManager::getCurrentNodeId()){
		this->stateMachine->handle(ack);
	}else{
		send(ack);
	}
	delete ack;
}

void ShardManager::resolve(SaveMetadataNotification * saveDataNotif){

	this->metadataManager->resolve(this->getConfigManager(), saveDataNotif);

	// reply ack
	SaveMetadataNotification::ACK * ack = new SaveMetadataNotification::ACK();
	ack->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
	ack->setDest(saveDataNotif->getSrc());
	if(ack->getDest().nodeId == ShardManager::getCurrentNodeId()){
		this->stateMachine->handle(ack);
	}else{
		send(ack);
	}
	delete ack;
}

void ShardManager::resolve(MergeNotification * mergeNotification){
	//TODO : merge can be unsuccessful. What should we do in that case ?
	// Move on all local shards and merge them.
	Cluster_Writeview * writeview = this->getWriteview();
	for(map<ClusterShardId, LocalPhysicalShard >::iterator localClusterShardItr = writeview->localClusterDataShards.begin();
			localClusterShardItr != writeview->localClusterDataShards.end(); ++localClusterShardItr){
		localClusterShardItr->second.server->getIndexer()->merge();
	}

	for(map<unsigned,  LocalPhysicalShard >::iterator localNodeShardItr = writeview->localNodeDataShards.begin();
			localNodeShardItr != writeview->localNodeDataShards.end(); ++localNodeShardItr){
		localNodeShardItr->second.server->getIndexer()->merge();
	}


	// reply ack
	MergeNotification::ACK * ack = new MergeNotification::ACK();
	ack->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
	ack->setDest(mergeNotification->getSrc());
	if(ack->getDest().nodeId == ShardManager::getCurrentNodeId()){
		stateMachine->handle(ack);
	}else{
		send(ack);
	}
	delete ack;
}


bool ShardManager::resolveLocal(ShardingNotification * request){
	if(request == NULL){
		ASSERT(false);
		return false;
	}
	if(request->getDest().nodeId != ShardManager::getCurrentNodeId()){
		ASSERT(false);
		return false;
	}
	// NOTE : we must detach the local notif handling thread here. And this must be the ONLY detachment point.
	pthread_t localThread;
    if (pthread_create(&localThread, NULL, _resolveLocal , (void *)request) != 0){
        // Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for handling local message");
        return false;
    }
    pthread_detach(localThread);
    return true;
}

void * ShardManager::_resolveLocal(void * arg){
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(ShardManager::getShardManager()->shardManagerGlobalMutex);
	ShardingNotification * request = (ShardingNotification *)arg;
	switch (request->messageType()) {
		case ShardingLockMessageType:
			LockingNotification::resolveNotif((LockingNotification *)request);
			break;
		case ShardingLockACKMessageType:
			LockingNotification::ACK::resolveNotif((LockingNotification::ACK *)request);
			break;
		case ShardingCommitMessageType:
		{
			MetadataChange * metadataChange = ((CommitNotification *)request)->getMetadataChange();
			ShardManager::getShardManager()->getMetadataManager()->applyAndCommit(metadataChange);
			break;
		}
		case ShardingMergeMessageType:
		{
			ShardManager::getShardManager()->resolve((MergeNotification *)request);
			break;
		}
		default:
			break;
	}
	return NULL;
}


void * ShardManager::periodicWork(void *args) {

	while(! ShardManager::getShardManager()->isCancelled()){

		/*
		 * 1. Resend bounced notifications.
		 * 2. is we are joined, start load balancing.
		 */
		//
		sleep(2);

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
			ShardManager::getShardManager()->stateMachine->registerOperation(new LoadBalancer());
		}

		// TODO remove
		cout << "===========================================================================================" << endl;
		cout << "===========================================================================================" << endl;
		cout << "===========================================================================================" << endl;
		cout << "===========================================================================================" << endl;
		cout << "===========================================================================================" << endl;
	    ShardManager::getShardManager()->print();
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

void ShardManager::getNodeInfoJson(Json::Value & nodeInfo){

	if(nodeInfo == nullJsonValue){
		nodeInfo = Json::Value(Json::arrayValue);
	}
	if(nodeInfo.type() != Json::arrayValue){
		ASSERT(false);
		return;
	}
	boost::shared_ptr<const ClusterResourceMetadata_Readview> readview;
	this->getReadview(readview);
	vector<Node> nodes;
	readview->getAllNodes(nodes);
	for(unsigned i = 0; i < nodes.size(); ++i){
		nodeInfo[i] = Json::Value(Json::objectValue);
		nodeInfo[i][c_node_name] = nodes.at(i).getName();
		nodeInfo[i][c_node_listening_host_name] = nodes.at(i).getIpAddress();
	}

}

}
}


