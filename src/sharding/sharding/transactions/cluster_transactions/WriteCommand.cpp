#include "WriteCommand.h"


#include "../../state_machine/StateMachine.h"
#include "../../metadata_manager/Cluster_Writeview.h"
#include "../../metadata_manager/Cluster.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

// Note: we will use ClusterPID() for the case of node shards
//       ClusterPID() == (-1,-1,-1)
PartitionWriter::PartitionWriter(const ClusterPID & pid,
		const ClusterRecordOperation_Type & insertUpdateDelete,
		vector<RecordWriteOpHandle *> & records, const vector<NodeTargetShardInfo> & targets,
		ConsumerInterface * consumer):ProducerInterface(consumer),
		pid(pid),
		insertUpdateDelete(insertUpdateDelete){
	ASSERT(consumer != NULL);
	ASSERT(consumer->getTransaction());
	ASSERT(consumer->getTransaction()->getSession() != NULL);
	locker = NULL;
	releaser = NULL;
	currentStep = StepPreStart;
	initTargets(targets);
	initRecords(records);
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->clusterReadview = ((ReadviewTransaction *)(this->getConsumer()->getTransaction().get()))->getReadview();
}

SP(Transaction) PartitionWriter::getTransaction(){
	return this->getConsumer()->getTransaction();
}

void PartitionWriter::produce(){
	/*
	 * 0. check if we should start (targets and records should not be empty.)
	 * 1. if cluster shards, we need distributed concurrency control so : call lock();
	 * 2. if node shards, only this node can write to the shard so we can directly : call performWrite()
	 */
	if(targets.empty()){
		// we should not do anything.
		ASSERT(false);
		finalize(false, HTTP_Json_General_Error);
		return;
	}
	if(records.empty()){
		// just call consume
		ASSERT(false);
		finalize(false, HTTP_Json_General_Error);
		return;
	}
	if(this->clusterOrNodeFlag){
		// case of cluster shard core write request
		// so pid must not be equal to ClusterPID()
		if(pid == ClusterPID()){
			finalize(false, HTTP_Json_General_Error);
			ASSERT(false);
			return;
		}
		// For example for the case of record insertion, acquire X locks on
		// primary keys on the host nodes of all replicas
		lock();
	}else{
		// case of node shard core write request
		if(pid != ClusterPID()){
			finalize(false, HTTP_Json_General_Error);
			ASSERT(false);
			return;
		}
		performWrite();
	}
}

void PartitionWriter::lock(){
	// 1. initializing lock related members
	// sort the records based on primary key
	// Note: list of primary keys must be sorted alphabetically
	//       to avoid two separate lists of records get stuck in a
	//       a deadlock at the host nodes lock managers.
	std::sort(records.begin(), records.end(), RecordComparator());
	// prepare the list of primary keys
	vector<string> primaryKeys;
	for(unsigned i = 0 ; i < records.size(); ++i){
		primaryKeys.push_back(records.at(i)->getPrimaryKey());
	}
	// locker object to perform the distributed lock request
	locker = new AtomicLock(primaryKeys, currentOpId, pid, this);
	currentStep = StepLock;
	locker->produce();
}

/*
 * Consume used for cluster shard case, the lock operation is finished and we can use the result.
 */
void PartitionWriter::consume(bool granted, const ClusterPID & pid){
	ASSERT(pid == this->pid);
	switch (currentStep) {
		case StepLock:
			if(granted){
				// primary key locks are granted
				ask2PC();
			}else{
				// the partition is locked and not ready to accepts any write operations
				// so we abort
				finalize(false, HTTP_Json_Partition_Is_Locked);
			}
			break;
		case StepRelease:
			if(granted){
				// primary key locks are released, we are done.
				finalize(false, HTTP_Json_General_Error);
			}else{
				// release must never be rejected
				ASSERT(false);
				finalize(false, HTTP_Json_General_Error);
			}
			break;
		default:
			// we should not reach this method for any other steps of the process
			ASSERT(false);
			finalize(false, HTTP_Json_General_Error);
			break;
	}
}


