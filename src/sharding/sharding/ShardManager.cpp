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
#include "sharding/configuration/ShardingConstants.h"
#include "core/util/Assert.h"
#include <pthread.h>
#include <signal.h>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardManager * ShardManager::singleInstance = NULL;
boost::shared_mutex ShardManager::singleInstanceLock;

ShardManager * ShardManager::createShardManager(ConfigManager * configManager, ResourceMetadataManager * metadataManager){
	boost::unique_lock<boost::shared_mutex> xLock(singleInstanceLock);
	if(singleInstance != NULL){
		return singleInstance;
	}
	// only shard manager must be singleton. ConfigManager must be accessed from shard manager

	singleInstance = new ShardManager(configManager, metadataManager);
	return singleInstance;
}

void ShardManager::deleteShardManager(){
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(singleInstance == NULL){
		return ;
	}
	sLock.unlock();
	singleInstance->setCancelled();
	boost::unique_lock<boost::shared_mutex> xLock(singleInstanceLock);
	delete singleInstance;
	singleInstance = NULL;
	return;
}

ShardManager * ShardManager::getShardManager(){
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	return singleInstance;
}

NodeId ShardManager::getCurrentNodeId(){
//	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	// theoretically it must also be s locked but this variables does not every change after initialization of system
	return ShardManager::getShardManager()->currentNodeId;
}


Cluster_Writeview * ShardManager::getWriteview_write(boost::unique_lock<boost::shared_mutex> & xLock){
	return ShardManager::getShardManager()->getMetadataManager()->getClusterWriteview_write(xLock);
}

const Cluster_Writeview * ShardManager::getWriteview_read(boost::shared_lock<boost::shared_mutex> & sLock){
	return ShardManager::getShardManager()->getMetadataManager()->getClusterWriteview_read(sLock);
}

SP(ClusterNodes_Writeview) ShardManager::getNodesWriteview_write(){
	return ShardManager::getShardManager()->getMetadataManager()->getClusterNodesWriteview_write();
}

SP(const ClusterNodes_Writeview) ShardManager::getNodesWriteview_read(){
	return ShardManager::getShardManager()->getMetadataManager()->getClusterNodesWriteview_read();
}

void ShardManager::getReadview(boost::shared_ptr<const ClusterResourceMetadata_Readview> & readview) {
	ShardManager::getShardManager()->getMetadataManager()->getClusterReadView(readview);
}

StateMachine * ShardManager::getStateMachine(){
	return  ShardManager::getShardManager()->_getStateMachine();
}

boost::shared_mutex & ShardManager::getShardManagerGuard(){
	return singleInstanceLock;
}

ShardManager::ShardManager(ConfigManager * configManager,ResourceMetadataManager * metadataManager){

	this->configManager = configManager;
	this->metadataManager = metadataManager;
	this->_lockManager = new LockManager();
	this->stateMachine = new StateMachine();
	this->joinedFlag = false;
	this->cancelledFlag = false;
	this->loadBalancingThread = new pthread_t;
	updateCurrentNodeId();

	for(unsigned i = 0 ; i < MAX_NUM_TRANS_GROUPS; ++i){
		mmSessionListenersGroup.push_back(std::make_pair(map<unsigned , ConsumerInterface *>(), new boost::mutex()));
		mmSessionListenersGroup_TransSharedPointers.push_back(map<unsigned , SP(Transaction)>());
	}

}

void ShardManager::attachToTransportManager(TransportManager * tm){
	this->transportManager = tm;
	this->transportManager->registerCallbackForShardingMessageHandler(this);
}

void ShardManager::initFirstNode(const bool shouldLock){
	// assign primary shards to this node :
	// X lock on writeviewMutex and nodesMutex must be obtained
	if(shouldLock){
		boost::unique_lock<boost::shared_mutex> xLock;
		Cluster_Writeview * writeview = ShardManager::getWriteview_write(xLock);
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		MetadataInitializer nodeInitializer(configManager, this->metadataManager);
		nodeInitializer.initializeCluster();
		this->getMetadataManager()->commitClusterMetadata(false);
		this->setJoined();
	}else{
		MetadataInitializer nodeInitializer(configManager, this->metadataManager);
		nodeInitializer.initializeCluster();
		this->getMetadataManager()->commitClusterMetadata(false);
		this->setJoined();
	}
}

void ShardManager::updateCurrentNodeId(Cluster_Writeview * writeviewLocked){
	if(writeviewLocked == NULL){
		boost::unique_lock<boost::shared_mutex> xLock;
		Cluster_Writeview * writeview = getWriteview_write(xLock);
		if(writeview == NULL){
			this->currentNodeId = 0;
			return;
		}
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		// xlock on writeview and nodesWriteview both
		boost::unique_lock<boost::shared_mutex> shardMngrContentXLock(shardManagerMembersMutex);
		this->currentNodeId = writeview->currentNodeId;
	}else{
		boost::unique_lock<boost::shared_mutex> shardMngrContentXLock(shardManagerMembersMutex);
		this->currentNodeId = writeviewLocked->currentNodeId;
	}
}

