#include "ShardCopyOperation.h"

#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../../configuration/ShardingConstants.h"
#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "server/Srch2Server.h"
#include "sharding/migration/MigrationManager.h"
#include "../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../state_machine/StateMachine.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardCopyOperation::ShardCopyOperation(const ClusterShardId & unassignedShard,
		NodeId srcNodeId, const ClusterShardId & shardToReplicate,
		ConsumerInterface * consumer):ProducerInterface(consumer),
		unassignedShardId(unassignedShard),replicaShardId(shardToReplicate),srcNodeId(srcNodeId){
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId());
	this->locker = NULL;
	this->releaser = NULL;
	this->committer = NULL;
	this->finalizedFlag = false;
	this->successFlag = true;
	this->currentAction = "";
}

ShardCopyOperation::~ShardCopyOperation(){
	if(this->locker != NULL){
		delete this->locker;
	}
	if(this->releaser != NULL){
		delete this->releaser;
	}
	if(this->committer != NULL){
		delete this->committer;
	}
}

Transaction * ShardCopyOperation::getTransaction() {
	return this->getConsumer()->getTransaction();
}

void ShardCopyOperation::produce(){
    Logger::debug("STEP : ShardCopyOperation starting ... ");
	lock();
}

void ShardCopyOperation::lock(){ // ** start **
	this->locker = new AtomicLock(replicaShardId, unassignedShardId, currentOpId, this);
	// locker calls all methods of LockResultCallbackInterface from us
	this->releaser = new AtomicRelease(replicaShardId, unassignedShardId, currentOpId, this);
	// releaser calls all methods of BooleanCallbackInterface from us
	this->currentAction = "lock";
    Logger::debug("DETAILS : ShardCopyOperation going to lock %s and %s for operation %s." ,
            replicaShardId.toString().c_str(), unassignedShardId.toString().c_str(), currentOpId.toString().c_str());
	this->locker->produce();
}
// for lock
void ShardCopyOperation::consume(bool granted){
	if(currentAction.compare("lock") == 0){
		if(granted){
			transfer();
		}else{
			this->successFlag = false;
			release();
		}
	}else if(currentAction.compare("release") == 0){
		if(! granted){
			this->successFlag = false;
		}
		finalize();
	}else if(currentAction.compare("commit") == 0){
		if(! granted){
			this->successFlag = false;
		}
		release();
	}
}
// ** if (granted)
void ShardCopyOperation::transfer(){ // : requires receiving a call to our callback registered in state-machine to get MM messages
	// transfer data by ordering MM
    Logger::debug("STEP : ShardCopyOperation going start transferring the shard ..." );
	// 1. register this transaction in shard manager to receive MM notification
	ShardManager::getShardManager()->registerMMSessionListener(currentOpId.operationId, this);
	// 2. send copyToMe notification to the srcNode to start transferring the data
	this->copyToMeNotif = SP(CopyToMeNotification)(new CopyToMeNotification(replicaShardId, unassignedShardId));

	// NOTE : this is deallocated by the state machine
	ConcurrentNotifOperation * copyer = new ConcurrentNotifOperation(copyToMeNotif, NULLType, srcNodeId , this, false);
	copyer->setOperationId(currentOpId.operationId);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(copyer);
}

void ShardCopyOperation::end(map<NodeId, ShardingNotification * > & replies){
	if(replies.size() < 1){
		this->successFlag = false;
		release();
	}
}
// if returns true, operation must stop and return null to state_machine
bool ShardCopyOperation::shouldAbort(const NodeId & failedNode){
	if(failedNode == srcNodeId){
		this->successFlag = false;
		release();
		return true;
	}
	return false;
}

// for transfer
void ShardCopyOperation::consume(const ShardMigrationStatus & status){
	// failed or succeed?
	if(status.status == MM_STATUS_FAILURE){
	    Logger::debug("DETAILS : ShardCopyOperation : transfer failed." );
		this->successFlag = false;
		release();
	}else if(status.status == MM_STATUS_SUCCESS){
	    Logger::debug("DETAILS : ShardCopyOperation : transfer was successful" );
		Cluster_Writeview * writeview = ShardManager::getWriteview();

		string indexDirectory = ShardManager::getShardManager()->getConfigManager()->getShardDir(writeview->clusterName,
				writeview->cores[unassignedShardId.coreId]->getName(), &unassignedShardId);
		if(indexDirectory.compare("") == 0){
			indexDirectory = ShardManager::getShardManager()->getConfigManager()->createShardDir(writeview->clusterName,
					writeview->cores[unassignedShardId.coreId]->getName(), &unassignedShardId);
		}

		physicalShard = LocalPhysicalShard(status.shard, indexDirectory, "");
		commit();
	}
}
void ShardCopyOperation::commit(){
    Logger::debug("STEP : ShardCopyOperation : going to commit shard assign change for shard %s" , unassignedShardId.toString().c_str());
	// start metadata commit
	// prepare the shard change
	ShardAssignChange * shardAssignChange = new ShardAssignChange(unassignedShardId, ShardManager::getCurrentNodeId(), 0);
	shardAssignChange->setPhysicalShard(physicalShard);
	this->committer = new AtomicMetadataCommit(shardAssignChange,  vector<NodeId>(), this, true); // last true arg : skip lock
	currentAction = "commit";
	this->committer->produce();
}
// ** end if
void ShardCopyOperation::release(){
	// release the locks
	currentAction = "release";
    Logger::debug("STEP : ShardCopyOperation : releasing locks ...");
	this->releaser->produce();
}

void ShardCopyOperation::finalize(){ // ** return **
	this->finalizedFlag = true;
	Logger::debug("STEP : ShardCopyOperation : finalizing ...");
	this->getConsumer()->consume(this->successFlag);
}

}
}