void PartitionWriter::ask2PC(){
	if(! validateAndFixTargets()){
		finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
		return;
	}
	currentStep = StepAsk2PC;

	// start the ask phase of 2PC
	sendWriteCommand(WriteNotificationModeAsk2PC);
}

void PartitionWriter::processAsk2PCResponse(map<NodeId, SP(ShardingNotification) > & _replies){

	// iterate on the nodes and check each reply to check which nodes
	// confirmed which record for the first phase of 2PC.

	// map will summarize result of 2PC ASK phase for each key which is a primary key
	map<string, bool > conjuncted2PCStatusValue;
	for(map<NodeId, SP(ShardingNotification) >::iterator nodeItr = _replies.begin(); nodeItr != _replies.end(); ++nodeItr){
		SP(Write2PCNotification::ACK) ack = boost::dynamic_pointer_cast<Write2PCNotification::ACK>(nodeItr->second);
		// map from a primary key to a list of results corresponding to all replicas
		map<string, vector<Write2PCNotification::ACK::ShardResult *> > & shardResults = ack->getResults();

		// iterate on records and check the 2PC ask phase answer for each record
		for(unsigned recIdx = 0; recIdx < records.size(); ++recIdx){
			RecordWriteOpHandle * recordHandle = records.at(recIdx);
			const string & primaryKey = recordHandle->getPrimaryKey();

			// is there actually any answer given for this record?
			if(shardResults.find(primaryKey) == shardResults.end()){
				ASSERT(false);
				recordHandle->addMessage(NULL, HTTP_Json_General_Error);
				conjuncted2PCStatusValue[primaryKey] = false;
			}else{
				vector<Write2PCNotification::ACK::ShardResult *> & primaryKeyShardResults = shardResults[primaryKey];
				// all replicas that will have to accept this primary key must confirm
				// AND the answers
				for(unsigned i = 0; i < primaryKeyShardResults.size(); ++i){
					Write2PCNotification::ACK::ShardResult * pkShardResultIth = primaryKeyShardResults.at(i);
					if(conjuncted2PCStatusValue.find(primaryKey) == conjuncted2PCStatusValue.end()){
						conjuncted2PCStatusValue[primaryKey] = pkShardResultIth->statusValue;
					}else{
						// AND
						conjuncted2PCStatusValue[primaryKey] = conjuncted2PCStatusValue[primaryKey] && pkShardResultIth->statusValue;
					}
					// Also store the produced messages in this phase
					for(unsigned mIdx = 0 ; mIdx < pkShardResultIth->messageCodes.size(); ++mIdx){
						recordHandle->addMessage(pkShardResultIth->shardId, pkShardResultIth->messageCodes.at(mIdx));
					}
				}
			}
		}
	}
	// now move on records and finalize those that are rejected in 2pc ask phase
	for(map<string, bool >::iterator pkItr = conjuncted2PCStatusValue.begin(); pkItr != conjuncted2PCStatusValue.end(); ++pkItr){
		// move on records and either leave them to continue to writePerform phase or
		// fail them and finalize them.
		if(! pkItr->second){
			// this primary key could not pass the 2pc ask phase
			if(recordsMap.find(pkItr->first) == recordsMap.end()){
				ASSERT(false);
				continue;
			}
			recordsMap[pkItr->first]->finalize(false);
		}
	}

	// now we move to the write phase. If no record
	// has passed the ASK phase, it will be detected in performWrite
	// and the task will be finalized w/o doing more distributed process
	performWrite();
}

// calls sendWriteCommand with WriteNotificationModePerformWrite argument indicating write perform phase
void PartitionWriter::performWrite(){
	if(! validateAndFixTargets()){
		finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
		return;
	}
	currentStep = StepWrite;
	sendWriteCommand(WriteNotificationModePerformWrite);
}


/*
 * Processes the actual write command responses and saves the status of each record write operation
 * to be reported to the user of this module.
 */