ShardManager::~ShardManager(){
	// waiting for all transactions to leave before dying ...
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
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
    joinedFlag = true;
}

bool ShardManager::isJoined() {
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
    return joinedFlag;
}

void ShardManager::setCancelled(){
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
	this->cancelledFlag = true;
}
bool ShardManager::isCancelled() {
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	return this->cancelledFlag;
}

void ShardManager::setLoadBalancing(){
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
	this->loadBalancingFlag = true;
}
void ShardManager::resetLoadBalancing(){
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
	this->loadBalancingFlag = false;
}
bool ShardManager::isLoadBalancing() {
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	return this->loadBalancingFlag;
}

pthread_t * ShardManager::getLoadbalancingThread() {
	return this->loadBalancingThread;
}

void ShardManager::print(){
	// lock writeview
	boost::unique_lock<boost::shared_mutex> xLock;
	metadataManager->getClusterWriteview_write(xLock);
	SP(const ClusterNodes_Writeview) nodesWriteview = metadataManager->getClusterNodesWriteview_read();
	metadataManager->print();
	xLock.unlock();
	nodesWriteview.reset();

	_lockManager->print();

	stateMachine->lockStateMachine();
	stateMachine->print();
	stateMachine->unlockStateMachine();

	// bounced notifications
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	printBouncedNotifications();

}

void ShardManager::start(){
	Logger::info("Starting data processor ...");
	// lock the ShardManager root pointer to avoid its deletion before we are done.
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	// insert this node and all older nodes in the lock manager in the beginning.
	ShardManager::getShardManager()->_lockManager->initialize();

	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	unsigned numberOfNodes = nodesWriteview->getNumberOfAliveNodes();
	nodesWriteview.reset(); // unlock nodes writeview

	if(numberOfNodes == 1){ // we are the first node:
		// assign primary shards to this node :
		initFirstNode();
		Logger::info("Cluster is ready to accept new nodes. Current node ID : %d",
				ShardManager::getCurrentNodeId());
	}else{
		// commit the readview to be accessed by readers until we join
		this->getMetadataManager()->commitClusterMetadata();
		Logger::info("Joining the existing cluster ...");
		ASSERT(! this->isJoined());
		Logger::sharding(Logger::Info, "Printing node information before join ...");
		print();

		// we must join an existing cluster :
		NodeJoiner::join();
	}
    if (pthread_create(loadBalancingThread, NULL, ShardManager::periodicWork , NULL) != 0){
        //        Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for load balancing.");
        __FUNC_LINE__
        Logger::sharding(Logger::Error, "Cannot create thread for load balancing.");
        return;
    }
}

void ShardManager::_shutdown(){
	Logger::console("Shutting down the instance ...");
	raise(SIGTERM);
//    kill(getpid(),SIGTERM);
//	exit(0);
	//TODO
}


