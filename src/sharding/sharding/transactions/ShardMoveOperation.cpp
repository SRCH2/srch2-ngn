#include "ShardMoveOperation.h"
#include "core/util/SerializationHelper.h"
#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "core/operation/IndexData.h"//for debug purpose
#include "server/Srch2Server.h"
#include "./cluster_transactions/LoadBalancer.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "../ShardManager.h"
#include "../../configuration/ShardingConstants.h"
#include "../state_machine/node_iterators/ConcurrentNotifOperation.h"
#include "../state_machine/StateMachine.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


ShardMoveOperation::ShardMoveOperation(const NodeId & srcNodeId,
		const ClusterShardId & moveShardId, ConsumerInterface * consumer):
		ProducerInterface(consumer), shardId(moveShardId){
	ASSERT(this->getTransaction() != NULL);
	this->srcAddress = NodeOperationId(srcNodeId, OperationState::DataRecoveryOperationId);
	this->currentOpId = NodeOperationId(ShardManager::getCurrentNodeId(), OperationState::getNextOperationId());
	this->successFlag = true;
	this->locker = NULL;
	this->releaser = NULL;
	this->committer = NULL;
	this->transferAckReceived = false;
	this->currentOp = PreStart;
}

ShardMoveOperation::~ShardMoveOperation(){
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

SP(Transaction) ShardMoveOperation::getTransaction() {
	if(this->getConsumer() == NULL){
		return SP(Transaction)();
	}
	return this->getConsumer()->getTransaction();
}


void ShardMoveOperation::produce(){
	Logger::sharding(Logger::Step, "ShardMove(opid=%s, mv {%s in %s} to self )| Starting ...", currentOpId.toString().c_str(),
			shardId.toString().c_str(), srcAddress.toString().c_str());
	lock();
}


void ShardMoveOperation::lock(){ // **** START ****
	Logger::sharding(Logger::Detail, "ShardMove(opid=%s, mv {%s in %s} to self )| Acquiring lock", currentOpId.toString().c_str(),
			shardId.toString().c_str(), srcAddress.toString().c_str());
	this->locker = new AtomicLock(shardId, currentOpId, LockLevel_X, this);
	// locker calls all methods of LockResultCallbackInterface from us
	this->currentOp = Lock;
	this->locker->produce();
}
void ShardMoveOperation::consume(bool granted){
	switch (currentOp) {
		case PreStart:
			ASSERT(false);
			finalize();
			break;
		case Lock:
		{
			if(granted){
				transfer();
			}else{
				this->successFlag = false;
				finalize();
			}
			break;
		}
		case Transfer:
			ASSERT(false);
			break;
		case Commit:
			if(granted){
				release();
			}else{
				ASSERT(false);
				this->successFlag = false;
				finalize();
			}
			break;
		case Release:
			if(granted){
				finalize();
			}else{
				ASSERT(false);
				this->successFlag = false;
				finalize();
			}
			break;
		default:
			ASSERT(false);
			finalize();
			break;
	}
}
// **** If lock granted
void ShardMoveOperation::transfer(){
	Logger::sharding(Logger::Step, "ShardMove(opid=%s, mv {%s in %s} to self )| Starting transfer", currentOpId.toString().c_str(),
			shardId.toString().c_str(), srcAddress.toString().c_str());
	// transfer data by ordering MM
	// 1. register this transaction in shard manager to receive MM notification
	ShardManager::getShardManager()->registerMMSessionListener(currentOpId.operationId, this);
	// 2. send copyToMe notification to the srcNode to start transferring the data
	this->moveToMeNotif = SP(MoveToMeNotification)(new MoveToMeNotification(shardId));

	// NOTE : this is deallocated by the state machine
	ConcurrentNotifOperation * copyer = new ConcurrentNotifOperation(moveToMeNotif,
			ShardingMoveToMeACKMessageType, srcAddress.nodeId , this);
	copyer->setOperationId(currentOpId.operationId);
	this->currentOp = Transfer;
	ShardManager::getShardManager()->getStateMachine()->registerOperation(copyer);
}


void ShardMoveOperation::end(map<NodeId, SP(ShardingNotification) > & replies){
	if(replies.empty()){
		this->successFlag = false;
		finalize();
		return;
	}
    if(this->currentOp == Transfer){
        if(transferAckReceived){
           consume(transferStatus);
        }else{
            transferAckReceived = true;
        	Logger::sharding(Logger::Detail, "ShardMove(opid=%s, mv {%s in %s} to self )| MoveToMe Ack received.",
        			currentOpId.toString().c_str(),
        			shardId.toString().c_str(), srcAddress.toString().c_str());
        }
    }
}


// for transfer
void ShardMoveOperation::consume(const ShardMigrationStatus & status){
	// failed or succeed?
    if(! transferAckReceived){
        transferAckReceived = true;
        transferStatus = status;
    	Logger::sharding(Logger::Detail, "ShardMove(opid=%s, mv {%s in %s} to self )| MM status received.",
    			currentOpId.toString().c_str(),
    			shardId.toString().c_str(), srcAddress.toString().c_str());
    }else{
    	Logger::sharding(Logger::Step, "ShardMove(opid=%s, mv {%s in %s} to self )| Transfer Done. Result : %s",
    			currentOpId.toString().c_str(),
    			shardId.toString().c_str(), srcAddress.toString().c_str(),
    			status.status == MM_STATUS_FAILURE ? "Failure" : "Success");
        if(status.status == MM_STATUS_FAILURE){
            this->successFlag = false;
            release();
        }else if(status.status == MM_STATUS_SUCCESS){
        	const Cluster_Writeview * writeview = ((WriteviewTransaction *)(this->getTransaction().get()))->getWriteview();
            string indexDirectory = ShardManager::getShardManager()->getConfigManager()->getShardDir(writeview->clusterName,
                    writeview->cores.at(shardId.coreId)->getName(), &shardId);
            if(indexDirectory.compare("") == 0){
                indexDirectory = ShardManager::getShardManager()->getConfigManager()->createShardDir(writeview->clusterName,
                        writeview->cores.at(shardId.coreId)->getName(), &shardId);
            }

            physicalShard = LocalPhysicalShard(status.shard, indexDirectory, "");
            if(physicalShard.server->__debugShardingInfo != NULL){
            	physicalShard.server->__debugShardingInfo->shardName = shardId.toString();
            	SP(const ClusterNodes_Writeview) nodesWriteview = ShardManager::getNodesWriteview_read();
            	physicalShard.server->__debugShardingInfo->nodeName =
            			nodesWriteview->getNodes_read().find(ShardManager::getCurrentNodeId())->second.second->toStringShort();
            	stringstream ss;
            	ss << "Result of shard move from node " << srcAddress.nodeId << " with " <<
            			physicalShard.server->getIndexer()->getNumberOfDocumentsInIndex() << " records.";
            	physicalShard.server->__debugShardingInfo->explanation = ss.str();
            }
            commit();
        }
    }
}

// **** If transfer was successful
void ShardMoveOperation::commit(){
	Logger::sharding(Logger::Step, "ShardMove(opid=%s, mv {%s in %s} to self )| committing move change.",
			currentOpId.toString().c_str(),
			shardId.toString().c_str(), srcAddress.toString().c_str());
	// prepare the shard change
	ShardMoveChange * shardMoveChange = new ShardMoveChange(shardId, srcAddress.nodeId, ShardManager::getCurrentNodeId());
	shardMoveChange->setPhysicalShard(physicalShard);
	committer = new AtomicMetadataCommit(vector<NodeId>(), shardMoveChange, this, true);
	this->currentOp = Commit;
	committer->produce();
}
// decide to do the cleanup
// **** end if

// **** end if
void ShardMoveOperation::release(){
	// release the locks
	this->releaser = new AtomicRelease(shardId, currentOpId, this);
	// releaser calls all methods of BooleanCallbackInterface from us
	Logger::sharding(Logger::Step, "ShardMove(opid=%s, mv {%s in %s} to self )| releasing lock.", currentOpId.toString().c_str(),
			shardId.toString().c_str(), srcAddress.toString().c_str());
	this->currentOp = Release;
	this->releaser->produce();
}

void ShardMoveOperation::finalize(){ // ***** END *****
	Logger::sharding(Logger::Step, "ShardMove(opid=%s, mv {%s in %s} to self )| Finalizing. Result : %s", currentOpId.toString().c_str(),
			shardId.toString().c_str(), srcAddress.toString().c_str(), this->successFlag ? "Success" : "Failure");
	this->getConsumer()->consume(this->successFlag);
}

}
}