void PartitionWriter::processWriteResponse(map<NodeId, SP(ShardingNotification) > & _replies){
	map<string, bool > conjuncted2PCStatusValue;
	for(map<NodeId, SP(ShardingNotification) >::iterator nodeItr = _replies.begin(); nodeItr != _replies.end(); ++nodeItr){
		SP(Write2PCNotification::ACK) ack = boost::dynamic_pointer_cast<Write2PCNotification::ACK>(nodeItr->second);
		map<string, vector<Write2PCNotification::ACK::ShardResult *> > & shardResults = ack->getResults();
		for(unsigned recIdx = 0; recIdx < records.size(); ++recIdx){
			RecordWriteOpHandle * recordHandle = records.at(recIdx);
			const string & primaryKey = recordHandle->getPrimaryKey();
			if(shardResults.find(primaryKey) == shardResults.end()){
				if(! recordHandle->isWorkDone()){
					ASSERT(false);
					conjuncted2PCStatusValue[primaryKey] = false;
				}
			}else{
				vector<Write2PCNotification::ACK::ShardResult *> & primaryKeyShardResults = shardResults[primaryKey];
				for(unsigned i = 0; i < primaryKeyShardResults.size(); ++i){
					Write2PCNotification::ACK::ShardResult * pkShardResultIth = primaryKeyShardResults.at(i);

					// AND the result of all replicas
					if(conjuncted2PCStatusValue.find(primaryKey) == conjuncted2PCStatusValue.end()){
						conjuncted2PCStatusValue[primaryKey] = pkShardResultIth->statusValue;
					}else{
						conjuncted2PCStatusValue[primaryKey] = conjuncted2PCStatusValue[primaryKey] && pkShardResultIth->statusValue;
					}
					for(unsigned mIdx = 0 ; mIdx < pkShardResultIth->messageCodes.size(); ++mIdx){
						recordHandle->addMessage(pkShardResultIth->shardId, pkShardResultIth->messageCodes.at(mIdx));
					}
				}
			}
		}
	}
	// now move on records and finalize those that are rejected in 2pc ask phase
	for(map<string, bool >::iterator pkItr = conjuncted2PCStatusValue.begin(); pkItr != conjuncted2PCStatusValue.end(); ++pkItr){
		// move on records and either leave them to continue to writePerform phase or
		// fail them and finalize them.
		if(recordsMap.find(pkItr->first) == recordsMap.end()){
			ASSERT(false);
			continue;
		}
		recordsMap[pkItr->first]->finalize(pkItr->second);
	}

	if(! validateAndFixTargets()){
		finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
		return;
	}

	vector<string> primaryKeys;
	for(unsigned i = 0 ; i < records.size(); ++i){
		primaryKeys.push_back(records.at(i)->getPrimaryKey());
	}
	if(! this->getConsumer()->getTransaction()){
		this->getConsumer()->getTransaction();
		ASSERT(false);
	}

	// write is done, release primary key locks
	releaser = new AtomicRelease(primaryKeys, currentOpId, pid, this);
	currentStep = StepRelease;
	releaser->produce();
}


/*
 * When all reply notifications resulted from call to sendWriteCommnad
 * reach to this node and ConcurrentNotifOperation
 * In fact the replies coming to this callback are either for the ASK phase
 * of 2PC or for the 'write' phase.
 */
void PartitionWriter::end(map<NodeId, SP(ShardingNotification) > & replies){
	if(replies.empty()){
		finalize(false, HTTP_Json_General_Error);
		return;
	}
	if(currentStep == StepAsk2PC){
		processAsk2PCResponse(replies);
	}else if(currentStep == StepWrite){
		processWriteResponse(replies);
	}else{
		ASSERT(false);
		finalize(false, HTTP_Json_General_Error);
	}
}

void PartitionWriter::finalize(bool defaultStatusValue, const JsonMessageCode & defaultMessageCode){ // calls callback on consumer
	// move on all records and if they are not done yet,
	// finalize them
	for(unsigned recIdx = 0; recIdx < records.size(); ++recIdx){
		if(! records.at(recIdx)->isWorkDone()){
			records.at(recIdx)->addMessage(NULL, defaultMessageCode);
			records.at(recIdx)->finalize(defaultStatusValue);
		}
	}
	if(this->clusterOrNodeFlag){
		this->getConsumer()->consume(this->pid);
	}else{
		ASSERT(pid == ClusterPID());
		this->getConsumer()->consume();
	}
}

