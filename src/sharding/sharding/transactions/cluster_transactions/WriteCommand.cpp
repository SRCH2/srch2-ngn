#include "WriteCommand.h"


#include "../../state_machine/StateMachine.h"
#include "../../metadata_manager/Cluster_Writeview.h"
#include "../../metadata_manager/Cluster.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

PartitionWriter::PartitionWriter(const ClusterPID & pid, // we will use ClusterPID() for the case of node shards
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
	 * 1. if cluster shards : call lock();
	 * 2. if node shards : call performWrite()
	 */
	if(targets.empty()){
		// we should not do anything.
		return;
	}
	if(records.empty()){
		// just call consume
		return;
	}
	if(this->clusterOrNodeFlag){
		// case of cluster shard core write request
		if(pid == ClusterPID()){
			ASSERT(false);
			return;
		}
		lock();
	}else{
		// case of node shard core write request
		if(pid != ClusterPID()){
			ASSERT(false);
			return;
		}
		performWrite();
	}
}

void PartitionWriter::lock(){
	// 1. initializing lock related members
	// sort the records based on primary key
	std::sort(records.begin(), records.end(), RecordComparator());
	// prepare the list of primary keys
	vector<string> primaryKeys;
	for(unsigned i = 0 ; i < records.size(); ++i){
		primaryKeys.push_back(records.at(i)->getPrimaryKey());
	}
	locker = new AtomicLock(primaryKeys, currentOpId, pid, this);
	// 2.
	// b. go over all partitions and start atomic lock and set the current operation
	currentStep = StepLock;
	locker->produce(); // this line will definitely lead to leaving this thread.

	// release the shard manager lock (automatic)
}

/*
 * Consume used for sharded case, PID is passed through the argument which tells us for which
 *  partition this callback is.
 */
void PartitionWriter::consume(bool granted, const ClusterPID & pid){
	ASSERT(pid == this->pid); // TODO : pid argument must be removed later ...
	switch (currentStep) {
		case StepLock:
			if(granted){
				ask2PC();
			}else{
				finalize(false, HTTP_Json_Partition_Is_Locked);
			}
			break;
		case StepRelease:
			if(granted){
				// Done
				finalize(false, HTTP_Json_General_Error);
			}else{
				ASSERT(false);
				finalize(false, HTTP_Json_General_Error);
			}
			break;
		default:
			ASSERT(false);
			finalize(false, HTTP_Json_General_Error);
			break;
	}
}


void PartitionWriter::ask2PC(){ // calls sendWriteCommand with 'false' argument indicating ask phase of 2PC
	if(! validateAndFixTargets()){
		finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
		return;
	}
	currentStep = StepAsk2PC;
	sendWriteCommand(WriteNotificationModeAsk2PC);
}

