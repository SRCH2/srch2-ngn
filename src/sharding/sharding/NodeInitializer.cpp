

#include "NodeInitializer.h"


#include "core/util/Assert.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

NodeInitializer * NodeInitializer::singleInstance = 0x0;
NodeInitializer * NodeInitializer::createNodeInitializer(){
	if(singleInstance != NULL){
		ASSERT(false);
		return singleInstance;
	}
	singleInstance = new NodeInitializer();
	return singleInstance;
}
NodeInitializer * NodeInitializer::getNodeInitializer(){
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	ASSERT(MetadataManager::getMetadataManager() != NULL);
	ASSERT(LockManager::getLockManager() != NULL);
	return singleInstance;
}

NodeInitializer * NodeInitializer::createNodeInitializer(){
	if(singleInstance != NULL){
		ASSERT(false);
		return singleInstance;
	}
	singleInstance = new NodeInitializer();
	return singleInstance;
}

NodeInitializer * NodeInitializer::getNodeInitializer(){
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	return singleInstance;
}


void NodeInitializerOperation::OUTPUT::execute(){
	//TODO : send the outputNotification to destNode,destOperation
}
StateTransitionOutputType NodeInitializerOperation::OUTPUT::getType(){
	return LockOutputType_NodeInitialization;
}

NodeInitializerOperation::NodeInitializerOperation(unsigned operationId, NodeId newNodeId, NodeId hostNodeId):OperationState(operationId){
	this->newNodeId = newNodeId;
	this->hostNodeId = hostNodeId;
}
NodeId NodeInitializerOperation::getNewNodeId() const{
return newNodeId;
}
NodeId NodeInitializerOperation::getHostNodeId() const{
return hostNodeId;
}
OperationStateType NewNodeOperation::INIT::getType(){
	return 	OperationStateType_NodeInitializer_NewNode_Init;
}
virtual std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::INIT::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::INIT::handle(NodeInitNotification::WELCOME * inputNotification){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::INIT::handle(NodeInitNotification::NEW_HOST * inputNotification){
	// TODO
	return {this, NULL};
}

bool NewNodeOperation::INIT::doesExpect(NodeInitNotification::WELCOME * inputNotification){
	// TODO
	return true;
}
bool NewNodeOperation::INIT::doesExpect(NodeInitNotification::NEW_HOST * inputNotification){
	// TODO
	return true;
}
OperationStateType NewNodeOperation::READY::getType(){
	return OperationStateType_NodeInitializer_NewNode_Ready;
}
std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::READY::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::READY::handle(NodeInitNotification::NEW_HOST * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::READY::handle(NodeInitNotification::SHARD_OFFER * inputNotification){
	// TODO
	return {this, NULL};
}

bool NewNodeOperation::READY::doesExpect(NodeInitNotification::NEW_HOST * inputNotification){
	// TODO
	return true;
}
bool NewNodeOperation::READY::doesExpect(NodeInitNotification::SHARD_OFFER * inputNotification){
	// TODO
	return true;
}
OperationStateType NewNodeOperation::DONE::getType(){
	return OperationStateType_NodeInitializer_NewNode_Done;
}
std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::DONE::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::DONE::handle(NodeInitNotification::NEW_HOST * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> NewNodeOperation::DONE::handle(NodeInitNotification::JOIN_PERMIT * inputNotification){
	// TODO
	return {this, NULL};
}

bool NewNodeOperation::DONE::doesExpect(NodeInitNotification::NEW_HOST * inputNotification){
	// TODO
	return true;
}
bool NewNodeOperation::DONE::doesExpect(NodeInitNotification::JOIN_PERMIT * inputNotification){
	// TODO
	return true;
}
OperationStateType HostNodeOperation::WAIT::getType(){
	return OperationStateType_NodeInitializer_HostNode_Wait;
}
std::pair<OperationState *, StateTransitionOutput *> HostNodeOperation::WAIT::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> HostNodeOperation::WAIT::handle(NodeInitNotification::BUSY * inputNotification){
	// TODO
	return {this, NULL};
}
bool HostNodeOperation::WAIT::doesExpect(NodeInitNotification::BUSY * inputNotification){
	// TODO
	return true;
}
OperationStateType HostNodeOperation::OFFERED::getType(){
	return 	OperationStateType_NodeInitializer_HostNode_Offered;
}
std::pair<OperationState *, StateTransitionOutput *> HostNodeOperation::OFFERED::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> HostNodeOperation::OFFERED::handle(NodeInitNotification::SHARDS_READY * inputNotification){
	// TODO
	return {this, NULL};
}
bool HostNodeOperation::OFFERED::doesExpect(NodeInitNotification::SHARDS_READY * inputNotification){
	// TODO
	return true;
}
OperationStateType HostNodeOperation::UPDATING::getType(){
	return OperationStateType_NodeInitializer_HostNode_Updating;
}

std::pair<OperationState *, StateTransitionOutput *> HostNodeOperation::UPDATING::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> HostNodeOperation::UPDATING::handle(CommitNotification::ACK * inputNotification){
	// TODO
	return {this, NULL};
}
bool HostNodeOperation::UPDATING::doesExpect(CommitNotification::ACK * inputNotification){
	// TODO
	return true;
}
OperationStateType OtherNodesOperation::WAIT::getType(){
	return OperationStateType_NodeInitializer_OtherNodes_Wait;
}
std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::WAIT::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::WAIT::handle(CommitNotification::ACK * inputNotification){
	// TODO
	return {this, NULL};
}
bool OtherNodesOperation::WAIT::doesExpect(CommitNotification::ACK * inputNotification){
	// TODO
	return true;
}
OperationStateType OtherNodesOperation::RECOVERY::getType(){
	return OperationStateType_NodeInitializer_OtherNodes_Recovery;
}

std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::RECOVERY::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::RECOVERY::handle(NodeInitNotification::BUSY * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::RECOVERY::handle(NodeInitNotification::SHARD_REQUEST * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::RECOVERY::handle(NodeInitNotification::SHARDS_READY * inputNotification){
	// TODO
	return {this, NULL};
}
bool OtherNodesOperation::RECOVERY::doesExpect(NodeInitNotification::BUSY * inputNotification){
	// TODO
	return true;
}
bool OtherNodesOperation::RECOVERY::doesExpect(NodeInitNotification::SHARD_REQUEST * inputNotification){
	// TODO
	return true;
}
bool OtherNodesOperation::RECOVERY::doesExpect(NodeInitNotification::SHARDS_READY * inputNotification){
	// TODO
	return true;
}
OperationStateType OtherNodesOperation::UPDATING::getType(){
	return OperationStateType_NodeInitializer_OtherNodes_Updating;
}

std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::UPDATING::entry(){
	// TODO
	return {this, NULL};
}

std::pair<OperationState *, StateTransitionOutput *> OtherNodesOperation::UPDATING::handle(CommitNotification::ACK * inputNotification){
	// TODO
	return {this, NULL};
}
bool OtherNodesOperation::UPDATING::doesExpect(CommitNotification::ACK * inputNotification){
	// TODO
	return true;
}

}
}
