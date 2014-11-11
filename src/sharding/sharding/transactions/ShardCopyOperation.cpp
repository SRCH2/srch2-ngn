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
	ASSERT(this->getTransaction());
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->locker = NULL;
	this->releaser = NULL;
	this->committer = NULL;
	this->finalizedFlag = false;
	this->successFlag = true;
	this->currentAction = "";
    this->transferAckReceived = false;
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

SP(Transaction) ShardCopyOperation::getTransaction() {
	if(this->getConsumer() == NULL){
		return SP(Transaction)();
	}
	return this->getConsumer()->getTransaction();
}

void ShardCopyOperation::produce(){
	Logger::sharding(Logger::Step, "ShardCopy(opid=%s, cp {%s in %d} to %s )| Starting ...", currentOpId.toString().c_str(),
			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());
	lock();
}

void ShardCopyOperation::lock(){ // ** start **
	this->locker = new AtomicLock(replicaShardId, unassignedShardId, currentOpId, this);
	// locker calls all methods of LockResultCallbackInterface from us
	this->releaser = new AtomicRelease(replicaShardId, unassignedShardId, currentOpId, this);
	// releaser calls all methods of BooleanCallbackInterface from us
	this->currentAction = "lock";
	Logger::sharding(Logger::Detail, "ShardCopy(opid=%s, cp {%s in %d} to %s )| acquiring lock", currentOpId.toString().c_str(),
			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());
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
	Logger::sharding(Logger::Step, "ShardCopy(opid=%s, cp {%s in %d} to %s )| Transferring ...", currentOpId.toString().c_str(),
			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());
	// 1. register this transaction in shard manager to receive MM notification
	ShardManager::getShardManager()->registerMMSessionListener(currentOpId.operationId, this);
	// 2. send copyToMe notification to the srcNode to start transferring the data
	this->copyToMeNotif = SP(CopyToMeNotification)(new CopyToMeNotification(replicaShardId, unassignedShardId));

	// NOTE : this is deallocated by the state machine
	ConcurrentNotifOperation * copyer = new ConcurrentNotifOperation(copyToMeNotif, ShardingCopyToMeACKMessageType, srcNodeId , this);
	copyer->setOperationId(currentOpId.operationId);
	ShardManager::getShardManager()->getStateMachine()->registerOperation(copyer);
}

void ShardCopyOperation::end(map<NodeId, SP(ShardingNotification) > & replies){
	if(replies.size() < 1){
		this->successFlag = false;
		release();
		return;
	}
	Logger::sharding(Logger::Detail, "ShardCopy(opid=%s, cp {%s in %d} to %s )| CopyToMe Ack received.", currentOpId.toString().c_str(),
			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());
    if(transferAckReceived){
        consume(transferStatus);
    }else{
        transferAckReceived = true;
    }
}
// if returns true, operation must stop and return null to state_machine
bool ShardCopyOperation::shouldAbort(const NodeId & failedNode){
	if(failedNode == srcNodeId){
		Logger::sharding(Logger::Step, "ShardCopy(opid=%s, cp {%s in %d} to %s )| srcNode failed. Aborting.", currentOpId.toString().c_str(),
				replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());
		this->successFlag = false;
		release();
		return true;
	}
	return false;
}

// for transfer
void ShardCopyOperation::consume(const ShardMigrationStatus & status){
    if(transferAckReceived){
    	Logger::sharding(Logger::Step, "ShardCopy(opid=%s, cp {%s in %d} to %s )| Transfer Done: %s", currentOpId.toString().c_str(),
    			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str(), status.status == MM_STATUS_FAILURE ? "Failed" : "Successful");
        // failed or succeed?
        if(status.status == MM_STATUS_FAILURE){
            this->successFlag = false;
            release();
        }else if(status.status == MM_STATUS_SUCCESS){
        	const Cluster_Writeview * writeview = ((WriteviewTransaction *)(this->getTransaction().get()))->getWriteview();
            string indexDirectory = ShardManager::getShardManager()->getConfigManager()->getShardDir(writeview->clusterName,
                    writeview->cores.at(unassignedShardId.coreId)->getName(), &unassignedShardId);
            if(indexDirectory.compare("") == 0){
                indexDirectory = ShardManager::getShardManager()->getConfigManager()->createShardDir(writeview->clusterName,
                        writeview->cores.at(unassignedShardId.coreId)->getName(), &unassignedShardId);
            }
            physicalShard = LocalPhysicalShard(status.shard, indexDirectory, "");
            commit();
        }
    }else{
		Logger::sharding(Logger::Detail, "ShardCopy(opid=%s, cp {%s in %d} to %s )| MM status received.", currentOpId.toString().c_str(),
				replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());
        transferAckReceived = true;
        transferStatus = status;
    }
}
void ShardCopyOperation::commit(){
	Logger::sharding(Logger::Step, "ShardCopy(opid=%s, cp {%s in %d} to %s )| commit shard assign change", currentOpId.toString().c_str(),
			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());

	// start metadata commit
	// prepare the shard change
	ShardAssignChange * shardAssignChange = new ShardAssignChange(unassignedShardId, ShardManager::getCurrentNodeId(), 0);
	shardAssignChange->setPhysicalShard(physicalShard);
	this->committer = new AtomicMetadataCommit(vector<NodeId>(), shardAssignChange, this, true); // last true arg : skip lock
	currentAction = "commit";
	this->committer->produce();
}
// ** end if
void ShardCopyOperation::release(){
	// release the locks
	currentAction = "release";
	Logger::sharding(Logger::Step, "ShardCopy(opid=%s, cp {%s in %d} to %s )| releasing lock", currentOpId.toString().c_str(),
			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str());

	this->releaser->produce();
}

void ShardCopyOperation::finalize(){ // ** return **
	this->finalizedFlag = true;
	Logger::sharding(Logger::Step, "ShardCopy(opid=%s, cp {%s in %d} to %s )| finalizing.Result : %s", currentOpId.toString().c_str(),
			replicaShardId.toString().c_str(), srcNodeId, unassignedShardId.toString().c_str() , this->successFlag ? "Successful" : "Failed");
	this->getConsumer()->consume(this->successFlag);
}

}
}