/*
 * We can write something similar to state machine
 * node iterator operators that uses different schemes to call producer and
 * consumer classes: like calling some sub-transactions in parallel or serial.
 */


string PartitionWriter::getName() const {
	return "partition-writer";
}


/*
 * Based on the current step, prepares either ASK notifications or
 * WRITE notifications with sufficient information and sends it to all
 * involved nodes.
 */
void PartitionWriter::sendWriteCommand(WriteNotificationMode mode){

	vector<RecordWriteOpHandle *> * recordsToPass = NULL;
	vector<RecordWriteOpHandle *> successful2PCRecords;
	if(mode == WriteNotificationModeAsk2PC){
		recordsToPass = &(this->records);
	}else if(mode == WriteNotificationModePerformWrite){
		for(unsigned i = 0 ; i < this->records.size(); ++i){
			if(! this->records.at(i)->isWorkDone()){
				successful2PCRecords.push_back(this->records.at(i));
			}
		}
		if(successful2PCRecords.empty()){
			finalize(false, HTTP_Json_General_Error);
			return;
		}
		recordsToPass = &successful2PCRecords;
	}else{
		finalize(false, HTTP_Json_General_Error);
		ASSERT(false);
		return;
	}
	vector<std::pair< SP(ShardingNotification), NodeId> > requests;
	for(unsigned targetIdx = 0; targetIdx < targets.size(); ++targetIdx){
		NodeTargetShardInfo & target = targets.at(targetIdx);
		requests.push_back(std::make_pair(
				SP(Write2PCNotification)(new Write2PCNotification(clusterReadview, insertUpdateDelete,
						target, mode , *recordsToPass)),
						target.getNodeId())
		);
	}
	ConcurrentNotifOperation * concurrent2PCAggregator =
			new ConcurrentNotifOperation(ShardingWriteCommand2PCACKMessageType, requests, this);

	ShardManager::getShardManager()->getStateMachine()->registerOperation(concurrent2PCAggregator);
}

void PartitionWriter::initTargets(const vector<NodeTargetShardInfo> & targets){
	/*
	 * 1. check node targets to see whether our insertion targets
	 *    are cluster shards or node shards.
	 *    NOTE: We also do one consistency check here :
	 *    all NodeTargetShardInfo objects must either have only
	 *    cluster shards or only node shards.
	 *
	 */
	for(vector<NodeTargetShardInfo>::const_iterator nItr = targets.begin(); nItr != targets.end(); ++nItr){
		const NodeTargetShardInfo & target = *nItr;
		if(nItr == targets.begin()){
			clusterOrNodeFlag = target.isClusterShardsMode();
		}else{
			bool newValue = target.isClusterShardsMode();
			if(newValue != clusterOrNodeFlag){
				ASSERT(false);
				return;
			}
		}
		this->targets.push_back(target);
	}
}
bool PartitionWriter::validateAndFixTargets(){
	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
	for(vector<NodeTargetShardInfo>::iterator nItr = targets.begin(); nItr != targets.end(); ++nItr){
		if(! nodesWriteview->isNodeAlive(nItr->getNodeId())){
			nItr = targets.erase(nItr);
		}
	}
	if(targets.empty()){
		return false;
	}
	return true;
}

/************** Record containers *******************/
void PartitionWriter::initRecords(vector<RecordWriteOpHandle *> & records){
	for(unsigned i = 0; i < records.size(); ++i){
		this->records.push_back(records.at(i));
		this->recordsMap[records.at(i)->getPrimaryKey()] = records.at(i);
	}
}

