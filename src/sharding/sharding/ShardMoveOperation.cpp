#include "ShardMoveOperation.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

ShardMoveOperation::OUTPUT::OUTPUT(ShardMoveOperation * shardMoveOperation, Notification * outputNotification){
	//TODO
}
void ShardMoveOperation::OUTPUT::execute(){
	//TODO
}
StateTransitionOutputType ShardMoveOperation::OUTPUT::getType(){
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

ShardMoveOperation::ShardMoveOperation(unsigned operationId, ShardMoveChange * change):OperationState(operationId){
	this->change = change;
};

ShardMoveOperation::DEST::PROPOSED::PROPOSED(unsigned operationId, ShardMoveChange * change):ShardMoveOperation(operationId, change){};
OperationStateType ShardMoveOperation::DEST::PROPOSED::getType(){
	return 	OperationStateType_ShardMove_Dest_Proposed;
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::PROPOSED::entry(){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::PROPOSED::handle(ProposalNotification * inputNotification){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::PROPOSED::handle(ProposalNotification::OK * inputNotification){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::PROPOSED::handle(ProposalNotification::NO * inputNotification){
	//TODO
}
bool ShardMoveOperation::DEST::PROPOSED::doesExpect(ProposalNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::DEST::PROPOSED::doesExpect(ProposalNotification::OK * inputNotification){
	//TODO
}
bool ShardMoveOperation::DEST::PROPOSED::doesExpect(ProposalNotification::NO * inputNotification){
	//TODO
}

ShardMoveOperation::DEST::LOCKING::LOCKING(unsigned operationId, ShardMoveChange * change):ShardMoveOperation(operationId, change){};
OperationStateType ShardMoveOperation::DEST::LOCKING::getType(){
	return 	OperationStateType_ShardMove_Dest_Locking;
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::LOCKING::entry(){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::LOCKING::handle(LockingNotification::GRANTED * inputNotification){
	//TODO
}
bool ShardMoveOperation::DEST::LOCKING::doesExpect(LockingNotification::GRANTED * inputNotification){
	//TODO
}

ShardMoveOperation::DEST::MOVING::MOVING(unsigned operationId, ShardMoveChange * change):ShardMoveOperation(operationId, change){};
OperationStateType ShardMoveOperation::DEST::MOVING::getType(){
	return 	OperationStateType_ShardMove_Dest_Moving;
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::MOVING::entry(){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::MOVING::handle(MMFinishedNotification * inputNotification){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::MOVING::handle(MMFailedNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::DEST::MOVING::doesExpect(MMFinishedNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::DEST::MOVING::doesExpect(MMFailedNotification * inputNotification){
	//TODO
}

ShardMoveOperation::DEST::COMMITTING::COMMITTING(unsigned operationId, ShardMoveChange * change):ShardMoveOperation(operationId, change){};
OperationStateType ShardMoveOperation::DEST::COMMITTING::getType(){
	return 	OperationStateType_ShardMove_Dest_Committing;
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::COMMITTING::entry(){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::DEST::COMMITTING::handle(CommitNotification::ACK * inputNotification){
	//TODO
}
bool ShardMoveOperation::DEST::COMMITTING::doesExpect(CommitNotification::ACK * inputNotification){
	//TODO
}

ShardMoveOperation::SRC::MOVING::MOVING(unsigned operationId, ShardMoveChange * change):ShardMoveOperation(operationId, change){};
OperationStateType ShardMoveOperation::SRC::MOVING::getType(){
	return 	OperationStateType_ShardMove_Src_Moving;
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::MOVING::entry(){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::MOVING::handle(CommitNotification * inputNotification){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::MOVING::handle(MMFinishedNotification * inputNotification){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::MOVING::handle(MMFailedNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::SRC::MOVING::doesExpect(CommitNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::SRC::MOVING::doesExpect(MMFinishedNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::SRC::MOVING::doesExpect(MMFailedNotification * inputNotification){
	//TODO
}

ShardMoveOperation::SRC::CLEANUP::CLEANUP(unsigned operationId, ShardMoveChange * change):ShardMoveOperation(operationId, change){};
OperationStateType ShardMoveOperation::SRC::CLEANUP::getType(){
	return 	OperationStateType_ShardMove_Src_Cleanup;
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::CLEANUP::entry(){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::CLEANUP::handle(LockingNotification * inputNotification){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::CLEANUP::handle(SMNodeFailureNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::SRC::CLEANUP::doesExpect(LockingNotification * inputNotification){
	//TODO
}
bool ShardMoveOperation::SRC::CLEANUP::doesExpect(SMNodeFailureNotification * inputNotification){
	//TODO
}

ShardMoveOperation::SRC::RECOVERY::RECOVERY(unsigned operationId, ShardMoveChange * change):ShardMoveOperation(operationId, change){};
OperationStateType ShardMoveOperation::SRC::RECOVERY::getType(){
	return OperationStateType_ShardMove_Src_Recovery;
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::RECOVERY::entry(){
	//TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardMoveOperation::SRC::RECOVERY::handle(CommitNotification::ACK * recoveryNotification){
	//TODO
}
bool ShardMoveOperation::SRC::RECOVERY::doesExpect(CommitNotification::ACK * inputNotification){
	//TODO
}


}
}

