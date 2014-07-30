#include "ShardAssignOperation.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


ShardAssignOperation::OUTPUT::OUTPUT(ShardAssignOperation * shardAssignOperation, Notification * outputNotification){
	// TODO
}
void ShardAssignOperation::OUTPUT::execute(){
	// TODO
}
StateTransitionOutputType ShardAssignOperation::OUTPUT::getType(){
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
ShardAssignOperation::ShardAssignOperation(unsigned operationId, ShardAssignChange * change):OperationState(operationId){
	this->change = change;
};
ShardAssignOperation::PROPOSED::PROPOSED(unsigned operationId, ShardAssignChange * change):ShardAssignOperation(operationId, change){}
OperationStateType ShardAssignOperation::PROPOSED::getType(){
	return OperationStateType_ShardAssign_Proposed;
}
std::pair<OperationState *, StateTransitionOutput *> ShardAssignOperation::PROPOSED::entry(){
	// TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardAssignOperation::PROPOSED::handle(ProposalNotification * inputNotification){
	// TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardAssignOperation::PROPOSED::handle(ProposalNotification::OK * inputNotification){
	// TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardAssignOperation::PROPOSED::handle(ProposalNotification::NO * inputNotification){
	// TODO
}
bool ShardAssignOperation::PROPOSED::doesExpect(ProposalNotification * inputNotification){
	// TODO
}
bool ShardAssignOperation::PROPOSED::doesExpect(ProposalNotification::OK * inputNotification){
	// TODO
}
bool ShardAssignOperation::PROPOSED::doesExpect(ProposalNotification::NO * inputNotification){
	// TODO
}
ShardAssignOperation::COMMITTING::COMMITTING(unsigned operationId, ShardAssignChange * change):ShardAssignOperation(operationId, change){}
OperationStateType ShardAssignOperation::COMMITTING::getType(){
	return OperationStateType_ShardAssign_Commiting;
}
std::pair<OperationState *, StateTransitionOutput *> ShardAssignOperation::COMMITTING::entry(){
	// TODO
}
std::pair<OperationState *, StateTransitionOutput *> ShardAssignOperation::COMMITTING::handle(CommitNotification::ACK * inputNotification){
	// TODO
}
bool ShardAssignOperation::COMMITTING::doesExpect(CommitNotification::ACK * inputNotification){
	// TODO
}

}
}