WriteCommand::WriteCommand(ConsumerInterface * consumer,
		vector<srch2is::Record *> records,
		ClusterRecordOperation_Type insertUpdateDelete,
		const CoreInfo_t * coreInfo):ProducerInterface(consumer){
	ASSERT(coreInfo != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	ASSERT(this->getConsumer()->getTransaction()->getSession() != NULL);
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->coreInfo = coreInfo;
	this->clusterReadview = ((ReadviewTransaction *)(this->getConsumer()->getTransaction().get()))->getReadview();
	this->response = (JsonRecordOperationResponse*) (this->getConsumer()->getTransaction()->getSession()->response);
	initActionType(insertUpdateDelete);
	ASSERT(this->insertUpdateDelete != Delete_ClusterRecordOperation_Type);
	initRecords(records);
	this->nodeWriter = NULL;
}
WriteCommand::WriteCommand(ConsumerInterface * consumer,
		vector<string> & primaryKeys,
		const CoreInfo_t * coreInfo):ProducerInterface(consumer){
	ASSERT(coreInfo != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	ASSERT(this->getConsumer()->getTransaction()->getSession() != NULL);
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->coreInfo = coreInfo;
	this->clusterReadview = ((ReadviewTransaction *)(this->getConsumer()->getTransaction().get()))->getReadview();
	this->response = (JsonRecordOperationResponse*) (this->getConsumer()->getTransaction()->getSession()->response);
	initActionType(Delete_ClusterRecordOperation_Type);
	initRecords(primaryKeys);
	this->nodeWriter = NULL;
}

WriteCommand::WriteCommand(ConsumerInterface * consumer,
		map<string, vector<string> > & primaryKeyRoleIds,
		RecordAclCommandType commandType,
		const CoreInfo_t * coreInfo):ProducerInterface(consumer){
	ASSERT(coreInfo != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	ASSERT(this->getConsumer()->getTransaction()->getSession() != NULL);
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->coreInfo = coreInfo;
	this->clusterReadview = ((ReadviewTransaction *)(this->getConsumer()->getTransaction().get()))->getReadview();
	this->response = (JsonRecordOperationResponse*) (this->getConsumer()->getTransaction()->getSession()->response);
	ClusterRecordOperation_Type recOpType;
	switch (commandType) {
		case Acl_Record_Add:
			recOpType = AclRecordAdd_ClusterRecordOperation_Type;
			break;
		case Acl_Record_Append:
			recOpType = AclRecordAppend_ClusterRecordOperation_Type;
			break;
		case Acl_Record_Delete:
			recOpType = AclRecordDelete_ClusterRecordOperation_Type;
			break;
	}
	initActionType(recOpType);
	initRecords(primaryKeyRoleIds);
	this->nodeWriter = NULL;
}


WriteCommand::WriteCommand(ConsumerInterface * consumer,
		map<string, vector<string> > & primaryKeyRoleIds,
		AclActionType commandType,
		const CoreInfo_t * coreInfo):ProducerInterface(consumer){
	ASSERT(coreInfo != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	ASSERT(this->getConsumer()->getTransaction()->getSession() != NULL);
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->coreInfo = coreInfo;
	this->clusterReadview = ((ReadviewTransaction *)(this->getConsumer()->getTransaction().get()))->getReadview();
	this->response = (JsonRecordOperationResponse*) (this->getConsumer()->getTransaction()->getSession()->response);
	ClusterRecordOperation_Type recOpType;
	switch (commandType) {
		case ACL_REPLACE:
			recOpType = AclAttrReplace_ClusterRecordOperation_Type;
			break;
		case ACL_DELETE:
			recOpType = AclAttrDelete_ClusterRecordOperation_Type;
			break;
		case ACL_APPEND:
			recOpType = AclAttrAppend_ClusterRecordOperation_Type;
			break;
	}
	initActionType(recOpType);
	initRecords(primaryKeyRoleIds);
	this->nodeWriter = NULL;
}

/*
 *  write command for user feedback
 */
WriteCommand::WriteCommand(ConsumerInterface * consumer,
			map<string, vector<string> > & primaryKeyQueryIdsMap,
			const CoreInfo_t * coreInfo): ProducerInterface(consumer) {
	ASSERT(coreInfo != NULL);
	ASSERT(this->getConsumer() != NULL);
	ASSERT(this->getConsumer()->getTransaction() != NULL);
	ASSERT(this->getConsumer()->getTransaction()->getSession() != NULL);

	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->coreInfo = coreInfo;
	this->clusterReadview = ((ReadviewTransaction *)(this->getConsumer()->getTransaction().get()))->getReadview();
	this->response = (JsonRecordOperationResponse*) (this->getConsumer()->getTransaction()->getSession()->response);

	initActionType(Feedback_ClusterRecordOperation_Type);
	initRecords(primaryKeyQueryIdsMap);
	this->nodeWriter = NULL;
}

WriteCommand::~WriteCommand(){
	if(this->nodeWriter != NULL){
		delete nodeWriter;
	}
	for(map<ClusterPID, PartitionWriter * >::iterator pItr = finishedPartitionWriters.begin();
			pItr != finishedPartitionWriters.end(); ++pItr){
		delete pItr->second;
	}
}

// start the write operation
void WriteCommand::produce(){
	// is there anything to work with?
	if(records.empty()){
		finalize();
		return;
	}
	// 1. first check if core is cluster core or node core
	if(coreInfo->isDistributedCore()){
		// cluster core
		// partition the records and prepare the PartitionWriter objects
		// to be used.
		if(! partitionRecords()){
			// Something went wrong in the partitioning process, abort.
			finalize();
			return;
		}
		// check if we are done, return.
		// it is possible because all records may have some problem and nothing be left
		// for continuing ...
		// For example in a bulk record insertion, maybe the host partition of all given records
		// is not available and insert must be rejected for all records.
		if(tryFinalize()){
			finalize();
			return;
		}
		// start writers
		if(partitionWriters.empty()){
			// We should never reach here, but just in case ...
			ASSERT(false);
			finalize();
			return;
		}

		// iterate on partitions and start PartitionWriter objects
		for(map<ClusterPID, PartitionWriter * >::iterator pItr = partitionWriters.begin();
				pItr != partitionWriters.end(); ++pItr){
			pItr->second->produce();
		}
	}else{
		// node core
		// use PartitionWriter with ClusterPID() input to insert to node shards
		vector<RecordWriteOpHandle *> recordsVector;
		for(map<string, RecordWriteOpHandle *>::iterator recItr = records.begin();
				recItr != records.end(); ++recItr){
			if(! recItr->second->isWorkDone()){
				recordsVector.push_back(recItr->second);
			}else{
				// The record item must not be done at this point, we have just started.
				ASSERT(false);
			}
		}

		// find node targets
		const CorePartitionContianer * corePartContainer = clusterReadview->getPartitioner(coreInfo->getCoreId());
		if(corePartContainer == NULL){
			ASSERT(false);
			finalize();
			return;
		}
		CorePartitioner * partitioner = new CorePartitioner(corePartContainer);
		vector<NodeTargetShardInfo> targets;

		// for the case of node shards, we pass 0 for the hash-key (this value is not used in this case)
		partitioner->getAllWriteTargets(0,ShardManager::getCurrentNodeId(), targets);
		this->nodeWriter = new PartitionWriter(ClusterPID(), insertUpdateDelete, recordsVector, targets, this);

		// start the submodule to perform the write operation on the local shard of this node.
		this->nodeWriter->produce();
	}
}

// coming back from cluster shard write operations
void WriteCommand::consume(const ClusterPID & pid){

	// if a PartitionWriter object gets back to this module,
	// we must have that PartitionWriter object in partitionWriters
	if(partitionWriters.find(pid) == partitionWriters.end()){
		ASSERT(false);
	}else{
		// done, move it to the finished writers map.
		finishedPartitionWriters[pid] = partitionWriters[pid];
		partitionWriters.erase(pid);
	}
	// did all parition writers finish?
	if(partitionWriters.empty()){
		finalize();
		return;
	}

	// Check all records to see if there is any record not ready to be finalized
	tryFinalize();
}

// coming back from node-shards write operation
void WriteCommand::consume(){
	finalize();
}

/*
 * Group the given record items based on their host partition and prepare the
 * partition writer objects.
 */
bool WriteCommand::partitionRecords(){

	// Access the container of all partitions of the data source (core) on which this write operation must be performed.
	const CorePartitionContianer * corePartContainer = clusterReadview->getPartitioner(coreInfo->getCoreId());
	if(corePartContainer == NULL){
		ASSERT(false);
		return false;
	}

	// Get the partitioner object for the data source into which the given records should be written.
	CorePartitioner * partitioner = new CorePartitioner(corePartContainer);

	// For each partition, prepare a list of records that fall into that partition
	// as well as the list of targets that cover that partition (i.e. cover all replicas of that patition)
	map<ClusterPID, vector<RecordWriteOpHandle *> > partitionedRecords;
	map<ClusterPID, vector<NodeTargetShardInfo> > partitionTargets;

	// iterate on records and assign it to one parition group
	for(map<string, RecordWriteOpHandle *>::iterator recItr = records.begin(); recItr != records.end(); ++recItr){

		if(recItr->second == NULL){
			ASSERT(false);
			delete partitioner;
			return false;
		}

		string primaryKey = recItr->first;
		RecordWriteOpHandle * recordHandle = recItr->second;

		vector<NodeTargetShardInfo> targets;
		// prepare the list of targets that involved in a write operation (basically all replicas)
		partitioner->getAllWriteTargets(partitioner->hashDJB2(primaryKey.c_str()),
				ShardManager::getCurrentNodeId(), targets);

		if(targets.empty()){
			// No target is available for this primary key. Finalize this record right here.
        	recordHandle->addMessage(NULL, HTTP_Json_No_Data_Shard_Available_For_Write);
        	recordHandle->finalize(false);
        	continue;
		}

		// decide the partition into which this record falls
		ClusterPID pid;
		if(! partitioner->getClusterPartitionId(primaryKey, pid)){
			ASSERT(false);
			delete partitioner;
			return false;
		}

		// If a replica of this partition is for example being copied, the partition is not
		// available for write operations and the following if statement is for checking that.
		if(corePartContainer->getClusterPartition(pid.partitionId)->isPartitionLocked()){
			// partition is locked, so we cannot write into it.
        	recordHandle->addMessage(NULL, HTTP_Json_Partition_Is_Locked);
        	recordHandle->finalize(false);
        	continue;
		}

		// remember the target
		if(partitionTargets.find(pid) == partitionTargets.end()){
			partitionTargets[pid] = targets;
		}
		if(partitionedRecords.find(pid) == partitionedRecords.end()){
			partitionedRecords[pid] = vector<RecordWriteOpHandle *>();
		}
		// add record to the list of records of partition pid.
		partitionedRecords[pid].push_back(recItr->second);
	}


	// move on all partitions and create the writers
	for(map<ClusterPID, vector<RecordWriteOpHandle *> >::iterator pItr = partitionedRecords.begin();
			pItr != partitionedRecords.end(); ++pItr){
		const ClusterPID & pid = pItr->first;
		vector<RecordWriteOpHandle *> & pidRecords = pItr->second;
		PartitionWriter * writer = new PartitionWriter(pid, insertUpdateDelete, pidRecords, partitionTargets[pid], this);
		partitionWriters[pid] = writer;
	}
	delete partitioner;
	return true;
}


bool WriteCommand::tryFinalize(){
	for(map<string, RecordWriteOpHandle *>::iterator recItr = records.begin();
			recItr != records.end(); ++recItr){
		// if there is still any record not ready to be finalized, return false;
		if(! recItr->second->isWorkDone()){
			return false;
		}
	}

	// all records are done, call finalize.
	finalize();
	return true;
}

void WriteCommand::finalize(){
	// If this method is called while some records are still not done,
	// we must first finalize those records by just using a failure message for them.
	for(map<string, RecordWriteOpHandle *>::iterator recItr = records.begin();
			recItr != records.end(); ++recItr){
		if(! recItr->second->isWorkDone()){
			recItr->second->finalize(false);
			recItr->second->addMessage(NULL, HTTP_Json_General_Error);
		}
	}
	map<string, bool> results;
	map<string, map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > > messageCodes;
	for(map<string, RecordWriteOpHandle *>::iterator recItr = records.begin();
			recItr != records.end(); ++recItr){
		results[recItr->first] = recItr->second->isSuccessful();
		messageCodes[recItr->first] = recItr->second->getMessageCodes();
	}
	// return the outcome to the called of this module.
	this->getConsumer()->consume(results, messageCodes);
}

SP(Transaction) WriteCommand::getTransaction(){
	return this->getConsumer()->getTransaction();
}

string WriteCommand::getName() const{
	return "write-command";
}


void WriteCommand::initActionType(ClusterRecordOperation_Type insertUpdateDelete){
	this->insertUpdateDelete = insertUpdateDelete;
	switch (insertUpdateDelete) {
		case Insert_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_insert);
			break;
		case Update_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_update);
			break;
		case Delete_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_delete);
			break;
		case AclRecordAdd_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_acl_record_add);
			break;
		case AclRecordAppend_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_acl_record_append);
			break;
		case AclRecordDelete_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_acl_record_delete);
			break;
		case AclAttrReplace_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_acl_attribute_replace);
			break;
		case AclAttrAppend_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_acl_attribute_append);
			break;
		case AclAttrDelete_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_acl_attribute_delete);
			break;
		case Feedback_ClusterRecordOperation_Type:
			actionNameStr = string(c_action_user_feedback);
			break;
	}
}