bool ShardManager::resolveMessage(Message * msg, NodeId senderNode){
	if(msg == NULL){
		return false;
	}

	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);

	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	// if a node fails, we don't accept any more messages from it.
	if(nodesWriteview->getNodes_read().find(senderNode) != nodesWriteview->getNodes_read().end() &&
			nodesWriteview->getNodes_read().at(senderNode).first == ShardingNodeStateFailed){
		Logger::sharding(Logger::Error, "SHM| !!! Warning: Message with type %s was", msg->getDescription().c_str());
		Logger::sharding(Logger::Error, "     ignored because source node had failed before.!!!");
		return true;
	}
	nodesWriteview.reset();

	// deserialize sharding header information
	NodeOperationId srcAddress, destAddress;
	bool bounced;
	ShardingNotification::deserializeHeader(msg, senderNode, srcAddress, destAddress, bounced);
	// deserialize to get notifications
	bool mustBounce = false;
	SP(ShardingNotification) notif;
	switch (msg->getType()) {
		case ShardingShardCommandMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CommandNotification>(msg);
			mustBounce = true;
			break;
		case StatusMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CommandStatusNotification>(msg);
			break;
		case ShardingNewNodeReadMetadataRequestMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MetadataReport::REQUEST>(msg);
			mustBounce = true;
			break;
		case ShardingNewNodeReadMetadataReplyMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MetadataReport>(msg);
			break;
		case ShardingLockMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LockingNotification>(msg);
			mustBounce = true;
			break;
		case ShardingLockACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(msg);
			break;
		case ShardingCommitMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CommitNotification>(msg);
			mustBounce = true;
			break;
		case ShardingCommitACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CommitNotification::ACK>(msg);
			break;
		case ShardingLoadBalancingReportMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LoadBalancingReport>(msg);
			break;
		case ShardingLoadBalancingReportRequestMessageType:
			notif = ShardingNotification::deserializeAndConstruct<LoadBalancingReport::REQUEST>(msg);
			mustBounce = true;
			break;
		case ShardingMoveToMeMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification>(msg);
			mustBounce = true;
			break;
		case ShardingMoveToMeACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::ACK>(msg);
			break;
		case ShardingMoveToMeCleanupMessageType:
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::CleanUp>(msg);
			break;
		case ShardingCopyToMeMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CopyToMeNotification>(msg);
			mustBounce = true;
			break;
		case ShardingCopyToMeACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<CopyToMeNotification::ACK>(msg);
			break;
		case ShardingShutdownMessageType:
			notif = ShardingNotification::deserializeAndConstruct<ShutdownNotification>(msg);
			break;
		case ShardingAclAttrReadMessageType:
			notif = ShardingNotification::deserializeAndConstruct<AclAttributeReadNotification>(msg);
			break;
		case ShardingAclAttrReadACKMessageType:
			notif = ShardingNotification::deserializeAndConstruct<AclAttributeReadNotification::ACK>(msg);
			break;
		default:
			ASSERT(false);
			break;
	}

	if(bounced){
        saveBouncedNotification(notif);
        Logger::sharding(Logger::Detail, "SHM| Bounced notification received and saved.");
        return true;
	}

	if(mustBounce && ! isJoined()){
		bounceNotification(notif);
		Logger::sharding(Logger::Detail, "SHM| Bouncing incoming notification %s.", notif->getDescription().c_str());
		return true;
	}

	if(msg->getType() == ShardingShutdownMessageType){
		sLock.unlock();
	}
	if(! notif->resolveNotification(notif)){
		Logger::sharding(Logger::Detail, "SHM| Notification resolve returned false : %s", notif->getDescription().c_str());
	}

	return true;
}

void * ShardManager::resolveReadviewRelease(void * vidPtr){
	unsigned metadataVersion = *(unsigned *)vidPtr;
	delete (unsigned *)vidPtr;
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);

	Logger::sharding(Logger::Detail, "SHM| Metadata release VID=%d", metadataVersion);
	ShardManager::getShardManager()->_getLockManager()->resolve(metadataVersion);
	Logger::sharding(Logger::Detail, "SHM| Metadata release VID=%d processed.", metadataVersion);

//    cout << "Shard Manager status after receiving RV release, vid = " << metadataVersion << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;

	return NULL;
}

void ShardManager::resolveMMNotification(const ShardMigrationStatus & migrationStatus){

	Logger::sharding(Logger::Detail, "SHM| MM (%d => %d) was %s", migrationStatus.sourceNodeId, migrationStatus.destinationNodeId,
			(migrationStatus.status == MM_STATUS_SUCCESS)? "Done."  : "Failed.");
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);

	unsigned groupId = migrationStatus.dstOperationId % MAX_NUM_TRANS_GROUPS;

	boost::unique_lock<boost::mutex> xLock(*(mmSessionListenersGroup.at(groupId).second));
	map<unsigned , ConsumerInterface *> & mmSessionListeners = mmSessionListenersGroup.at(groupId).first;
	map<unsigned, SP(Transaction)> & mmSessionListeners_TransHolder = mmSessionListenersGroup_TransSharedPointers.at(groupId);

	if(mmSessionListeners.find(migrationStatus.dstOperationId) == mmSessionListeners.end()){
		return;
	}

	// consumer found, first call threadBegin() of its transaction if
	// it's possible
	SP(Transaction) transaction = mmSessionListeners_TransHolder.find(migrationStatus.dstOperationId)->second;
	transaction->threadBegin(transaction);
	mmSessionListeners.find(migrationStatus.dstOperationId)->second->consume(migrationStatus);
	transaction->threadEnd();
	mmSessionListeners.erase(mmSessionListeners.find(migrationStatus.dstOperationId));
	mmSessionListeners_TransHolder.erase(mmSessionListeners_TransHolder.find(migrationStatus.dstOperationId));

	Logger::sharding(Logger::Detail, "SHM| MM (%d => %d) was %s Processed.", migrationStatus.sourceNodeId,
			migrationStatus.destinationNodeId,
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
	unsigned groupId = operationId % MAX_NUM_TRANS_GROUPS;

	boost::unique_lock<boost::mutex> xLock(*(mmSessionListenersGroup.at(groupId).second));
	map<unsigned , ConsumerInterface *> & mmSessionListeners = mmSessionListenersGroup.at(groupId).first;
	map<unsigned, SP(Transaction)> & mmSessionListeners_TransHolder = mmSessionListenersGroup_TransSharedPointers.at(groupId);
	if(mmSessionListeners.find(operationId) != mmSessionListeners.end()){
		ASSERT(false);
		return;
	}
	mmSessionListeners[operationId] = listener;
	ASSERT(listener->getTransaction());
	mmSessionListeners_TransHolder[operationId] = listener->getTransaction();
}

