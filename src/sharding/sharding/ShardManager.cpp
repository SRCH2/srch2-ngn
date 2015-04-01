// Author : Jamshid
#include "ShardManager.h"


#include "./state_machine/StateMachine.h"
#include "./state_machine/ConsumerProducer.h"
#include "./metadata_manager/ResourceMetadataManager.h"
#include "./lock_manager/LockManager.h"
#include "./lock_manager/LockManager.h"
#include "./metadata_manager/Cluster_Writeview.h"
#include "./metadata_manager/Cluster.h"
#include "./metadata_manager/MetadataInitializer.h"
#include "./transactions/cluster_transactions/ShutdownCommand.h"
#include "./notifications/Notification.h"
#include "./notifications/CommitNotification.h"
#include "./notifications/LoadBalancingReport.h"
#include "./notifications/LockingNotification.h"
#include "./notifications/MetadataReport.h"
#include "./notifications/MoveToMeNotification.h"
#include "./notifications/CopyToMeNotification.h"
#include "./notifications/CommandNotification.h"
#include "./notifications/AclAttributeReadNotification.h"
#include "./notifications/AclAttributeReplaceNotification.h"
#include "./notifications/Write2PCNotification.h"
#include "./notifications/SearchCommandNotification.h"
#include "./notifications/SearchCommandResultsNotification.h"
#include "./transactions/cluster_transactions/LoadBalancer.h"
#include "./transactions/ShardMoveOperation.h"
#include "./transactions/cluster_transactions/NodeJoiner.h"
#include "./migration/MigrationManager.h"
#include "./configuration/ShardingConstants.h"
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
	// only ShardManager must be singleton. ConfigManager must be accessed from ShardManager
	singleInstance = new ShardManager(configManager, metadataManager);
	xLock.unlock();
	// ShardManager object is constructed, set the value of currentNodeId
	singleInstance->updateCurrentNodeId();
	return singleInstance;
}

void ShardManager::deleteShardManager(){
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(singleInstance == NULL){
		return ;
	}
	sLock.unlock();
	// set the cancel flag to stop new tasks coming into the ShardManager
	singleInstance->setCancelled();

	// S lock on singleInstanceLock is acquired in all entry points of ShardManager
	// this way the next line will block until all of them finish.
	singleInstanceLock.lock();
	delete singleInstance;
	singleInstance = NULL;
	return;
}

ShardManager * ShardManager::getShardManager(){
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(singleInstance == NULL){
		return NULL;
	}
	return singleInstance;
}

NodeId ShardManager::getCurrentNodeId(){
	// in fact it must also get S lock on shardManagerMembersMutex but this variables
	// does not every change after initialization of system
	return ShardManager::getShardManager()->currentNodeId;
}


Cluster_Writeview * ShardManager::getWriteview_write(boost::unique_lock<boost::shared_mutex> & xLock){
	return ShardManager::getShardManager()->getMetadataManager()->getClusterWriteview_write(xLock);
}
Cluster_Writeview * ShardManager::getWriteview_nolock(){
	return ShardManager::getShardManager()->getMetadataManager()->getClusterWriteview_nolock();
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

	this->currentNodeId = 0;
	this->dpInternal = NULL;
	this->transportManager = NULL;
	this->migrationManager = NULL;
	this->configManager = configManager;
	this->metadataManager = metadataManager;
	this->_lockManager = new LockManager();
	this->stateMachine = new StateMachine();
	this->joinedFlag = false;
	this->cancelledFlag = false;
	this->loadBalancingFlag = false;
	this->loadBalancingCheckInterval = 500000;
	this->loadBalancingThread = new pthread_t;

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

	// lock the writeview only if the caller passes true
	// which means caller has not locked the writeview before
	// calling this method.
	if(shouldLock){
		boost::unique_lock<boost::shared_mutex> xLock;
		Cluster_Writeview * writeview = ShardManager::getWriteview_write(xLock);
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		MetadataInitializer nodeInitializer(configManager, this->metadataManager);
		// prepares the metadata so that this node becomes the first node of
		// cluster. Primary shards of all partitions are assigned to this node.
		nodeInitializer.initializeCluster(false);
		// commit the metadata to reflect the changes on the readview copy
		// which is accessed by RESTful API requests
		this->getMetadataManager()->commitClusterMetadata(false);
		// Set joined to the cluster.
		// After the next line the normal execution of ShardManager is
		// started (so for example other nodes can start coming in...)
		this->setJoined();
	}else{
		MetadataInitializer nodeInitializer(configManager, this->metadataManager);
		nodeInitializer.initializeCluster(false); // since shouldLock is false
		this->getMetadataManager()->commitClusterMetadata(false);
		this->setJoined();
	}
}

