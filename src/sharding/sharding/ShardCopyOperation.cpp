#include "ShardCopyOperation.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


ShardCopyOperation::OUTPUT::OUTPUT(ShardCopyOperation * shardCopyOperation, Notification * outputNotification){

}
void ShardCopyOperation::OUTPUT::execute(){
	//TODO
}
StateTransitionOutputType ShardCopyOperation::OUTPUT::getType(){
	// TODO
	switch (outputNotification->getType()) {
		case NotificationType_Sharding_Proposal_OK:
			return StateTransitionOutputType_Proposal_OK;
		case NotificationType_Sharding_Proposal_NO:
			return StateTransitionOutputType_Proposal_NO;
		default:
			return LockOutputType_Default;
	}
}

ShardCopyOperation::ShardCopyOperation(unsigned operationId, ShardCopyChange * change):OperationState(operationId){
	this->change = change;
}

ShardCopyOperation::SRC::COPYING(unsigned operationId, ShardCopyChange * change):ShardCopyOperation(operationId, change){}

OperationStateType ShardCopyOperation::SRC::COPYING::getType(){
	return OperationStateType_ShardCopy_Src_Copying;
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::SRC::COPYING::entry(){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::SRC::COPYING::handle(MMFinishedNotification * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::SRC::COPYING::handle(MMFailedNotification * inputNotification){
	// TODO
	return {this, NULL};
}
bool ShardCopyOperation::SRC::COPYING::doesExpect(MMFinishedNotification * inputNotification){
	// TODO
	return true;
}
bool ShardCopyOperation::SRC::COPYING::doesExpect(MMFailedNotification * inputNotification){
	// TODO
	return true;
}

ShardCopyOperation::DEST::PROPOSED::PROPOSED(unsigned operationId, ShardCopyChange * change):ShardCopyOperation(operationId, change){}

OperationStateType ShardCopyOperation::DEST::PROPOSED::getType(){
	return OperationStateType_ShardCopy_Dest_Proposed;
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::PROPOSED::entry(){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::PROPOSED::handle(ProposalNotification * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::PROPOSED::handle(ProposalNotification::OK * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::PROPOSED::handle(ProposalNotification::NO * inputNotification){
	// TODO
	return {this, NULL};
}
bool ShardCopyOperation::DEST::PROPOSED::doesExpect(ProposalNotification * inputNotification){
	// TODO
	return true;
}
bool ShardCopyOperation::DEST::PROPOSED::doesExpect(ProposalNotification::OK * inputNotification){
	// TODO
	return true;
}
bool ShardCopyOperation::DEST::PROPOSED::doesExpect(ProposalNotification::NO * inputNotification){
	// TODO
	return true;
}
ShardCopyOperation::DEST::LOCKING::LOCKING(unsigned operationId, ShardCopyChange * change):ShardCopyOperation(operationId, change){}
OperationStateType ShardCopyOperation::DEST::LOCKING::getType(){
	return OperationStateType_ShardCopy_Dest_Locking;
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::LOCKING::entry(){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::LOCKING::handle(LockingNotification::GRANTED * inputNotification){
	// TODO
	return {this, NULL};
}
bool ShardCopyOperation::DEST::LOCKING::doesExpect(LockingNotification::GRANTED * inputNotification){
	// TODO
	return true;
}
ShardCopyOperation::DEST::COPYING::COPYING(unsigned operationId, ShardCopyChange * change):ShardCopyOperation(operationId, change){}
OperationStateType ShardCopyOperation::DEST::COPYING::getType(){
	return OperationStateType_ShardCopy_Dest_Copying;
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::COPYING::entry(){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::COPYING::handle(MMFinishedNotification * inputNotification){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::COPYING::handle(MMFailedNotification * inputNotification){
	// TODO
	return {this, NULL};
}
bool ShardCopyOperation::DEST::COPYING::doesExpect(MMFinishedNotification * inputNotification){
	// TODO
	return true;
}
bool ShardCopyOperation::DEST::COPYING::doesExpect(MMFailedNotification * inputNotification){
	// TODO
	return true;
}
ShardCopyOperation::DEST::COMMITTING::COMMITTING(unsigned operationId, ShardCopyChange * change):ShardCopyOperation(operationId, change){}
OperationStateType ShardCopyOperation::DEST::COMMITTING::getType(){
	return OperationStateType_ShardCopy_Dest_Commiting;
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::COMMITTING::entry(){
	// TODO
	return {this, NULL};
}
std::pair<OperationState *, StateTransitionOutput *> ShardCopyOperation::DEST::COMMITTING::handle(CommitNotification::ACK * inputNotification){
	// TODO
	return {this, NULL};
}
bool ShardCopyOperation::DEST::COMMITTING::doesExpect(CommitNotification::ACK * inputNotification){
	// TODO
	return true;
}

}
}
