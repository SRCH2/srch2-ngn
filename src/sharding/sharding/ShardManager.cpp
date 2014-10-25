#include "ShardManager.h"


#include "./state_machine/StateMachine.h"
#include "./state_machine/ConsumerProducer.h"
#include "./metadata_manager/ResourceMetadataManager.h"
#include "./lock_manager/LockManager.h"
#include "./lock_manager/LockManager.h"
#include "./metadata_manager/Cluster_Writeview.h"
#include "./metadata_manager/Cluster.h"
#include "./metadata_manager/MetadataInitializer.h"
#include "./transactions/cluster_transactions/ClusterShutdownOperation.h"
#include "./notifications/Notification.h"
#include "./notifications/CommitNotification.h"
#include "./notifications/LoadBalancingReport.h"
#include "./notifications/LockingNotification.h"
#include "./notifications/MetadataReport.h"
#include "./notifications/MoveToMeNotification.h"
#include "./notifications/CopyToMeNotification.h"
#include "./notifications/CommandNotification.h"
#include "./transactions/cluster_transactions/LoadBalancer.h"
#include "./transactions/ShardMoveOperation.h"
#include "./transactions/cluster_transactions/NodeJoiner.h"
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
	this->_lockManager = new LockManager();
	this->stateMachine = new StateMachine();
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
LockManager * ShardManager::_getLockManager() const{
	return _lockManager;
}

StateMachine * ShardManager::_getStateMachine() const{
	return this->stateMachine;
}

MigrationManager * ShardManager::getMigrationManager() const{
	return this->migrationManager;
}

void ShardManager::setDPInternal(DPInternalRequestHandler * dpInternal){
	this->dpInternal = dpInternal;
}
DPInternalRequestHandler * ShardManager::getDPInternal() const{
	return this->dpInternal;
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

//	lockManager->print(); //TODO

	stateMachine->print();

//	recordOperationsStateMachine->print();

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
		SP(ShardingNotification) notif = bouncedNotifications.at(i);
		cout << notif->getDest().toString() << endl;
	}

}

void ShardManager::start(){
	Logger::info("Starting data processor ...");

	// insert this node and all older nodes in the lock manager in the beginning.
	ShardManager::getShardManager()->_lockManager->initialize();
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
		Logger::debug("Printing node information before join ...");
		print();

		// we must join an existing cluster :
		NodeJoiner::join();
	}
    if (pthread_create(loadBalancingThread, NULL, ShardManager::periodicWork , NULL) != 0){
        //        Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for load balancing.");
        return;
    }
}

void ShardManager::insert(const unsigned coreId , evhttp_request *req){
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(shardManagerGlobalMutex);
//	ClusterRecordOperation * insertOperation = new ClusterRecordOperation(Insert_ClusterRecordOperation_Type, coreId, req);
//	this->recordOperationsStateMachine->registerOperation(insertOperation);
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

	JsonResponseHandler httpResponse(req);

	httpResponse.setResponseAttribute(c_cluster_name, Json::Value(readview->getClusterName()));
	Json::Value nodes(Json::arrayValue);
	this->getNodeInfoJson(nodes);
	httpResponse.setResponseAttribute(c_nodes, nodes);

	httpResponse.finalizeOK();

}


bool ShardManager::handleBouncing(SP(ShardingNotification) notif){
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
	// deserialize sharding header information
	NodeOperationId srcAddress, destAddress;
	bool bounced;
	ShardingNotification::deserializeHeader(msg, senderNode, srcAddress, destAddress, bounced);
	// deserialize to get notifications
	bool isBouncingActive = false;
	SP(ShardingNotification) notif;
	switch (msg->getType()) {
		case ShardingShardCommandMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CommandNotification>(msg);
			break;
		case ShardingNewNodeReadMetadataRequestMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MetadataReport::REQUEST>(msg);
			break;
		case ShardingNewNodeReadMetadataReplyMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MetadataReport>(msg);
			break;
		case ShardingLockMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LockingNotification>(msg);
			break;
		case ShardingLockACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(msg);
			break;
		case ShardingCommitMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CommitNotification>(msg);
			break;
		case ShardingCommitACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CommitNotification::ACK>(msg);
			break;
		case ShardingLoadBalancingReportMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LoadBalancingReport>(msg);
			break;
		case ShardingLoadBalancingReportRequestMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LoadBalancingReport::REQUEST>(msg);
			break;
		case ShardingMoveToMeMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification>(msg);
			break;
		case ShardingMoveToMeACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::ACK>(msg);
			break;
		case ShardingMoveToMeCleanupMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::CleanUp>(msg);
			break;
		case ShardingCopyToMeMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CopyToMeNotification>(msg);
			break;
		case ShardingShutdownMessageType:
			notif = ShardingNotification::deserializeAndConstruct<ShutdownNotification>(msg);
			break;
		default:
			ASSERT(false);
			break;
	}

	if(bounced){
		if(! handleBouncing(notif)){
			Logger::debug("SHM | Bounced notification saved to be sent again later. Notification : %s", notif->getDescription().c_str());
			return true;
		}
	}

	if(! notif->resolveNotification(notif)){
		Logger::debug("SHM | Notification resolve returned false : %s", notif->getDescription().c_str());
	}

	Logger::debug("SHM | Type :  %s , MsgID : %d . Processed.", msg->getDescription().c_str() , msg->getMessageId());

	return true;
}

