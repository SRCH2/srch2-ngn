#include "ShardManager.h"

#include "sharding/routing/RoutingManager.h"
#include "sharding/migration/ShardManagerCommand.h"
#include <map>



namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {



ShardManager::ShardManager(ConfigManager * config, DPInternalRequestHandler * dpInternal, RoutingManager * rm){
	this->configManager = config;
	this->dpInternal = dpInternal;
	this->routingManager = rm;
	this->routingManager->getInternalMessageHandler()->setShardManager(this);

	this->currentTransactionId = 0; // no ongoing transaction at the moment

	// check the clusterWriteview to see if we are master or client
	this->isMaster = configManager->getClusterWriteView()->getCurrentNode()->isMaster();

	// starts the cluster and makes sure cluster metadata is consistent on all nodes
	bootstrapCluster();

    pthread_t periodicExecuteThread;
    shouldExecute = true;
    // run for the same shard in a separate thread
    // the reason is that even the request message for some external threads are not sent
    // so if we use the same thread those external shards won't start the job until this shard
    // is finished which makes it sequential and wrong.
    if (pthread_create(&periodicExecuteThread, NULL, periodicExecute, this) != 0){
        //        Logger::console("Cannot create thread for handling local message");
        perror("Cannot create thread for handling local message");
        return;
    }
    // since we don't join this thread it must be detached so that its resource gets deallocated.
    pthread_detach(periodicExecuteThread);
}


// this method periodically executes and performs the tasks of ShardManager
void * ShardManager::periodicExecute(void * args){
	ShardManager * sm = (ShardManager *)args;
	sm->shardManagerGlobalLock.lock();
	bool shouldExecute = sm->shouldExecute;
	sm->shardManagerGlobalLock.unlock();
	while(shouldExecute){
		if(sm->isMasterNode()){
			Logger::console("Master: executing for one round.");
			sm->executeMaster();
		}else{
			Logger::console("Client: executing for one round.");
			sm->executeClient();
		}
		// sleep for two seconds
		sleep(2);
		// check if we should still execute
		sm->shardManagerGlobalLock.lock();
		shouldExecute = sm->shouldExecute;
		sm->shardManagerGlobalLock.unlock();
	}

	return NULL;
}
// executes the task of Master in one wakeup time
void ShardManager::executeMaster(){
	// TODO : what should master do periodically ?
}
// executes the task of a client in one wakeup time
void ShardManager::executeClient(){
	// TODO : what should client do periodically?
	// 1. check the Command Buffer to see if we received any commands from master
	NodeId masterNodeId = this->configManager->getClusterWriteView()->getMasterNodeId();
	SHMRequestReport * command;
	unsigned requestMessageId = 0;
	if(commandBuffer.getNextNodeCommand(masterNodeId, command, requestMessageId)){
		// command is from master
		switch (command->getCommandCode()) {
		case SHMRequestReport::SHM_TRANS_START:
		{
			resolveStartTransactionCommand(command->getTransactionId(), requestMessageId, masterNodeId);
			ASSERT(command->getMetadata() != NULL); // metadata should not be NULL
			ShardManagerCommandMetadata::TransactionType tType = command->getMetadata()->transactionType;
			// TODO : based on type client should start doing something ...
//			switch (tType) {
//				case value:
//
//					break;
//				default:
//					break;
//			}
			Logger::console("Client: START command received in executeClient, NOT EXPECTED.");
			ASSERT(false);
			delete command;
			break;
		}
		case SHMRequestReport::SHM_TRANS_COMMIT_COMPLETE:
		{
			// the last transaction is complete
			Logger::console("Client: Transaction completed.");
			resolveCompleteCommand();
			delete command;
			break;
		}
		case SHMRequestReport::SHM_TRANS_COMMIT:
		case SHMRequestReport::SHM_TRANS_START_CONFIRM:
		case SHMRequestReport::SHM_TRANS_START_ERROR_ONGOING:
		case SHMRequestReport::SHM_TRANS_START_ERROR_ABORTED:
		case SHMRequestReport::SHM_TRANS_START_ERROR_SUCCEED:
		case SHMRequestReport::SHM_TRANS_COMMIT_FAILED:
		case SHMRequestReport::SHM_TRANS_COMMIT_CONFIRM:
		case SHMRequestReport::SHM_BUSY:
		case SHMRequestReport::SHM_BOOTSTRAP_STATUS:
		case SHMRequestReport::SHM_BOOTSTRAP_DONE:
		{
			// These messages should not be buffered and seen here...
			ASSERT(false);
				break;
		}
		default:
			// what is it ?
			ASSERT(false);
			break;
		}
	}

	// no message from master
	// for now, we don't have anything else to do because clients don't message each other
	return;
}




//TODO : called only in the starting phase of this node,
// when this function is finished a thread periodically executes periodicExecute();
// there is also onetimeExecute(); which can be called by other modules to do the same job
void ShardManager::bootstrapCluster(){
	// load the existing shards
	bootstrapLoad();

	// synchronizes the shard information in cluster writeview
	// and commits the writeview. After this point readview is available to readers
	if(isMasterNode()){
		bootstrapSynchronizeMaster();
	}else{
		bootstrapSynchronizeClient();
	}

}


/*
 * The first thing done in startup :
 *   this function loads the existing shard indices and updates clusterWrite view
*  Phase: Load
*	All nodes load any shard that they can find in their directory structure.
*
*   Example 1:
*	/node1/core1/C1_P1/ *.idx
*	/node1/core1/C1_P3/ *.idx
*
*	Example 2:
*
*	/node1/core1/C1_P1/ *.idx
*	/node1/core1/C1_P2/ *.idx
*
*	And add the ShardIds of those shards to the CoreShardContainer object of their own node in cluster WriteView.
*	The state of cluster WriteView after reading indices from file.
*	Example 1:
*	Node 1 : 2 shards (C1_P1 and C1_P3)
*
*	Example 2:
*	Node 1 : 2 shards (C1_P1 and C1_P2)
 */
void ShardManager::bootstrapLoad(){

	// Lock the global lock so that resolveMessage knows that we are busy and replies with BUSY message very fast
	shardManagerGlobalLock.lock();

	Logger::console("Node going to BUSY mode, loading/creating local shards ...");
	//1. find the shardId of all indices saved in the directory structure.
	//   use Cluster.idx file to obtain this metadata about what's on the disk
	//   result is a map from shardId to a string which is the index directory
	// TODO
	map<ShardId, string> existingShards;
	//2. if existingShards is empty, for every core, find out if there is any json file for index creation
	// and use [coreId, nodeId, 0] for it's ShardId;
	// map is from coreId to its file path
	// TODO

	map<unsigned , string > jsonFileShards;

	//3. populate cluster writeview and add these shards to cluster
	Cluster * clusterWriteview = configManager->getClusterWriteView();
	std::vector<CoreShardContainer * > * nodeShardInfo =
			clusterWriteview->getNodeShardInformation_Writeview(clusterWriteview->getCurrentNode()->getId());
	// we assume the placeholder of nodeInfo of this node is already added to writeview by SM
	ASSERT(nodeShardInfo != NULL);


	// Temporary code : TODO : just to fill jsonFileShards map now
	for(std::vector<CoreShardContainer * >::iterator coreItr = nodeShardInfo->begin(); coreItr != nodeShardInfo->end(); ++coreItr){
		jsonFileShards.insert(std::make_pair((*coreItr)->getCore()->getCoreId(), ""));
	}


	ASSERT(nodeShardInfo != NULL);
	if(existingShards.empty()){
		for(std::vector<CoreShardContainer * >::iterator coreItr = nodeShardInfo->begin(); coreItr != nodeShardInfo->end(); ++coreItr){
			map<unsigned , string >::iterator coreEntryItr = jsonFileShards.find((*coreItr)->getCore()->getCoreId());
			if(coreEntryItr == jsonFileShards.end()){ // no json file for this core, we should add no core
				continue;
			}
			// TODO for future, we must get the path to JSON file from the jsonFileShards map, but currently it's
			// accessed from coreInfo_t so there can be only one json file for each core.

			// create the shard for json file
			Shard * newShard = new Shard(
					clusterWriteview->getCurrentNode()->getId(),
					(*coreItr)->getCore()->getCoreId(),
					clusterWriteview->getCurrentNode()->getId()); // if shards are created from JSON files, we use NodeId for Partition id

			// add the new shard to writeview
			(*coreItr)->getPrimaryShards()->push_back(newShard);

			// create the index and save Srch2Server pointer here
			configManager->createShardDir(configManager->getClusterWriteView()->getClusterName(),
					configManager->getClusterWriteView()->getCurrentNode()->getName(),
					(*coreItr)->getCore()->getName(), newShard->getShardId());
			string directoryPath = configManager->getShardDir(configManager->getClusterWriteView()->getClusterName(),
					configManager->getClusterWriteView()->getCurrentNode()->getName(),
					(*coreItr)->getCore()->getName(), newShard->getShardId());
			Logger::console("Loading/Creating the indexes from directory: %s", directoryPath.c_str());
			newShard->setSrch2Server(this->dpInternal->registerAndInitializeSrch2Server(newShard->getShardId(),
					(*coreItr)->getCore(), directoryPath));

		}
	}else{ // there are some existing shards so nothing needed from json files
		//add existingShards
		for(map<ShardId, string>::iterator shardItr = existingShards.begin(); shardItr != existingShards.end(); ++shardItr){
			for(std::vector<CoreShardContainer * >::iterator coreItr = nodeShardInfo->begin(); coreItr != nodeShardInfo->end(); ++coreItr){
				if(shardItr->first.coreId == (*coreItr)->getCore()->getCoreId()){
					vector<Shard *> * shardVector;
					if(shardItr->first.isPrimaryShard()){// add primary shard
						shardVector = (*coreItr)->getPrimaryShards();
					}else{ // add replica
						shardVector = (*coreItr)->getReplicaShards();
					}

					// create new shard
					Shard * existingShard = new Shard(
							clusterWriteview->getCurrentNode()->getId(),
							shardItr->first.coreId,
							shardItr->first.partitionId, shardItr->first.replicaId);

					// add this shard to writeview
					shardVector->push_back(existingShard);

//					// load the shard and set the srch2Server pointer
//					string directoryPath = configManager->getShardDir(configManager->getClusterWriteView()->getClusterName(),
//							configManager->getClusterWriteView()->getCurrentNode()->getName(),
//							(*coreItr)->getCore()->getName(), existingShard->getShardId());
					existingShard->setSrch2Server(this->dpInternal->registerAndInitializeSrch2Server(existingShard->getShardId(),
							(*coreItr)->getCore(), shardItr->second));
				}
			}
		}
	}

	// unlock global lock so that we can start replying messages, we are not busy anymore
	shardManagerGlobalLock.unlock();
}


/*
 * TODO
 * This function synchronizes the ShardManagers of all nodes in the cluster
 * in the startup phase
 * Master Execution :
 *
*	12. Master sends LOADING_STATUS to others and waits for LOADING_DONE message from all nodes.
*	13. (receives R11 from all nodes) aggregates shard information coming from all nodes
*	and updates its own cluster WriteView. Also, remember in some structure what shards are available.
*	14. Send the cluster WriteView to all nodes.
*	and wait for confirmation.
*	15. (receives R13 from all nodes) COMMIT.
 */
void ShardManager::bootstrapSynchronizeMaster(){

	Logger::console("Master Bootstrap Started ...");
	// 0. start a new transaction for bootstrap synchronization
	// initializes the current transaction ID and makes everyone synchronized
	startTransaction();
	// 1. prepare SHM_BOOTSTRAP_STATUS message and send it to others
	// a) prepare the command object
	SHMRequestReport * bootstrapStatusCommand = new SHMRequestReport(getCurrentTransactionId(), SHMRequestReport::SHM_BOOTSTRAP_STATUS);
	// b) prepare an aggregator for this command
	boost::shared_ptr<CommandAggregator> bootstrapStatusAggregator(new CommandAggregator(bootstrapStatusCommand, &(this->responseSynchronizer)));
	// prepare result synchronizer to wait for new commands
	this->responseSynchronizer.wait();
	// c) prepare a list of nodeIds of all nodes to sent this command to
	vector<NodeId> destinations;
	this->getListOfNodeIds(destinations);
	// d) use RM to send this command to others
	// wait for 2 seconds
	Logger::console("Master: broadcasting BOOT_STATUS command to nodes : %s.", this->getListOfNodeIdsString(destinations).c_str());
	time_t timeValue;
	time(&timeValue);
	timeValue = timeValue + 2;
	routingManager->broadcastToNodes<SHMRequestReport, SHMRequestReport>(bootstrapStatusCommand,
			true, true,
			bootstrapStatusAggregator,
			timeValue, destinations,
			configManager->getClusterWriteView()->getCurrentNode()->getId());


	// get Cluster * of each node's response
	std::map<NodeId, Cluster * > clusterInfos;
	while(true){
		Logger::console("Master: waiting for BOOT_STATUS response from nodes : %s", this->getListOfNodeIdsString(destinations).c_str());
		sleep(2);
		if(this->responseSynchronizer.isAvailable()){// responses came
			Logger::console("Master: BOOT_STATUS responses received.");
			for(unsigned nodeIndex = 0; nodeIndex < destinations.size(); ++nodeIndex){
				NodeId nodeId = destinations.at(nodeIndex);
				SHMRequestReport::CommandCode code ;
				if(responseSynchronizer.getReplyCommandCode(nodeId, code )){
					switch (code) {
						case SHMRequestReport::SHM_BOOTSTRAP_DONE:
						{
							// cluster info must be available
							Cluster * nodeWriteview = NULL;
							if(! responseSynchronizer.getReplyClusterInfo(nodeId, nodeWriteview)){
								ASSERT(false);
								// TODO : client sent a wrong message
							}
							clusterInfos.insert(std::make_pair(nodeId, nodeWriteview));
							Logger::console("Master: cluster info of node %d collected.", nodeId);
							break;
						}
						default:
						{
							// SHM_BOOTRSRAP_STATUS does not expect any other code in the response
							ASSERT(false);
							break;
						}
					}
				}else{ // timedout
					// TODO : we do nothing on timeout at this point for now
					Logger::console("Master: node %d timedout.", nodeId);
				}
			}//for
		}
	}

	// 3. aggregate all cluster metadata coming from all nodes and
	//    update the cluster writeView to be in the final state before load balancing
	//    also remember which shards are available
	Logger::console("Master: Using node cluster information to fill out master cluster writeview.");
	configManager->getClusterWriteView()->bootstrapMergeWithClientsInfo(clusterInfos);
	// refresh resultSynchronizer
	// also delete the copies of nodes
	responseSynchronizer.referesh();

	// 4. finalize this transaction by comitting the writeview and sending COMMIT command
	//    to all nodes.
	finalizeTransaction();
}


 /*
 * non-Master Execution :
 *
*   11. Sends master a LOADING_DONE confirmation which includes its own shard information.
*   12. (receives L14) receives the master’s cluster WriteView which must be consistent with
*   its own WriteView. Saves master’s WriteView in its own cluster.
*   13. COMMIT the cluster WriteView. And send confirmation (COMMIT_DONE) message to the master.
 *
 */
void ShardManager::bootstrapSynchronizeClient(){

	// 1. wait for master to start the first transaction, if it has already started
	// while we were loading, resolveMessage has replied with BUSY code and we should
	// wait for master to retry.
	// when start_transaction command is received, resolve it to open a new transaction
	NodeId masterNodeId = configManager->getClusterWriteView()->getMasterNodeId();
	SHMRequestReport * newCommand;
	unsigned requestMessageId = 0; // used for reply
	while(true){
		Logger::console("Client: waiting for TRANS_START command from master.");
		if(commandBuffer.getNextNodeCommand(masterNodeId, newCommand, requestMessageId)){
			if(newCommand->getCommandCode() == SHMRequestReport::SHM_TRANS_START){
				Logger::console("Client: TRANS_START received.");
				resolveStartTransactionCommand(newCommand->getTransactionId(), requestMessageId, masterNodeId);
				// ASSERT : metadata of TRANS_START should not be available
				ASSERT(newCommand->getMetadata() == NULL);
				break;
			}else{
				// TODO : at this point we must not receive any message from master other than opening a new transaction
				// because it's in bootstrap phase
				ASSERT(false);
				return;
			}
		}else{ // no message's received from master yet
			// sleep two seconds and check for start message again
			sleep(2);
		}
	}
	// 2. wait for master to ask for transaction status
	// a) receive the status request
	while(true){
		Logger::console("Client: waiting for BOOT_START command from master.");
		if(commandBuffer.getNextNodeCommand(masterNodeId, newCommand, requestMessageId)){
			if(newCommand->getCommandCode() == SHMRequestReport::SHM_BOOTSTRAP_STATUS){
				Logger::console("Client: BOOT_STATUS received.");
				// b) reply with cluster writeview data
				// b.1) create a reply SHM_BOOTSTRAP_DONE message
				SHMRequestReport * bootstrapDoneCommand = new SHMRequestReport(getCurrentTransactionId(),
						SHMRequestReport::SHM_BOOTSTRAP_DONE, this->configManager->getClusterWriteView());
				Message * bootstrapDoneReplyMsg = this->routingManager->prepareExternalMessage<SHMRequestReport>(NULL, bootstrapDoneCommand, false);

				// b.2) send the reply via TM
				bootstrapDoneReplyMsg->setRequestMessageId(requestMessageId);
				bootstrapDoneReplyMsg->setSHMReply();
				routingManager->getTransportManager().sendMessage(masterNodeId, bootstrapDoneReplyMsg, 0);
				Logger::console("Client: BOOT_DONE sent to the master.");
				routingManager->getMessageAllocator()->deallocateByMessagePointer(bootstrapDoneReplyMsg);
				delete bootstrapDoneCommand;
				break;
			}else{
				// TODO : at this point we must not receive any message from master other than requesting status of startup (bootstrap)
				ASSERT(false);
				return;
			}
		}else{ // no message's received from master yet
			// sleep two seconds and check for start message again
			sleep(2);
		}
	}

	// 3. wait for master to finalize this transaction,
	// a) wait for COMMIT COMMAND
	while(true){
		Logger::console("Client: waiting for TRANS_COMMIT command from master.");
		if(commandBuffer.getNextNodeCommand(masterNodeId, newCommand, requestMessageId)){
			if(newCommand->getCommandCode() == SHMRequestReport::SHM_TRANS_COMMIT){
				Logger::console("Client: TRANS_COMMIT received from master.");
				// b) use master writeview instead of current writeview (map the old ShardIds to new ones)
				// c) commit the writeview and reply with COMMIT_DONE
				resolveCommitCommand(newCommand->getClusterWriteview(), requestMessageId, masterNodeId);
				break;
			}else{
				// TODO : at this point we must not receive any message from master other than requesting status of startup (bootstrap)
				ASSERT(false);
				return;
			}
		}else{ // no message's received from master yet
			// sleep two seconds and check for start message again
			sleep(2);
		}
	}

	// d) wait for master to send COMPLETE
	// e) change the status of transaction to complete
	// this will be done in executeClient();
}


////////////////////////////////////////////////////////////////
////////////// Message resolver functions //////////////////////

// Responsibility of this class to delete request object, and response object if created.
// If this object returns NULL, it is responsible of deleting the request object
// If it returns a response object, everything will be deleting by RM after reply is sent and communication is
// complete.
SHMRequestReport * ShardManager::resolveMessage(Message* requestMessage, SHMRequestReport * request, NodeId node){
	ASSERT(requestMessage != NULL && request != NULL);
	// reply messages go through the pending message framework of RM
	ASSERT(requestMessage->isSHMReply() == false && requestMessage->isSHMInternal());

	// if message is not SHM_TRANS_START and it's transactionId is not the same as
	// current transactionId, we ignore it for now.
	// because it means the request is either from a future transaction or from an old one.
	if(request->getCommandCode() != SHMRequestReport::SHM_TRANS_START){
		ASSERT(request->getTransactionId() == getCurrentTransactionId());
		if(request->getTransactionId() != getCurrentTransactionId()){
			if(request->getTransactionId() < getCurrentTransactionId()){
				// TODO : request left from before
				Logger::console("Transaction code %s has too old id %d (current id : %d)", request->getCommandCodeString().c_str(),
						request->getTransactionId() , getCurrentTransactionId());
			}else{
				// TODO request has a greater TID, it's from future
				Logger::console("Transaction code %s has future id %d (current id : %d)", request->getCommandCodeString().c_str(),
						request->getTransactionId() , getCurrentTransactionId());
			}
			ASSERT(false);
			delete request;
			return NULL;
			// TODO : for future, we should do something about it
		}
	}

	switch (request->getCommandCode()) {
		case SHMRequestReport::SHM_TRANS_START:
		case SHMRequestReport::SHM_TRANS_COMMIT:
		case SHMRequestReport::SHM_TRANS_COMMIT_COMPLETE:
		case SHMRequestReport::SHM_BOOTSTRAP_STATUS:
			// if this node is busy, we reply busy, but it shouln't happen
			if(! shardManagerGlobalLock.try_lock()){
				// for now only start can get BUSY as reply
				ASSERT(request->getCommandCode() == SHMRequestReport::SHM_TRANS_START);
				Logger::console("This node is BUSY, command %s received.", request->getCommandCodeString().c_str());
				return new SHMRequestReport(getCurrentTransactionId(), SHMRequestReport::SHM_BUSY);
			}else{
				// we don't need the lock, we just check this lock to see if manager is busy or not
				shardManagerGlobalLock.unlock();
			}
			// save the command in command buffer
			this->commandBuffer.saveCommand(node, request, requestMessage->getMessageId());
			return NULL;
		case SHMRequestReport::SHM_TRANS_START_CONFIRM:
		case SHMRequestReport::SHM_TRANS_START_ERROR_ONGOING:
		case SHMRequestReport::SHM_TRANS_START_ERROR_ABORTED:
		case SHMRequestReport::SHM_TRANS_START_ERROR_SUCCEED:
		case SHMRequestReport::SHM_TRANS_COMMIT_FAILED:
		case SHMRequestReport::SHM_TRANS_COMMIT_CONFIRM:
		case SHMRequestReport::SHM_BUSY:
		case SHMRequestReport::SHM_BOOTSTRAP_DONE:
			// these cases are all replies and they should not happen
			ASSERT(false);
			delete request;
			return NULL;
			break;
		default:
			ASSERT(false);
			break;
	}
	return NULL;
}



bool ShardManager::isMasterNode(){
	return isMaster;
}

void ShardManager::initializeLocalShards(){
	// TODO : we ignore replicas at this point, it will be completed in phase 3
	Cluster * cluster = configManager->getClusterWriteView();

	std::vector<CoreShardContainer * > * currentNodeShardInfo =
			cluster->getNodeShardInformation_Writeview(cluster->getCurrentNode()->getId());

	for(unsigned cid = 0 ; cid < currentNodeShardInfo->size() ; ++cid){
		vector<Shard *> * primaryShards = currentNodeShardInfo->at(cid)->getPrimaryShards();

		for(unsigned sid = 0 ; sid < primaryShards->size() ; ++sid){

			ASSERT(primaryShards->at(sid)->getShardState() == SHARDSTATE_UNALLOCATED);

			primaryShards->at(sid)->setShardState(SHARDSTATE_INDEXING);
			configManager->createShardDir(cluster->getClusterName(),
									cluster->getCurrentNode()->getName(),
									currentNodeShardInfo->at(cid)->getCore()->getName(), primaryShards->at(sid)->getShardId());
			string directoryPath = configManager->getShardDir(cluster->getClusterName(),
					cluster->getCurrentNode()->getName(),
					currentNodeShardInfo->at(cid)->getCore()->getName(), primaryShards->at(sid)->getShardId());
			boost::shared_ptr<Srch2Server> srch2Server =
					this->dpInternal->registerAndInitializeSrch2Server(primaryShards->at(sid)->getShardId(),
							currentNodeShardInfo->at(cid)->getCore(), directoryPath);

			primaryShards->at(sid)->setShardState(SHARDSTATE_ALLOCATED);

			primaryShards->at(sid)->setSrch2Server(srch2Server);
		}

	}

	configManager->commitClusterMetadata();
}


/*
 * In this functions master starts a new transaction and makes sure all
 * other nodes' currentTransactionIds is up to our current transaction id
 * NOTE: all transactions are started by Master
 */
void ShardManager::startTransaction(){
	// 1. Get a new transaction id
	unsigned newTransactionId = getCurrentTransactionId() + 1;
	Logger::console("Master: starting a new transaction with id %d", newTransactionId);
	vector<NodeId> destinaitons;
	this->getListOfNodeIds(destinaitons);
	// 2. Send message to all other nodes to start the new transaction and give them the new Tid
	Logger::console("Master: broadcasting start transaction to nodes :  %s", this->getListOfNodeIdsString(destinaitons).c_str());
	broadcastStartTransactionCommand(destinaitons, newTransactionId);
	// 3. Wait for confirmation of all other nodes, when we get a response from a node
	//    a) CONFIRMATION, nothing
	//    b) node's current transaction is ONGOING, ABORTED or SUCCEED : TODO : look it up in the map and communicate with the node to handle this problem.
	//    c) node timedout : TODO : for now we just ignore this node.
	//    d) node busy : repeat the request after a wait time
	while(true){
		Logger::console("Master: waiting for start transaction response from nodes : %s", this->getListOfNodeIdsString(destinaitons).c_str());
		sleep(2);
		// check to see if results are available
		if(this->responseSynchronizer.isAvailable()){
			Logger::console("Master: nodes replied.");
			// collect busy nodes to resend the command,
			// bootstrap start command happens after LOAD and clients might still be busy
			vector<NodeId> busyNodes;
			// move on destinations and check the reply of each node
			for(unsigned nodeIndex = 0; nodeIndex < destinaitons.size(); ++nodeIndex){
				NodeId nodeId = destinaitons.at(nodeIndex);
				SHMRequestReport::CommandCode code ;
				if(responseSynchronizer.getReplyCommandCode(nodeId, code )){
					switch (code) {
						case SHMRequestReport::SHM_TRANS_START:
							ASSERT(false); // we should not receive this as a reply
							break;
						case SHMRequestReport::SHM_TRANS_START_CONFIRM:
							// Everything good
							Logger::console("Node %d started the new transaction. Everything good!!!", nodeId);
							break;
						case SHMRequestReport::SHM_TRANS_START_ERROR_ONGOING:
						case SHMRequestReport::SHM_TRANS_START_ERROR_ABORTED:
						case SHMRequestReport::SHM_TRANS_START_ERROR_SUCCEED:
							// TODO : we should into the map and see why these guys are not synch with master
							ASSERT(false);
							break;
						case SHMRequestReport::SHM_BUSY:
							busyNodes.push_back(nodeId);
							Logger::console("Node %d is still busy.", nodeId);
							break;
						default:
							ASSERT(false);
							break;
					}
				}else{ // timedout
					// TODO : we do nothing on timeout at this point for now
					Logger::console("Master: node %d timedout.", nodeId);
				}
			}// end of for

			if(busyNodes.size() == 0){
				// TODO : later in future we want to do more communication with non-confirmed nodes
				// for now we just finish waiting.
				break;
			}else{
				// repeat the process, send the command to these nodes again
				// 1. first, refresh the result synchronizer
				this->responseSynchronizer.referesh();
				// 2. send start transaction message to these nodes again
				destinaitons = busyNodes;
				broadcastStartTransactionCommand(destinaitons, newTransactionId);
			}
		}
	}

	// 4. when we have the confirmation from all nodes,
	// a) refresh the resultsSynchronizer
	this->responseSynchronizer.referesh();
	// b) move this node to the new transaction : move the current transaction id forward and register it
	setCurrentTransactionIdAndRegister(newTransactionId);
	Logger::console("Transaction successfully started, new TID is : %d", this->getCurrentTransactionId());
}


void ShardManager::resolveStartTransactionCommand(unsigned newTransactionId, unsigned requestMessageId, NodeId requestNodeId){
	//Starts upon receiving a command from master to start the new transaction
	//NOTE: this means a SHM_TRANS_START is already received by resolveMessage
	// 1. Check the current transaction of this node, see what's going on now :
	//    a) COMPLETE : nothing
	//    b) ABORTED,COMMITTED  :
	//          for now delete the current tID from map, we want to forget this transaction
	//          but for future : TODO : ask the master about this, why is that?
	//    c) ONGOING : TODO we should send the id to the master and .... For now, we don't do anything and it times out

	TransactionStatus currentStatus = getTransactionStatus(getCurrentTransactionId());
	switch (currentStatus) {
		case ShardManager_Transaction_OnGoing:
			// TODO : In future, send the current TID to the master see what's going on
			Logger::console("Client: Current transaction is still on-going.");
			ASSERT(false);
			break;
		case ShardManager_Transaction_Aborted:
		case ShardManager_Transaction_Committed:
			// forget this transaction (for now)
			Logger::console("Client: Current transaction is aborted/committed.");
			ASSERT(false);
			setCurrentTransactionStatus(ShardManager_Transaction_Completed);// setting to complete actually deletes the transaction id from map
			// TODO : in future we must communicate with master to see why is that ...
			break;
		case ShardManager_Transaction_Completed:
			// last transaction is finished, everything good ...
			break;
		default:
			ASSERT(false);
			break;
	}

	// 2. move to the new transaction and register it in map
	// a) check if this new transaction id is OK
	if(getTransactionStatus(newTransactionId) != ShardManager_Transaction_Completed){
		// TODO : this new transaction id is actually not new, possibly message left from before ...
		// For now, we just ignore it
		Logger::console("Client: new transaction id is found in the map!!!!");
		ASSERT(false);
		return;
	}
	setCurrentTransactionIdAndRegister(newTransactionId);
	// 3. send the confirmation
	// a) prepare the reply object
	SHMRequestReport * confirmationReply = new SHMRequestReport(newTransactionId, SHMRequestReport::SHM_TRANS_START_CONFIRM);

	Message * confirmationReplyMsg = this->routingManager->prepareExternalMessage<SHMRequestReport>(NULL, confirmationReply, false);

	confirmationReplyMsg->setRequestMessageId(requestMessageId);
	confirmationReplyMsg->setSHMReply();
	routingManager->getTransportManager().sendMessage(requestNodeId, confirmationReplyMsg, 0);
	Logger::console("Client: TRANS_START_CONFIRM is sent to master, node id is : %d" , requestNodeId);
	routingManager->getMessageAllocator()->deallocateByMessagePointer(confirmationReplyMsg);
	delete confirmationReply;

}


void ShardManager::finalizeTransaction(){
	// 1. commit cluster writeview to readview and
	//    change the status of current transaction to SUCCEED
	Logger::console("Master: committing the writeview.");
	configManager->commitClusterMetadata();
	this->setCurrentTransactionStatus(ShardManager_Transaction_Committed);

	// 2. send COMMIT command to all nodes in the cluster
	// a) prepare the COMMIT command object
	SHMRequestReport * commitCommandObj = new SHMRequestReport(getCurrentTransactionId(),
			SHMRequestReport::SHM_TRANS_COMMIT,
			configManager->getClusterWriteView());
	// b) prepare the COMMIT command aggregator
	boost::shared_ptr<CommandAggregator> commitAggregator(new CommandAggregator(commitCommandObj, &(this->responseSynchronizer)));
	// set synchronizer to wait for all confirmations to come
	this->responseSynchronizer.wait();
	// c) prepare a list of nodeIds of other nodes for broadcast
	vector<NodeId> destinations;
	this->getListOfNodeIds(destinations);
	// d) use RM to do this broadcast
	// wait for 2 seconds
	Logger::console("Master: broadcasting COMMIT command to nodes : %d", getListOfNodeIdsString(destinations).c_str());
	time_t timeValue;
	time(&timeValue);
	timeValue = timeValue + 2;
	routingManager->broadcastToNodes<SHMRequestReport, SHMRequestReport>(commitCommandObj,
			true, true,
			commitAggregator,
			timeValue, destinations,
			configManager->getClusterWriteView()->getCurrentNode()->getId());

	// 3. wait for all nodes to confirm commit
	// when all confirmations came,
	// a) change the status of current transaction to COMPLETE
	// keep track of nodes that confirmed because we want to send COMPLETE to them
	vector<NodeId> confirmedNodes;
	while(true){
		Logger::console("Master: waiting for COMMIT response from nodes : %s", this->getListOfNodeIdsString(destinations).c_str());
		sleep(2);
		// check to see if results are available
		if(this->responseSynchronizer.isAvailable()){

			for(unsigned nodeIndex = 0; nodeIndex < destinations.size(); ++nodeIndex){
				NodeId nodeId = destinations.at(nodeIndex);
				SHMRequestReport::CommandCode code ;
				if(this->responseSynchronizer.getReplyCommandCode(nodeId, code)){
					switch (code) {
						case SHMRequestReport::SHM_TRANS_COMMIT_CONFIRM:
							// this node confirmed, everything good ....
							confirmedNodes.push_back(nodeId);
							Logger::console("Master: Node %d confirmed commit.", nodeId);
							break;
						case SHMRequestReport::SHM_TRANS_COMMIT_FAILED:
//							// TODO : this node failed to commit, which means
							//        now it is behind the cluster, for now we just ignore
							//        this because we don't care about failure but we should
							//        do more conversation with this node in future to see
							//        what's wrong ...
							// 1. get the cluster writeview of this node ...
							// Cluster * nodeWriteview = NULL;
							// this->resultSynchronizer.getReplyClusterInfo(nodeId, nodeWriteview);
							ASSERT(false);
							break;
						default:
							ASSERT(false); // for now, no other code can be received
							break;
					}
				}else{ // node has timed out
					//TODO : for now, we ignore nodes that fail w/o committing. Later we
					//        might start another conversation with this node to see
					//        what's going in ....
					Logger::console("Master: node %d timedout.", nodeId);
				}
			}
			// Confirmations are received, change the status of current transaction
			setCurrentTransactionStatus(ShardManager_Transaction_Completed);
			break;
		}
	}
	// refresh the result synchronizer here
	this->responseSynchronizer.referesh();
	// c) send COMPLETE command to all nodes with no callback or aggragator

	// c-1) prepare the COMPLETE command object
	SHMRequestReport * completeCommandObj = new SHMRequestReport(getCurrentTransactionId(),
			SHMRequestReport::SHM_TRANS_COMMIT_COMPLETE);
	// c-2) we don't need an aggregator for this command because we don't wait for it
	boost::shared_ptr<CommandAggregator> completeAggregator;
	Logger::console("Master: broadcasting COMPLETE command to nodes : %d", getListOfNodeIdsString(destinations).c_str());
	routingManager->broadcastToNodes<SHMRequestReport, SHMRequestReport>(completeCommandObj,
			false, false,
			completeAggregator,
			timeValue, confirmedNodes,
			configManager->getClusterWriteView()->getCurrentNode()->getId());
	// done.
}


void ShardManager::resolveCommitCommand(Cluster * masterClusterWriteview, unsigned requestMessageId, NodeId requestNodeId){
	// Starts upon receiving a command from master to commit the current transaction
	// NOTE: this means a SHM_TRANS_COMMIT is already received by resolveMessage
	// 1.
	// a) merge local writeview into the master copy
	if(! Cluster::mergeLocalIntoMaster(this->configManager->getClusterWriteView(), masterClusterWriteview)){
		// could not successfully merge master's meta data with current writeview
		// TODO : for now, this case shouldn't happen, we return and master will timeout
		//        but for future we must comminunicate with master and reply
		//        SHM_TRANS_COMMIT_FAILED along with the current writeview
		ASSERT(false);
		return;
	}
	// b) change local writeview pointer to use master copy
	this->configManager->setClusterWriteViewAndDeleteTheOldOne(masterClusterWriteview);
	Logger::console("Client: used master cluster info for writeview.");
	// 2. commit the new writeview
	configManager->commitClusterMetadata();
	Logger::console("Client: committed the cluster metadata.");
	// 3. change the status of current transaction to COMMITED
	this->setCurrentTransactionStatus(ShardManager_Transaction_Committed);

	// 4. Reply master's message with CONFIRMATION
	// a) prepare the confirmation object
	// b) prepare the confirmation message
	// c) send the confirmation message to master
	SHMRequestReport * confirmationReply = new SHMRequestReport(getCurrentTransactionId(), SHMRequestReport::SHM_TRANS_COMMIT_CONFIRM);

	Message * confirmationReplyMsg = this->routingManager->prepareExternalMessage<SHMRequestReport>(NULL, confirmationReply, false);

	confirmationReplyMsg->setRequestMessageId(requestMessageId);
	confirmationReplyMsg->setSHMReply();
	routingManager->getTransportManager().sendMessage(requestNodeId, confirmationReplyMsg, 0);
	Logger::console("Client: TRANS_COMMIT_CONFIRM is sent to the master.");
	routingManager->getMessageAllocator()->deallocateByMessagePointer(confirmationReplyMsg);
	delete confirmationReply;

}


void ShardManager::resolveCompleteCommand(){
	// TODO for future : check if complete command is arrived for
	// a transaction that is already SUCCEED.
	// Starts upon receiving a command from master to complete the current transaction
	// NOTE: this means a SHM_TRANS_COMMIT_COMPLETE is already received by resolveMessage
	// 1. Change the status of current transaction to COMPLETE
	setCurrentTransactionStatus(ShardManager_Transaction_Completed);
}

void ShardManager::getListOfNodeIds(vector<NodeId> & nodeIds){
	Cluster * cluster = configManager->getClusterWriteView();
	vector<const Node *> nodes;
	cluster->getAllNodes(nodes);
	for(unsigned nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex){
		nodeIds.push_back(nodes.at(nodeIndex)->getId());
	}
}

string ShardManager::getListOfNodeIdsString(const vector<NodeId> & nodeIds){
	stringstream result;
	result << "{";
	for(unsigned nodeIndex = 0; nodeIndex < nodeIds.size(); ++nodeIndex){
		result << nodeIds.at(nodeIndex);
		if(nodeIndex < nodeIds.size() - 1){
			result << ", ";
		}
	}
	result << "}";
	return result.str();
}


void ShardManager::broadcastStartTransactionCommand(vector<NodeId> & destinations, unsigned newTransactionId){
	// a) create the start command object
	SHMRequestReport * startTransCommnad = new SHMRequestReport(newTransactionId, SHMRequestReport::SHM_TRANS_START);
	// b) create a command aggregator for this command and connect it to resultSynchronizer
	ASSERT(responseSynchronizer.isAvailable());
	boost::shared_ptr<CommandAggregator> startTransCommandAggr(new CommandAggregator(startTransCommnad, &(this->responseSynchronizer)));
	// make it unavailable until aggregator releases it
	this->responseSynchronizer.wait();

	// c) prepare a list of nodeIds for this broadcast

	// d) use RM to do this broadcast
	// wait for 2 seconds
	time_t timeValue;
	time(&timeValue);
	timeValue = timeValue + 2;
	routingManager->broadcastToNodes<SHMRequestReport, SHMRequestReport>(startTransCommnad,
			true, true,
			startTransCommandAggr,
			timeValue, destinations,
			configManager->getClusterWriteView()->getCurrentNode()->getId());
}


void ShardManager::setCurrentTransactionIdAndRegister(unsigned newTransationId){
	transactionsStatusLock.lock();

	ASSERT(newTransationId > currentTransactionId); // we assume TIDs are always increasing
	std::map<unsigned, TransactionStatus>::iterator transactionStatusItr =
			transactionsStatus.find(newTransationId);
	if(transactionStatusItr != transactionsStatus.end()){ // it's already in the map
		ASSERT(false);
		// TODO : might be a case of failure, probably a message came too late
		transactionsStatusLock.unlock();
		return;
	}
	currentTransactionId = newTransationId;
	transactionsStatus.insert(std::make_pair(currentTransactionId, ShardManager_Transaction_OnGoing));

	transactionsStatusLock.unlock();
}
TransactionStatus ShardManager::getTransactionStatus(unsigned transactionId){
	transactionsStatusLock.lock_shared();

	std::map<unsigned, TransactionStatus>::iterator transactionStatusItr =
			transactionsStatus.find(transactionId);
	if(transactionStatusItr == transactionsStatus.end()){ // not in the map so it's completed and removed.
		return ShardManager_Transaction_Completed;
	}
	TransactionStatus result = transactionStatusItr->second;

	transactionsStatusLock.unlock_shared();
	return result;
}
unsigned ShardManager::getCurrentTransactionId(){
	transactionsStatusLock.lock_shared();

	unsigned result = currentTransactionId;

	transactionsStatusLock.unlock_shared();
	return result;
}

void ShardManager::setCurrentTransactionStatus(TransactionStatus status){
	transactionsStatusLock.lock();
	std::map<unsigned, TransactionStatus>::iterator transactionStatusItr =
			transactionsStatus.find(currentTransactionId);
	if(transactionStatusItr == transactionsStatus.end()){ // it's already in the map
		ASSERT(false);
		transactionsStatusLock.unlock();
		return;
	}

	if(status == ShardManager_Transaction_Completed){
		transactionsStatus.erase(transactionStatusItr);
	}else{
		transactionStatusItr->second = status;
	}
	transactionsStatusLock.unlock();
}


}
}