void ShardManager::updateCurrentNodeId(Cluster_Writeview * writeviewLocked){
	if(writeviewLocked == NULL){
		// xlock on metadata writeview
		boost::unique_lock<boost::shared_mutex> xLock;
		Cluster_Writeview * writeview = getWriteview_write(xLock);
		if(writeview == NULL){
			this->currentNodeId = 0;
			return;
		}
		// xlock on the nodes info part of metadata writeview
		SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
		// xlock on the mutex which protects the content of ShardManager
		boost::unique_lock<boost::shared_mutex> shardMngrContentXLock(shardManagerMembersMutex);
		this->currentNodeId = writeview->currentNodeId;
	}else{
		boost::unique_lock<boost::shared_mutex> shardMngrContentXLock(shardManagerMembersMutex);
		this->currentNodeId = writeviewLocked->currentNodeId;
	}
}

ShardManager::~ShardManager(){
	// delete the content of ShardManager
	delete this->loadBalancingThread;
	delete this->_lockManager;
	delete stateMachine;
	for(unsigned i = 0 ; i < MAX_NUM_TRANS_GROUPS; ++i){
		delete mmSessionListenersGroup.at(i).second;
	}
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
	shardManagerMembersMutex.lock();
	this->cancelledFlag = true;
	shardManagerMembersMutex.unlock();
	// clearning the StateMachine destroys all ongoing
	// operations, and therefore, in case an operation also
	// keeps a reference to a transaction, that reference also gets
	// deleted and transactions deletes.
	this->getStateMachine()->clear();
	// Same as StateMachine, we want all waiting transactions to be freed.
	this->clearMMRegistrations();
	// cancel all other threads, shutdown in progress
	this->cancelAllThreads(false);
}
bool ShardManager::isCancelled() {
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	return this->cancelledFlag;
}

void ShardManager::setLoadBalancing(){
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
	this->loadBalancingFlag = true;
}
void ShardManager::resetLoadBalancing(bool loadBalancingHappened){
	boost::unique_lock<boost::shared_mutex> xLock(shardManagerMembersMutex);
	this->loadBalancingFlag = false;
	// if load balancing could successfully find a task, we set the
	// amount of time to wait before next LB check
	// to its base value, otherwise, we increase this interval to
	// avoid too many LB checks
	if(loadBalancingHappened){
		// reset interval to base value
		this->loadBalancingCheckInterval = 500000;
	}else{
		// increase interval
		this->loadBalancingCheckInterval *= 2;
		if(this->loadBalancingCheckInterval > 5000000){
			this->loadBalancingCheckInterval = 5000000;
		}
	}
}
bool ShardManager::isLoadBalancing() {
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	return this->loadBalancingFlag;
}

uint32_t ShardManager::getLoadBalancingCheckInterval(){
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	return this->loadBalancingCheckInterval ;
}

pthread_t * ShardManager::getLoadbalancingThread() {
	return this->loadBalancingThread;
}

