// Author: Jamshid
#ifndef __SHARDING_SHARDING_TRANSACTIONS_CLUSTER_TRANSACTIONS_WRITE_COMMAND_H__
#define __SHARDING_SHARDING_TRANSACTIONS_CLUSTER_TRANSACTIONS_WRITE_COMMAND_H__

#include "../../state_machine/State.h"
#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../../notifications/Notification.h"
#include "../../notifications/Write2PCNotification.h"
#include "../../metadata_manager/Shard.h"
#include "../../state_machine/ConsumerProducer.h"
#include "../AtomicLock.h"
#include "../AtomicRelease.h"
#include "sharding/processor/Partitioner.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "include/instantsearch/Record.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * When a group of records are going to be manipulated (insert/update/delete), they are
 * grouped based on which partition they fall into and the operation is done separately
 * for each partition records group.
 * This class keeps track of the operation status for a partition
 *
 * Notes:
 * Implements ProducerInterface because it's used by WriteCommand module (write command is
 * its consumer)
 * Implements NodeIteratorListenerInterface because in the 2PC process, it has to ask all involved nodes
 * to confirm the possibility of this write operation
 */
class PartitionWriter : public ProducerInterface, public NodeIteratorListenerInterface{
public:

	/*
	 * Perform the 'insertUpdateDelete' record write operation for 'records' that fall into the
	 * data partition pid. The involved data buckets are 'targets'.
	 * Report the result of this operation to 'consumer' by giving a call to its consume method.
	 */
	PartitionWriter(const ClusterPID & pid, // we will use ClusterPID() for the case of node shards
			const ClusterRecordOperation_Type & insertUpdateDelete,
			vector<RecordWriteOpHandle *> & records, const vector<NodeTargetShardInfo> & targets,
			ConsumerInterface * consumer);

	SP(Transaction) getTransaction();

	// Start performing the record operation
	// Note: We come out of this method, but the rest of task may continue by another thread.
	void produce();

	// Different steps of record operation to be performed by this PartitionWriter object
	enum WriteOperationStep{
		StepPreStart,
		StepLock,
		StepAsk2PC,
		StepWrite,
		StepRelease
	};

	/*
	 * Consume used for sharded case, PID is passed through the argument which tells us for which
	 *  partition this callback is.
	 */
	void consume(bool granted, const ClusterPID & pid);

	/*
	 * We use an instance of ConcurrentNotifOperation class to perform the process of
	 * distributing a command to a group of nodes and aggregating the responses. This
	 * ConcurrentNotifOperation object returns the replies to the 'end' method of this
	 * class (which is its user)
	 */
	void end(map<NodeId, SP(ShardingNotification) > & replies);

	string getName() const ;
private:

	/*
	 * Acquires the needed locks before performing this record write operation
	 */
	void lock();

	/*
	 * Perform the Two Phase Commit protocol 'ask phase'.
	 * Explanation on how we do 2PC:
	 * TODO
	 */
	void ask2PC();

	/*
	 * Receives all node replies to the 'ask' phase and decides on what to do for the next step
	 */
	void processAsk2PCResponse(map<NodeId, SP(ShardingNotification) > & _replies);

	/*
	 * Performs the 'write' phase of 2PC. Basically this method sends the
	 * 'write' command to all involed nodes.
	 */
	void performWrite();

	/*
	 * Receives the write phase replies of all nodes and processes these replies.
	 */
	void processWriteResponse(map<NodeId, SP(ShardingNotification) > & _replies);

	/*
	 * When this method is called it means write operation is finished and ready to be finalized.
	 * finalize prepares the result of our task and reports the result back to the caller of this module
	 * which is typically WriteCommandHttp
	 */
	void finalize(bool defaultStatusValue, const JsonMessageCode & defaultMessageCode);

	/*
	 * Implements the logic of sending the a command to the nodes that
	 * host target data bulks. This command is 'just asking if it's possible' in the first phase
	 * of 2PC and 'actually asking to do the job' in the second phase of 2PC.
	 * Which type of command to send, is determined based on the current step.
	 */
	void sendWriteCommand(WriteNotificationMode mode);

	/*
	 * Prepare the list of 'targets' of this write operation. For example, if
	 * this is a record insertion, 'targets' contains all data shard replicas of
	 * the partition assigned to this PartitionWriter object.
	 *
	 */
	void initTargets(const vector<NodeTargetShardInfo> & targets);

	/*
	 * Checks to make sure the host node of a target is still alive; and if it's not,
	 * it removes that target from target list.
	 */
	bool validateAndFixTargets();

	/*
	 * The cluster metadata readview version which is used throughout this
	 * write operation.
	 */
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;

	/*
	 * The partition corresponding to the records given to this object
	 * If pid is (-1,-1,-1) it means PartitionWriter is going to be used for
	 * performing a write operation on a local node shard.
	 */
	const ClusterPID pid;

	/*
	 * There are many operations that require the same sequence of distributed
	 * processing steps. We call all of them 'write command' and that's why this module
	 * is called WriteCommand. Member 'insertUpdateDelete' keeps the kind of operation
	 * to be performed by this object.
	 */
	const ClusterRecordOperation_Type insertUpdateDelete;

	/*
	 * Keeps the ID of the current node. It is for easier access because currentNodeId can always
	 * be accessed by calling ShardManager::getCurrentNodeId().
	 */
	NodeOperationId currentOpId;

	// true is cluster shard mode, false is node shard mode
	bool clusterOrNodeFlag;