void WriteCommand::initRecords(vector<Record *> & recordsVector){
	for(unsigned i = 0 ; i < recordsVector.size() ; ++i){
		if(records.find(recordsVector.at(i)->getPrimaryKey()) == records.end()){
			records[recordsVector.at(i)->getPrimaryKey()] = new RecordWriteOpHandle(recordsVector.at(i));
		}else{
			// we must give an error because you cannot have to similar primary keys in the same batch
        	Json::Value recordJsonResponse = JsonRecordOperationResponse::getRecordJsonResponse(
        			recordsVector.at(i)->getPrimaryKey(), actionNameStr.c_str(), true , coreInfo->getName());
        	JsonRecordOperationResponse::addRecordError(recordJsonResponse, HTTP_JSON_Custom_Error,
        			JsonResponseHandler::getJsonSingleMessageStr(HTTP_Json_DUP_PRIMARY_KEY) );
        	response->addRecordShardResponse(recordJsonResponse);
		}
	}
}

void WriteCommand::initRecords(vector<string> & recordsVector){
	for(unsigned i = 0 ; i < recordsVector.size() ; ++i){
		if(records.find(recordsVector.at(i)) == records.end()){
			records[recordsVector.at(i)] = new RecordWriteOpHandle(recordsVector.at(i));
		}else{
			// we must give an error because you cannot have to similar primary keys in the same batch
        	Json::Value recordJsonResponse = JsonRecordOperationResponse::getRecordJsonResponse(
        			recordsVector.at(i), actionNameStr.c_str(), true , coreInfo->getName());
        	JsonRecordOperationResponse::addRecordError(recordJsonResponse, HTTP_JSON_Custom_Error,
        			JsonResponseHandler::getJsonSingleMessageStr(HTTP_Json_DUP_PRIMARY_KEY) );
        	response->addRecordShardResponse(recordJsonResponse);
		}
	}
}


void WriteCommand::initRecords(map<string, vector<string> > & primaryKeyRoleIds){
	for(map<string, vector<string> >::iterator pkItr = primaryKeyRoleIds.begin();
			pkItr != primaryKeyRoleIds.end(); ++pkItr){
		if(records.find(pkItr->first) == records.end()){
			records[pkItr->first] = new RecordWriteOpHandle(pkItr->first, pkItr->second);
		}else{
			// we must give an error because you cannot have to similar primary keys in the same batch
        	Json::Value recordJsonResponse = JsonRecordOperationResponse::getRecordJsonResponse(
        			pkItr->first, actionNameStr.c_str(), true , coreInfo->getName());
        	JsonRecordOperationResponse::addRecordError(recordJsonResponse, HTTP_JSON_Custom_Error,
        			JsonResponseHandler::getJsonSingleMessageStr(HTTP_Json_DUP_PRIMARY_KEY) );
        	response->addRecordShardResponse(recordJsonResponse);
		}
	}
}


}
}