void ShardManager::print(JsonResponseHandler * response){
	if(response != NULL){
		// lock writeview
		// TODO : reviewer please make sure we need XLock and we can't
		// just grab S lock for print.
		boost::unique_lock<boost::shared_mutex> xLock;
		metadataManager->getClusterWriteview_write(xLock);
		SP(const ClusterNodes_Writeview) nodesWriteview = metadataManager->getClusterNodesWriteview_read();
		metadataManager->print(response);
		xLock.unlock();
		nodesWriteview.reset();
		_lockManager->print(response);
		if(stateMachine->lockStateMachine()){
			stateMachine->print(response);
			stateMachine->unlockStateMachine();
		}
		// bounced notifications
		boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
		printBouncedNotifications(response);

		return;
	}
	// lock writeview
	boost::unique_lock<boost::shared_mutex> xLock;
	metadataManager->getClusterWriteview_write(xLock);
	SP(const ClusterNodes_Writeview) nodesWriteview = metadataManager->getClusterNodesWriteview_read();
	metadataManager->print();
	xLock.unlock();
	nodesWriteview.reset();

	_lockManager->print();

	if(stateMachine->lockStateMachine()){
		stateMachine->print();
		stateMachine->unlockStateMachine();
	}else{
		//TODO : print : stateMachine could not be locked.
	}

	// bounced notifications
	boost::shared_lock<boost::shared_mutex> sLock(shardManagerMembersMutex);
	printBouncedNotifications();

}

void ShardManager::start(){
	Logger::info("Starting data processor ...");
	// S lock on the ShardManager single object pointer : to avoid deletion before
	// this logic finishes
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	// if ShardManager is cancelled before reaching here, stop.
	if(isCancelled()){
		return;
	}
	// do the main lock manager initialization
	// the node Id of this node and older nodes are added to the writeview at this point,
	// lock manager will also add these information. the main reason is
	// nodes must join the cluster in ascending order of their IDs so lockManager must be
	// aware of all arrived nodes.
	ShardManager::getShardManager()->_lockManager->initialize();

	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	unsigned numberOfNodes = nodesWriteview->getNumberOfAliveNodes();
	// unlock nodes writeview : when shared pointer destroys, lock object also gets destroyed.
	nodesWriteview.reset();

	if(numberOfNodes == 1){ // we are the first node:
		// start a new cluster with this node.
		initFirstNode();
		Logger::info("Cluster is ready to accept new nodes. Current node ID : %d",
				ShardManager::getCurrentNodeId());
	}else{
		// commit the readview to be accessed by readers until we join
		// NOTE: since local data shards which are stored on disk are loaded into memory
		//       we can let RESTful API requests come in earlier.
		this->getMetadataManager()->commitClusterMetadata();
		Logger::info("Joining the existing cluster ...");
		// we must not be joined already
		ASSERT(! this->isJoined());
		Logger::sharding(Logger::Info, "Printing node information before join ...");
		print();

		// we must join an existing cluster :
		// start the NodeJoiner transaction which communicates with other nodes
		// to add itself to the existing cluster.
		NodeJoiner::join();
	}

	// start the thread which does periodic logic. This thread will periodically
	// perform some tasks such as checking for possible load balancing task
    if (pthread_create(loadBalancingThread, NULL, ShardManager::periodicWork , NULL) != 0){
        //        Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for load balancing.");
        __FUNC_LINE__
        Logger::sharding(Logger::Error, "Cannot create thread for load balancing.");
        return;
    }
}

