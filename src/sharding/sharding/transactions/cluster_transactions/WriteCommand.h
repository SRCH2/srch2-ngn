#ifndef __SHARDING_SHARDING_WRITE_COMMAND_H__
#define __SHARDING_SHARDING_WRITE_COMMAND_H__

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
 * contains the state of write operation per partition
 */
class PartitionWriter : public ProducerInterface, public NodeIteratorListenerInterface{
public:
	PartitionWriter(const ClusterPID & pid, // we will use ClusterPID() for the case of node shards
			const ClusterRecordOperation_Type & insertUpdateDelete,
			vector<RecordWriteOpHandle *> & records, const vector<NodeTargetShardInfo> & targets,
			ConsumerInterface * consumer);

	SP(Transaction) getTransaction();

	void produce();
private:
	enum WriteOperationStep{
		StepPreStart,
		StepLock,
		StepAsk2PC,
		StepWrite,
		StepRelease
	};

	void lock();

	/*
	 * Consume used for sharded case, PID is passed through the argument which tells us for which
	 *  partition this callback is.
	 */
	void consume(bool granted, const ClusterPID & pid);


	void ask2PC();

	void processAsk2PCResponse(map<NodeId, SP(ShardingNotification) > & _replies);

	void performWrite();

	void processWriteResponse(map<NodeId, SP(ShardingNotification) > & _replies);

	/*
	 * When all reply notifications resulted from call to sendWriteCommnad
	 * reach to this node and ConcurrentNotifOperation
	 */
	void end(map<NodeId, SP(ShardingNotification) > & replies);

	void finalize(bool defaultStatusValue, const JsonMessageCode & defaultMessageCode);

	/*
	 * We can write something similar to state machine
	 * node iterator operators that uses different schemes to call producer and
	 * consumer classes: like calling some sub-transactions in parallel or serial.
	 */


	string getName() const ;
private:


	void sendWriteCommand(WriteNotificationMode mode);

	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;

	const ClusterPID pid;
	const ClusterRecordOperation_Type insertUpdateDelete;
	NodeOperationId currentOpId;

	void initTargets(const vector<NodeTargetShardInfo> & targets);
	bool validateAndFixTargets();
	bool clusterOrNodeFlag; // true is cluster shard mode, false is node shard mode
	vector<NodeTargetShardInfo> targets;

	/************** Record containers *******************/
	void initRecords(vector<RecordWriteOpHandle *> & records);
	vector<RecordWriteOpHandle *> records;
	/*
	* NOTE : this structure contains exactly the same set of
	*        data as records vector. This map is only kept to
	*        make the access through primaryKey easier.
	*/
	map<string, RecordWriteOpHandle *> recordsMap;
	/****************************************************/


	AtomicLock * locker;
	AtomicRelease * releaser;
	WriteOperationStep currentStep;
};

// 	ExternalInsertUpdateCommand::insert(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
//	evhttp_request *req, unsigned coreId);
class WriteCommand : public ProducerInterface, public NodeIteratorListenerInterface{
public:
	WriteCommand(ConsumerInterface * consumer,
			vector<srch2is::Record *> records,
			ClusterRecordOperation_Type insertUpdateDelete,
			const CoreInfo_t * coreInfo);
	WriteCommand(ConsumerInterface * consumer,
			vector<string> & primaryKeys,
			const CoreInfo_t * coreInfo);
	WriteCommand(ConsumerInterface * consumer,
			map<string, vector<string> > & primaryKeyRoleIds,
			RecordAclCommandType commandType,
			const CoreInfo_t * coreInfo);
	WriteCommand(ConsumerInterface * consumer,
			map<string, vector<string> > & primaryKeyRoleIds,
			AclActionType commandType,
			const CoreInfo_t * coreInfo);
	WriteCommand(ConsumerInterface * consumer,
				map<string, vector<string> > & primaryKeyQueryIdsMap,
				const CoreInfo_t * coreInfo);
	~WriteCommand();
	void produce();



	// coming back from cluster shard write operations
	void consume(const ClusterPID & pid);

	// coming back from node-shards write operation
	void consume();

	// returns false if we cannot continue with this
	// set of records and otherwise return true
	// if there is a sharding scheme for this coreId, we
	// partition records in partitionedRecords
	// else : doWriteCommandNonSharded() is called
	bool partitionRecords();

	bool tryFinalize();

	void finalize();

	SP(Transaction) getTransaction();

	string getName() const;
private:
	// just for easier access
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	JsonRecordOperationResponse* response;

	NodeOperationId currentOpId;

	const CoreInfo_t * coreInfo;
	ClusterRecordOperation_Type insertUpdateDelete; // INSERT/UPDATE
	string actionNameStr;

	void initActionType(ClusterRecordOperation_Type insertUpdateDelete);

	map<ClusterPID, PartitionWriter * > partitionWriters;
	map<ClusterPID, PartitionWriter * > finishedPartitionWriters;
	PartitionWriter * nodeWriter;
	map<string, RecordWriteOpHandle *> records;

	void initRecords(vector<Record *> & recordsVector);

	void initRecords(vector<string> & recordsVector);

	void initRecords(map<string, vector<string> > & primaryKeyRoleIds);
};

}
}
#endif // __SHARDING_SHARDING_WRITE_COMMAND_H__