void ShardManager::resolveSMNodeArrival(const Node & newNode){
	Logger::sharding(Logger::Detail, "SHM| SM Node %d arrival.", newNode.getId());
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = ShardManager::getWriteview_write(xLock);
	SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
    nodesWriteview->addNode(newNode);
	Logger::sharding(Logger::Detail, "SHM| SM Node %d arrival. Processed.", newNode.getId());
//    cout << "Shard Manager status after arrival of node " << newNode.getId() << ":" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::resolveSMNodeFailure(const NodeId failedNodeId){
	Logger::sharding(Logger::Detail, "SHM| SM Node %d failure.", failedNodeId);
	boost::unique_lock<boost::shared_mutex> xLock(singleInstanceLock);
	SP(Notification) nodeFailureNotif(new NodeFailureNotification(failedNodeId));

	// 1. metadata manager
	this->metadataManager->resolve(boost::dynamic_pointer_cast<NodeFailureNotification>(nodeFailureNotif));
	// 2. lock manager
	this->_lockManager->resolveNodeFailure(failedNodeId);

	// 3. state machine
	this->stateMachine->handle(nodeFailureNotif);
	Logger::sharding(Logger::Detail, "SHM| SM Node %d failure. Processed.", failedNodeId);
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
        Logger::sharding(Logger::Error, "SHM| Cannot create thread for handling local message");
        return false;
    }
    pthread_detach(localThread);
    return true;
}

void * ShardManager::_resolveLocal(void * _args){
	ResolveLocalArgs * args = (ResolveLocalArgs *)_args;
	SP(ShardingNotification) request = args->notif;
	delete args;
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(! request->resolveNotification(request)){
		Logger::sharding(Logger::Detail, "SHM| Notification resolve returned false : %s", request->getDescription().c_str());
	}
	return NULL;
}


void * ShardManager::periodicWork(void *args) {

	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	while(! ShardManager::getShardManager()->isCancelled()){

		/*
		 * 1. Resend bounced notifications.
		 * 2. is we are joined, start load balancing.
		 */
		//
		sleep(2);

		// 1. Resend bounced notifications.
		ShardManager::getShardManager()->resendBouncedNotifications();

		/*
		 * TODO (not necessary but good and safer to have) : we must implement a method with
		 *       periodically goes through all data structures and
		 *       makes sure they are not stuck anywhere ....
		 */
		// 2. if we are joined, start load balancing.
		if(ShardManager::getShardManager()->isJoined() && ! ShardManager::getShardManager()->isLoadBalancing()){
			ShardManager::getShardManager()->setLoadBalancing();
			LoadBalancer::runLoadBalancer();
		}

	    ShardManager::getShardManager()->print();
	}
	return NULL;
}

void ShardManager::saveBouncedNotification(SP(ShardingNotification) notif){
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
	notif->resetBounced();
	notif->swapSrcDest();
	bouncedNotifications.push_back(notif);
}
void ShardManager::resendBouncedNotifications(){
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
	for(unsigned i = 0 ; i < bouncedNotifications.size() ; ++i){
		SP(ShardingNotification) notif = bouncedNotifications.at(i);
		ShardingNotification::send(notif);
	}
	bouncedNotifications.clear();
}

void ShardManager::bounceNotification(SP(ShardingNotification) notif){
	notif->setBounced();
	notif->swapSrcDest();
	ShardingNotification::send(notif);
}

void ShardManager::printBouncedNotifications(){
	if(bouncedNotifications.empty()){
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

void ShardManager::getNodeInfoJson(Json::Value & nodeInfo){

	if(nodeInfo == nullJsonValue){
		nodeInfo = Json::Value(Json::arrayValue);
	}
	if(nodeInfo.type() != Json::arrayValue){
		ASSERT(false);
		return;
	}
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	vector<const Node *> nodes;
	nodesWriteview->getAllNodes(nodes);
	for(unsigned i = 0; i < nodes.size(); ++i){
		nodeInfo[i] = Json::Value(Json::objectValue);
		nodeInfo[i][c_node_name] = nodes.at(i)->getName();
		nodeInfo[i][c_node_listening_host_name] = nodes.at(i)->getIpAddress();
	}

}

}
}