void PartitionWriter::processAsk2PCResponse(map<NodeId, SP(ShardingNotification) > & _replies){

	map<string, bool > conjuncted2PCStatusValue;
	for(map<NodeId, SP(ShardingNotification) >::iterator nodeItr = _replies.begin(); nodeItr != _replies.end(); ++nodeItr){
		SP(Write2PCNotification::ACK) ack = boost::dynamic_pointer_cast<Write2PCNotification::ACK>(nodeItr->second);
		map<string, vector<Write2PCNotification::ACK::ShardResult *> > & shardResults = ack->getResults();
		for(unsigned recIdx = 0; recIdx < records.size(); ++recIdx){
			RecordWriteOpHandle * recordHandle = records.at(recIdx);
			const string & primaryKey = recordHandle->getPrimaryKey();
			if(shardResults.find(primaryKey) == shardResults.end()){
				ASSERT(false);
				recordHandle->addMessage(NULL, HTTP_Json_General_Error);
				conjuncted2PCStatusValue[primaryKey] = false;
			}else{
				vector<Write2PCNotification::ACK::ShardResult *> & primaryKeyShardResults = shardResults[primaryKey];
				for(unsigned i = 0; i < primaryKeyShardResults.size(); ++i){
					Write2PCNotification::ACK::ShardResult * pkShardResultIth = primaryKeyShardResults.at(i);
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
		if(! pkItr->second){
			// this primary key could not pass the 2pc ask phase
			if(recordsMap.find(pkItr->first) == recordsMap.end()){
				ASSERT(false);
				continue;
			}
			recordsMap[pkItr->first]->finalize(false);
		}
	}
	performWrite();
}

void PartitionWriter::performWrite(){ // calls sendWriteCommand with 'true' argument indicating write perform phase
	if(! validateAndFixTargets()){
		finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
		return;
	}
	currentStep = StepWrite;
	sendWriteCommand(WriteNotificationModePerformWrite);
}

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

	// now release the records
	if(! validateAndFixTargets()){
		finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
		return;
	}

	vector<string> primaryKeys;
	for(unsigned i = 0 ; i < records.size(); ++i){
		primaryKeys.push_back(records.at(i)->getPrimaryKey());
	}
	releaser = new AtomicRelease(primaryKeys, currentOpId, pid, this);
	currentStep = StepRelease;
	releaser->produce();
}


bool PartitionWriter::shouldAbort(const NodeId & failedNode){
	unsigned nodeTargetInfoIndex = targets.size();
	for(nodeTargetInfoIndex = 0; nodeTargetInfoIndex < targets.size(); ++nodeTargetInfoIndex){
		if(targets.at(nodeTargetInfoIndex).getNodeId() == failedNode){
			break;
		}
	}
	if(nodeTargetInfoIndex < targets.size()){
		targets.erase(targets.begin() + nodeTargetInfoIndex);
	}
	if(targets.empty()){
		if(currentStep == StepAsk2PC){
			// abort : there is no target anymore to continue
			finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
			return true;
		}else if(currentStep == StepWrite){
			// abort : there is no target anymore to continue
			finalize(false, HTTP_Json_No_Data_Shard_Available_For_Write);
			return true;
		}else{
			ASSERT(false);
			return true;
		}
	}
	return false;
}

/*
 * When all reply notifications resulted from call to sendWriteCommnad
 * reach to this node and ConcurrentNotifOperation
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

void WriteCommand::produce(){
	if(records.empty()){
		return;
	}
	// 1. first check if core is cluster core or node core
	if(coreInfo->isDistributedCore()){
		// cluster core
		// partition the records and prepare the PartitionWriters
		if(! partitionRecords()){
			return;
		}
		// check if we are done, return.
		// it is possible because all records may have some problem and nothing be appropriate
		// for continuing ...
		if(tryFinalize()){
			return;
		}
		// start writers
		if(partitionWriters.empty()){
			ASSERT(false);
			return;
		}
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
				ASSERT(false);
			}
		}

		// find node targets
		const CorePartitionContianer * corePartContainer = clusterReadview->getPartitioner(coreInfo->getCoreId());
		if(corePartContainer == NULL){
			ASSERT(false);
			return;
		}
		CorePartitioner * partitioner = new CorePartitioner(corePartContainer);
		vector<NodeTargetShardInfo> targets;
		partitioner->getAllWriteTargets(0,ShardManager::getCurrentNodeId(), targets);
		this->nodeWriter = new PartitionWriter(ClusterPID(), insertUpdateDelete, recordsVector, targets, this);
		this->nodeWriter->produce();
	}
}

// coming back from cluster shard write operations
void WriteCommand::consume(const ClusterPID & pid){
	if(partitionWriters.find(pid) == partitionWriters.end()){
		ASSERT(false);
	}
	delete partitionWriters[pid];
	partitionWriters.erase(pid);
	if(partitionWriters.empty()){
		finalize();
		return;
	}
	tryFinalize();
}

// coming back from node-shards write operation
void WriteCommand::consume(){
	finalize();
}

bool WriteCommand::partitionRecords(){
	const CorePartitionContianer * corePartContainer = clusterReadview->getPartitioner(coreInfo->getCoreId());
	if(corePartContainer == NULL){
		ASSERT(false);
		return false;
	}

	CorePartitioner * partitioner = new CorePartitioner(corePartContainer);


	map<ClusterPID, vector<RecordWriteOpHandle *> > partitionedRecords;
	map<ClusterPID, vector<NodeTargetShardInfo> > partitionTargets;
	for(map<string, RecordWriteOpHandle *>::iterator recItr = records.begin(); recItr != records.end(); ++recItr){

		if(recItr->second == NULL){
			ASSERT(false);
			delete partitioner;
			return false;
		}

		string primaryKey = recItr->first;
		RecordWriteOpHandle * recordHandle = recItr->second;

		vector<NodeTargetShardInfo> targets;
		partitioner->getAllWriteTargets(partitioner->hashDJB2(primaryKey.c_str()),
				ShardManager::getCurrentNodeId(), targets);

		if(targets.empty()){
			// No target is available for this primary key. Finalize this record right here.
        	recordHandle->addMessage(NULL, HTTP_Json_No_Data_Shard_Available_For_Write);
        	recordHandle->finalize(false);
        	continue;
		}

		ClusterPID pid;
		if(! partitioner->getClusterPartitionId(primaryKey, pid)){
			ASSERT(false);
			delete partitioner;
			return false;
		}

		if(corePartContainer->getClusterPartition(pid.partitionId)->isPartitionLocked()){
			// partition is locked, so we cannot write into it.
        	recordHandle->addMessage(NULL, HTTP_Json_Partition_Is_Locked);
        	recordHandle->finalize(false);
        	continue;
		}

		if(partitionTargets.find(pid) == partitionTargets.end()){
			partitionTargets[pid] = targets;
		}
		if(partitionedRecords.find(pid) == partitionedRecords.end()){
			partitionedRecords[pid] = vector<RecordWriteOpHandle *>();
		}
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
		if(! recItr->second->isWorkDone()){
			return false;
		}
	}
	finalize();
	return true;
}

void WriteCommand::finalize(){
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