bool ShardManager::resolveMessage(Message * msg, NodeId senderNode){
	// TODO : return value of this method is not used currently.
	//        if we use the return value, then, return statements of this method
	//        must be double checked.
	if(msg == NULL){
		return false;
	}

	// S lock on the singleton object pointer to avoid deletion before this method exits
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	// if shard manager is cancelled before reaching here, exit: shutdown is in progress.
	if(isCancelled()){
		return true;
	}

	// get read access to node information
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	// if a node fails, we don't accept any more messages from it.
	if(nodesWriteview->getNodes_read().find(senderNode) != nodesWriteview->getNodes_read().end() &&
			nodesWriteview->getNodes_read().at(senderNode).first == ShardingNodeStateFailed){
		Logger::sharding(Logger::Error, "SHM| !!! Warning: Message with type %s was", msg->getDescription().c_str());
		Logger::sharding(Logger::Error, "     ignored because source node had failed before.!!!");
		return true;
	}
	// release the S lock on node info part of metadata
	nodesWriteview.reset();

	NodeOperationId srcAddress, destAddress;
	// flag indicating whether this message is a returning bounced message
	bool bounced;
	// deserialize sharding header information
	ShardingNotification::deserializeHeader(msg, senderNode, srcAddress, destAddress, bounced);

	// flag indicating whether this notification must be bounced so that
	// we serve it later when it's sent to here again...
	bool mustBounce = false;
	SP(ShardingNotification) notif;
	switch (msg->getType()) {
		case ShardingShardCommandMessageType:
			/*
			 * A shard command message is a request to perform one of the following tasks on
			 * a data shard which resides on the current node :
	         * SaveData,
	         * SaveMetadata,
	         * Export,
             * Commit,
             * Merge,
             * MergeSetOn,
             * MergeSetOff,
             * ResetLogger
			 */
			notif = ShardingNotification::deserializeAndConstruct<CommandNotification>(msg);
			// not able to serve this before joining the cluster
			mustBounce = true;
			break;
		case StatusMessageType:
			/*
			 * A CommandStatusNotification contains the outcome of task performed upon receiving
			 * a CommandNotification by the sender node of this notification.
			 */
			notif = ShardingNotification::deserializeAndConstruct<CommandStatusNotification>(msg);
			break;
		case ShardingNewNodeReadMetadataRequestMessageType:
			/*
			 * A MetadataReport::REQUEST is sent to an existing node of cluster by a new node
			 * to read its current cluster metadata
			 */
			notif = ShardingNotification::deserializeAndConstruct<MetadataReport::REQUEST>(msg);
			// not able to serve this before joining the cluster
			mustBounce = true;
			break;
		case ShardingNewNodeReadMetadataReplyMessageType:
			/*
			 * A MetadataReport contains the cluster metadata information of the sender node
			 * and is used by the current node to have a consistent first version of metadata and be able
			 * to join the cluster.
			 */
			notif = ShardingNotification::deserializeAndConstruct<MetadataReport>(msg);
			break;
		case ShardingLockMessageType:
			/*
			 * LockingNotification is used to ask this node to perform a locking request on it's lockManager (grab or release)
			 */
			notif = ShardingNotification::deserializeAndConstruct<LockingNotification>(msg);
			// not able to serve this before joining the cluster
			mustBounce = true;
			break;
		case ShardingLockACKMessageType:
			/*
			 * A LockingNotification::ACK contains the result of a lock request to the sender node.
			 * for example if the request was a lock-acquisition, ACK contains GRANT or REJECT
			 */
			notif = ShardingNotification::deserializeAndConstruct<LockingNotification::ACK>(msg);
			break;
		case ShardingCommitMessageType:
			/*
			 * A CommitNotification contains the changes that this node must make on its
			 * cluster metadata and then commit.
			 */
			notif = ShardingNotification::deserializeAndConstruct<CommitNotification>(msg);
			// not able to serve this before joining the cluster
			mustBounce = true;
			break;
		case ShardingCommitACKMessageType:
			/*
			 * A CommitNotification::ACK contains the result of a metadata commit request.
			 * NOTE: this commit is a distributed 'commit' which also includes a writeview=>readview commit
			 */
			notif = ShardingNotification::deserializeAndConstruct<CommitNotification::ACK>(msg);
			break;
		case ShardingLoadBalancingReportMessageType:
			/*
			 * A LoadBalancingReport contains load information of the sender node. It will be passed to
			 * the state-machine to eventually reach the LoadBalancing transaction
			 */
			notif = ShardingNotification::deserializeAndConstruct<LoadBalancingReport>(msg);
			break;
		case ShardingLoadBalancingReportRequestMessageType:
			/*
			 * Notification LoadBalancingReport::REQUEST is used to collect load information
			 * in the beginning of a load balancing process.
			 */
			notif = ShardingNotification::deserializeAndConstruct<LoadBalancingReport::REQUEST>(msg);
			// not able to serve this before joining the cluster
			mustBounce = true;
			break;
		case ShardingMoveToMeMessageType:
			/*
			 * A MoveToMeNotification is used in a ShardMove process. The destination uses this
			 * notification to as the source to start the move process.
			 */
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification>(msg);
			// not able to serve this before joining the cluster
			mustBounce = true;
			break;
		case ShardingMoveToMeACKMessageType:
			/*
			 * The ack to MoveToMeNotification. It indicates that the shard move process has started.
			 */
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::ACK>(msg);
			break;
		case ShardingMoveToMeCleanupMessageType:
			/*
			 * TODO
			 * Currently not used. But it will be used if we want to be able to recover a data shard that's moved to
			 * a destination and then the destination has died.
			 */
			notif = ShardingNotification::deserializeAndConstruct<MoveToMeNotification::CleanUp>(msg);
			break;
		case ShardingCopyToMeMessageType:
			/*
			 * A CopyToMeNotification is used in a shard copy process by the destination to ask
			 * the source node to start.
			 */
			notif = ShardingNotification::deserializeAndConstruct<CopyToMeNotification>(msg);
			// not able to serve this before joining the cluster
			mustBounce = true;
			break;
		case ShardingCopyToMeACKMessageType:
			/*
			 * CopyToMeNotification::ACK is the ACK to CopyToMeNotification sent back to the destination by the copy source.
			 */
			notif = ShardingNotification::deserializeAndConstruct<CopyToMeNotification::ACK>(msg);
			break;
		case ShardingShutdownMessageType:
			/*
			 * When a cluster-shutdown request comes to one node, a ShutdownNotification
			 * is used to ask everybody to shutdown.
			 */
			notif = ShardingNotification::deserializeAndConstruct<ShutdownNotification>(msg);
			break;
		case ShardingAclAttrReadMessageType:
			/*
			 * A AclAttributeReadNotification is used to read some attribute-acl information
			 * maintained in this node.
			 */
			notif = ShardingNotification::deserializeAndConstruct<AclAttributeReadNotification>(msg);
			break;
		case ShardingAclAttrReadACKMessageType:
			/*
			 * ACK to AclAttributeReadNotification
			 */
			notif = ShardingNotification::deserializeAndConstruct<AclAttributeReadNotification::ACK>(msg);
			break;
		case ShardingAclAttrReplaceMessageType:
			/*
			 * A AclAttributeReplaceNotification is used for the attribute-ACL replace request.
			 */
			notif = ShardingNotification::deserializeAndConstruct<AclAttributeReplaceNotification>(msg);
			break;
		case ShardingAclAttrReplaceACKMessageType:
			/*
			 * ACK to AclAttributeReplaceNotification
			 */
			notif = ShardingNotification::deserializeAndConstruct<AclAttributeReplaceNotification::ACK>(msg);
			break;
		case ShardingWriteCommand2PCMessageType:
			/*
			 * The Write2PCNotification is used for both phases of 2PC insert process.
			 * It's used by the WriteCommandHttp transaction and WriteCommand module which
			 * performs record/acl write operations.
			 */
			notif = ShardingNotification::deserializeAndConstruct<Write2PCNotification>(msg);
			break;
		case ShardingWriteCommand2PCACKMessageType:
			/*
			 * Result of performing whatever Write2PCNotification has requested.
			 */
			notif = ShardingNotification::deserializeAndConstruct<Write2PCNotification::ACK>(msg);
			break;
		case ShardingSearchCommandMessageType:
			/*
			 * A SearchCommand is a notification used in performing a keyword-search operation.
			 */
			notif = ShardingNotification::deserializeAndConstruct<SearchCommand>(msg);
			break;
		case ShardingSearchResultsMessageType:
			/*
			 * A SearchCommandResults is in fact the ACK to SearchCommand. It contains the search results.
			 */
			notif = ShardingNotification::deserializeAndConstruct<SearchCommandResults>(msg);
			break;
		default:
			ASSERT(false);
			break;
	}

	// if it's a returned bounced notification, save it to send it again later.
	if(bounced){
        saveBouncedNotification(notif);
        Logger::sharding(Logger::Detail, "SHM| Bounced notification received and saved.");
        return true;
	}

	// if we haven't joined the cluster and this notification cannot be served immediately, we
	// bounce the notification to its sender.
	if(mustBounce && ! isJoined()){
		bounceNotification(notif);
		Logger::sharding(Logger::Detail, "SHM| Bouncing incoming notification %s.", notif->getDescription().c_str());
		return true;
	}

	// Serve this notification based on its own implementation of resolveNotification pure virtual method from
	// ShardingNotification
	if(! notif->resolveNotification(notif)){
		Logger::sharding(Logger::Detail, "SHM| Notification resolve returned false : %s", notif->getDescription().c_str());
	}

	return true;
}