	// The list of targets of this partition write operation
	vector<NodeTargetShardInfo> targets;

	/************** Record containers *******************/
	/*
	 * reads the 'records' vector which contains the input records for this
	 * write operation and initializes the record containers: 'records' and 'recordsMap'
	 */
	void initRecords(vector<RecordWriteOpHandle *> & records);
	vector<RecordWriteOpHandle *> records;
	/*
	* NOTE : this structure contains exactly the same set of
	*        data as records vector. This map is only kept to
	*        make access by primaryKey easier.
	*/
	map<string, RecordWriteOpHandle *> recordsMap;
	/****************************************************/

	/*
	 * AtomicLock module used for acquiring necessary locks for this operation
	 */
	AtomicLock * locker;

	/*
	 * AtomicRelease module used for releasing all locks acquired by AtomicL
	 */
	AtomicRelease * releaser;

	/*
	 * Keeps the on-going step of the write operation.
	 */
	WriteOperationStep currentStep;
};


/*
 * WriteCommand class is the main module which performs write operations.
 */
class WriteCommand : public ProducerInterface, public NodeIteratorListenerInterface{
public:
	/*
	 * Used for inserting a list of records (arg 'records') into the data source
	 * 'coreInfo'. Argument 'insertUpdateDelete' determines the type of operation to be done
	 */
	WriteCommand(ConsumerInterface * consumer,
			vector<srch2is::Record *> records,
			ClusterRecordOperation_Type insertUpdateDelete,
			const CoreInfo_t * coreInfo);
	/*
	 * Used to pass a list of primary keys ('primaryKeys') to be deleted from the
	 * data of data source 'coreInfo'
	 */
	WriteCommand(ConsumerInterface * consumer,
			vector<string> & primaryKeys,
			const CoreInfo_t * coreInfo);
	/*
	 * Used for Record ACL Add, Append, and Delete operations.
	 * Argument 'primaryKeyRoleIds' is map from 'primary key' to list of role IDs in that request
	 */
	WriteCommand(ConsumerInterface * consumer,
			map<string, vector<string> > & primaryKeyRoleIds,
			RecordAclCommandType commandType,
			const CoreInfo_t * coreInfo);
	/*
	 * Used for Attribute ACL Replace, Delete, and Append operations.
	 */
	WriteCommand(ConsumerInterface * consumer,
			map<string, vector<string> > & primaryKeyRoleIds,
			AclActionType commandType,
			const CoreInfo_t * coreInfo);

	/*
	 * Used for user-feedback feedback operation.
	 */
	WriteCommand(ConsumerInterface * consumer,
				map<string, vector<string> > & primaryKeyQueryIdsMap,
				const CoreInfo_t * coreInfo);
	~WriteCommand();

	// Starts the write operation
	void produce();

	// coming back from cluster shard write operations
	void consume(const ClusterPID & pid);

	// coming back from node-shards write operation
	void consume();

	/*
	 * Iterates on the given records and groups them based on which partition they must join to.
	 * Returns false if anything goes wrong.
	 */
	bool partitionRecords();

	/*
	 * Checks to see if all partitionWriter objects have returned to this (WriteCommand),
	 * if this is the case, it proceeds with actually finalizing the task.
	 */
	bool tryFinalize();

	/*
	 * Finalizes the given task and reports the results back to the called of this module.
	 */
	void finalize();

	SP(Transaction) getTransaction();

	string getName() const;
private:
	// The cluster metadata readview version used throughout this write operation
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;

	// the Json response handler object which is directly used for storing the results.
	JsonRecordOperationResponse* response;

	// The node ID of the current node.
	NodeOperationId currentOpId;

	// The data source corresponding to this write operation
	const CoreInfo_t * coreInfo;

	// The kind of operation to be performed by this WriterCommand object.
	ClusterRecordOperation_Type insertUpdateDelete;

	// The string representation of the gieven task
	string actionNameStr;

	// Initializes actionNameStr based on the operation type stored in the enum variable
	void initActionType(ClusterRecordOperation_Type insertUpdateDelete);

	/*
	 * This map holds the references to PartitionWriter objects which perform the
	 * part of write operation which is only related to a partition. the key is the
	 * partition id, the value is the pointer to the corresponding PartitionWriter object.
	 */
	map<ClusterPID, PartitionWriter * > partitionWriters;

	/*
	 * Once a PartitionWriter object finish its task, it is moved to the following map. When
	 * the paritionWriters map becomes empty, this write operation can finalize.
	 */
	map<ClusterPID, PartitionWriter * > finishedPartitionWriters;

	/*
	 * Object used for insertion when the data source is not distributed on the cluster and
	 * the given records must be just inserted into the current node (for the example
	 * of record insertion usage).
	 */
	PartitionWriter * nodeWriter;

	/*
	 * A map from unique identifier of the given records to the RecordWriteOpHandle
	 * object corresponding to them.
	 * RecordWriteOpHandle is like a 'tuple' class which maintains the information related to a single
	 * item to be written to the data.
	 */
	map<string, RecordWriteOpHandle *> records;

	/*
	 * Helper methods for initializing this class based on the given items given
	 * within the write operation.
	 */
	void initRecords(vector<Record *> & recordsVector);
	void initRecords(vector<string> & recordsVector);
	void initRecords(map<string, vector<string> > & primaryKeyRoleIds);
};

}
}
#endif // __SHARDING_SHARDING_TRANSACTIONS_CLUSTER_TRANSACTIONS_WRITE_COMMAND_H__