void * ShardManager::resolveReadviewRelease_ThreadChange(void * vidPtr){
	unsigned metadataVersion = *(unsigned *)vidPtr;
	delete (unsigned *)vidPtr;
	Logger::debug("DP | Metadata release VID=%d", metadataVersion);
	ShardManager::getShardManager()->_getLockManager()->resolve(metadataVersion);
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
		return;
	}
	mmSessionListeners.find(migrationStatus.srcOperationId)->second->consume(migrationStatus);
	if(mmSessionListeners.find(migrationStatus.srcOperationId)->second->getTransaction() != NULL
			&& mmSessionListeners.find(migrationStatus.srcOperationId)->second->getTransaction()->isFinished()){
		delete mmSessionListeners.find(migrationStatus.srcOperationId)->second;
	}
	mmSessionListeners.erase(mmSessionListeners.find(migrationStatus.srcOperationId));

	Logger::debug("MM | (%d => %d) was %s Processed.", migrationStatus.sourceNodeId, migrationStatus.destinationNodeId,
			(migrationStatus.status == MM_STATUS_SUCCESS)? "Done."  : "Failed.");
//    cout << "Shard Manager status after receiving migration manager notification:" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::registerMMSessionListener(const unsigned operationId, ConsumerInterface * listener){
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
	SP(Notification) nodeFailureNotif(new NodeFailureNotification(failedNodeId));
	// 1. metadata manager
	this->metadataManager->resolve(boost::dynamic_pointer_cast<NodeFailureNotification>(nodeFailureNotif));
	// 2. lock manager
	this->_lockManager->resolveNodeFailure(failedNodeId);
	// 3. state machine
	this->stateMachine->handle(nodeFailureNotif);
	Logger::debug("Node %d failure. Processed.", failedNodeId);

}



bool ShardManager::resolveLocal(SP(ShardingNotification) request){
	if(! request){
		ASSERT(false);
		return false;
	}
	if(request->getDest().nodeId != ShardManager::getCurrentNodeId()){
		ASSERT(false);
		return false;
	}
	// NOTE : we must detach the local notif handling thread here. And this must be the ONLY detachment point.
	pthread_t localThread;
	ResolveLocalArgs * args = new ResolveLocalArgs(request);
    if (pthread_create(&localThread, NULL, _resolveLocal , args) != 0){
        // Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for handling local message");
        return false;
    }
    pthread_detach(localThread);
    return true;
}

void * ShardManager::_resolveLocal(void * _args){
	boost::unique_lock<boost::mutex> shardManagerGlobalLock(ShardManager::getShardManager()->shardManagerGlobalMutex);
	ResolveLocalArgs * args = (ResolveLocalArgs *)_args;
	SP(ShardingNotification) request = args->notif;
	delete args;
	if(! request->resolveNotification(request)){
		Logger::debug("SHM | Notification resolve returned false : %s", request->getDescription().c_str());
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
			SP(ShardingNotification) notif = ShardManager::getShardManager()->bouncedNotifications.at(i);
			ShardingNotification::send(notif);
		}
		ShardManager::getShardManager()->bouncedNotifications.clear();


		// 2. if we are joined, start load balancing.
		if(ShardManager::getShardManager()->isJoined() && ! ShardManager::getShardManager()->isLoadBalancing()){
			ShardManager::getShardManager()->setLoadBalancing();
			LoadBalancer::runLoadBalancer();
		}

		// TODO remove
//		cout << "===========================================================================================" << endl;
//		cout << "===========================================================================================" << endl;
//		cout << "===========================================================================================" << endl;
//		cout << "===========================================================================================" << endl;
//		cout << "===========================================================================================" << endl;
	    ShardManager::getShardManager()->print();
	}
	return NULL;
}

void ShardManager::saveBouncedNotification(SP(ShardingNotification) notif){
	notif->resetBounced();
	notif->swapSrcDest();
	bouncedNotifications.push_back(notif);
}

void ShardManager::bounceNotification(SP(ShardingNotification) notif){
	notif->setBounced();
	notif->swapSrcDest();
	ShardingNotification::send(notif);
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