void * ShardManager::resolveReadviewRelease(void * vidPtr){
	// the version of metadata that has just destroyed.
	unsigned metadataVersion = *(unsigned *)vidPtr;
	delete (unsigned *)vidPtr;
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(ShardManager::getShardManager()->isCancelled()){
		return NULL;
	}

	Logger::sharding(Logger::Detail, "SHM| Metadata release VID=%d", metadataVersion);
	// All lock requesters waiting for a readview version <= this version will be unblocked.
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
	if(isCancelled()){
		return;
	}

	// find out the groupId of the operation that's waiting for this MM session to finish (or is listening
	// to any updates from it)
	unsigned groupId = migrationStatus.dstOperationId % MAX_NUM_TRANS_GROUPS;

	// lock only the corresponding group
	boost::unique_lock<boost::mutex> xLock(*(mmSessionListenersGroup.at(groupId).second));
	map<unsigned , ConsumerInterface *> & mmSessionListeners = mmSessionListenersGroup.at(groupId).first;
	map<unsigned, SP(Transaction)> & mmSessionListeners_TransHolder = mmSessionListenersGroup_TransSharedPointers.at(groupId);

	if(mmSessionListeners.find(migrationStatus.dstOperationId) == mmSessionListeners.end()){
		// nobody is listening for this update, so just leave.
		return;
	}

	// consumer found, first call threadBegin() of its transaction if
	// it's possible
	SP(Transaction) transaction = mmSessionListeners_TransHolder.find(migrationStatus.dstOperationId)->second;
	// prepare the transaction for continue of execution
	// for example, if it is a WriteviewTransaction, we re-lock the writeview here.
	// another example, the transaction keeps a shared pointer to itself to make sure
	// it doesn't die in the middle of this thread execution
	transaction->threadBegin(transaction);
	// pass the update to the consumer
	mmSessionListeners.find(migrationStatus.dstOperationId)->second->consume(migrationStatus);
	// continue of execution of this transaction is done. Do all
	// needed tasks: for example: free the X lock on writeview in case of a WriteviewTransaction
	transaction->threadEnd();
	// Stop tracking this MM session, it's already reported to the transaction which was waiting for it.
	mmSessionListeners.erase(mmSessionListeners.find(migrationStatus.dstOperationId));
	// also remove the shared pointer to transaction so that later when all SPs are released, this
	// transaction gets deleted.
	mmSessionListeners_TransHolder.erase(mmSessionListeners_TransHolder.find(migrationStatus.dstOperationId));

	Logger::sharding(Logger::Detail, "SHM| MM (%d => %d) was %s Processed.", migrationStatus.sourceNodeId,
			migrationStatus.destinationNodeId,
			(migrationStatus.status == MM_STATUS_SUCCESS)? "Done."  : "Failed.");
//    cout << "Shard Manager status after receiving migration manager notification:" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::clearMMRegistrations(){

	// Remove all ongoing MM sessions : shutdown in progress
	for(unsigned i = 0 ; i < MAX_NUM_TRANS_GROUPS; ++i){
		boost::unique_lock<boost::mutex> xLock(*(mmSessionListenersGroup.at(i).second));
		map<unsigned , ConsumerInterface *> & mmSessionListeners = mmSessionListenersGroup.at(i).first;
		map<unsigned, SP(Transaction)> & mmSessionListeners_TransHolder = mmSessionListenersGroup_TransSharedPointers.at(i);
		mmSessionListeners_TransHolder.clear();
	}
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
	// save the shared pointer to the responsible transaction to prevent its deallocation in the time
	// MM is working on this session (because maybe no other shared pointer to this transaction is saved
	// anywhere else such as StateMachine operations)
	mmSessionListeners_TransHolder[operationId] = listener->getTransaction();
}

void ShardManager::resolveSMNodeArrival(const Node & newNode){
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(isCancelled()){
		return;
	}
	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = ShardManager::getWriteview_write(xLock);
	SP(ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_write();
	Logger::sharding(Logger::Detail, "SHM| SM Node %d arrival.", newNode.getId());
	// add the new node information to the metadata. if it's the first time we see this node
	// a NotArrived label will be assigned to it until it actually joins the cluster
	// by its NodeJoiner transaction
    nodesWriteview->addNode(newNode);
	Logger::sharding(Logger::Detail, "SHM| SM Node %d arrival. Processed.", newNode.getId());
//    cout << "Shard Manager status after arrival of node " << newNode.getId() << ":" << endl;
//    ShardManager::getShardManager()->print();
//    cout << "======================================================================" << endl;
}

void ShardManager::resolveSMNodeFailure(const NodeId failedNodeId){
	Logger::sharding(Logger::Detail, "SHM| SM Node %d failure.", failedNodeId);
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	if(isCancelled()){
		return ;
	}
	SP(Notification) nodeFailureNotif(new NodeFailureNotification(failedNodeId));

	// 1. metadata manager
	// If some shards are assigned the failedNodeId, change them to UNASSIGNED state
	this->metadataManager->resolve(boost::dynamic_pointer_cast<NodeFailureNotification>(nodeFailureNotif));
	// 2. lock manager
	// If some locks are owned by an operation from failedNodeId, remove those lock tokens and
	// unlock the resources the had.
	this->_lockManager->resolveNodeFailure(failedNodeId);

	// 3. state machine
	// Also pass node failure notification to state machine so that if somebody is waiting
	// for a response from this node, it knows about this happening.
	this->stateMachine->handle(nodeFailureNotif);
	Logger::sharding(Logger::Detail, "SHM| SM Node %d failure. Processed.", failedNodeId);
}

void ShardManager::resolveTimeoutNotification(){
	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);

	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	vector<NodeId> failedNodes;
	nodesWriteview->getFailedNodes(failedNodes);
	nodesWriteview.reset();
	boost::unique_lock<boost::shared_mutex> shardMngrContentXLock(shardManagerMembersMutex);

	// Move on the list of failedNodes from the writeview and try to finalize its failure
	// if it's exactly the second time this periodic execution knows about the failure of that node.
	for(vector<NodeId>::iterator nodeItr = failedNodes.begin();  nodeItr != failedNodes.end();){
		if(failedNodesHandledByTimeout.find(*nodeItr) == failedNodesHandledByTimeout.end()){
			failedNodesHandledByTimeout[*nodeItr] = 1;
			 ++nodeItr;
		}else{
			if(failedNodesHandledByTimeout.at(*nodeItr) > 2){
				nodeItr = failedNodes.erase(nodeItr);
			}else{
				failedNodesHandledByTimeout[*nodeItr]++;
				++nodeItr;
			}
		}
	}
	shardMngrContentXLock.unlock();
	if(failedNodes.empty()){
		return;
	}

	for(vector<NodeId>::iterator nodeItr = failedNodes.begin();  nodeItr != failedNodes.end(); ++nodeItr){

		// so for those nodes that are failed and we haven't taken care of them yet,
		// just give a call to another entry we have.
		this->resolveSMNodeFailure(*nodeItr);
	}
//
//	SP(TimeoutNotification) timeoutNotif = SP(TimeoutNotification)(new TimeoutNotification());
//	ShardManager::getStateMachine()->handle(boost::dynamic_pointer_cast<Notification>(timeoutNotif));
}


/*
 * Because we want to avoid many possible deadlocks and have a much simpler code,
 * even the local part of a transaction is executed by a different thread. This method
 * passes this notification to the actul executor of this task.
 */
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
	if(ShardManager::getShardManager()->isCancelled()){
		return NULL;
	}
	if(! request->resolveNotification(request)){
		Logger::sharding(Logger::Detail, "SHM| Notification resolve returned false : %s", request->getDescription().c_str());
	}
	return NULL;
}

void ShardManager::cancelAllThreads(bool shouldLock ){
	if(shouldLock){
		shardManagerMembersMutex.lock();
	}

//	for(unsigned i = 0 ; shardManagerThreads.size(); ++i){
//		Logger::console("ShardManager threads are canceled.");
//		// Surendra -May be the code below is not required
//		#ifdef ANDROID
//			pthread_kill(*(this->shardManagerThreads[i]), SIGUSR2);
//		#else
//			pthread_cancel(*(this->shardManagerThreads[i]));
//		#endif
//	}
    // Surendra -May be the code below is not required
	#ifdef ANDROID
		pthread_kill(*(this->loloadBalancingThread), SIGUSR2);
	#else
		pthread_cancel(*(this->loadBalancingThread));
	#endif
	if(shouldLock){
		shardManagerMembersMutex.unlock();
	}

}

void * ShardManager::periodicWork(void *args) {

	boost::shared_lock<boost::shared_mutex> sLock(singleInstanceLock);
	while(! ShardManager::getShardManager()->isCancelled()){

		uint32_t sleepTime = ShardManager::getShardManager()->getLoadBalancingCheckInterval();
		// sleep before we check things again
		usleep(sleepTime);

		// 1. Resend bounced notifications.
		ShardManager::getShardManager()->resendBouncedNotifications();

		// 2. give timeout notification to shard manager
		ShardManager::getShardManager()->resolveTimeoutNotification();

		// 2. if we are joined, and no other load balancing process is on-going, start load balancing.
		if(ShardManager::getShardManager()->isJoined() && ! ShardManager::getShardManager()->isLoadBalancing()){
			ShardManager::getShardManager()->setLoadBalancing();
			LoadBalancer::runLoadBalancer();
		}

		/*
		 * NOTE : all debug info can now be collected through
		 *        http://www.hostname:port/_debug/stats
		 */
	    //ShardManager::getShardManager()->print();
	}
	return NULL;
}

void ShardManager::saveBouncedNotification(SP(ShardingNotification) notif){
	// switch src and dest addresses and save this notification to send it later.
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

void ShardManager::printBouncedNotifications(JsonResponseHandler * response){
	if(bouncedNotifications.empty()){
		return;
	}
	if(response != NULL){
		Json::Value bouncedNotifJson(Json::arrayValue);
		for(unsigned i = 0; i < bouncedNotifications.size(); ++i){
			SP(ShardingNotification) notif = bouncedNotifications.at(i);
			bouncedNotifJson[i]["address"] = notif->getDescription();
			bouncedNotifJson[i]["type"] = getShardingMessageTypeStr(notif->messageType());
		}
		response->setResponseAttribute("bounced-notifications", bouncedNotifJson);
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


