#include "Transaction.h"
#include "TransactionSession.h"
#include "../state_machine/State.h"

#include "core/util/Assert.h"
#include "core/util/Logger.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


Transaction::Transaction():transactionId(OperationState::getNextOperationId()){
	attachedFlag = true;
	finishedFlag = false;
	session = NULL;
}

Transaction::~Transaction(){
    __FUNC_LINE__
	delete session;
}

void Transaction::startTransaction(Transaction * trans){
	if(trans == NULL){
		ASSERT(false);
		return;
	}
	if(! trans->run() || ! trans->isAttached()){
		__FUNC_LINE__
	    Logger::sharding(Logger::Error, "New transaction could not be attached to another thread, so it was aborted.");
		delete trans;
	}
}

TRANS_ID Transaction::getTID() const {
	return this->transactionId;
}

void Transaction::setAttached(){
	this->attachedFlag = true;
}
void Transaction::setUnattached(){
		this->attachedFlag = false;
}
bool Transaction::isAttached() const{
	return this->attachedFlag;
}

bool Transaction::isFinished() const{
	return this->finishedFlag;
}
TransactionSession * Transaction::getSession(){
	return this->session;
}

void Transaction::setSession(TransactionSession * session){
	ASSERT(session != NULL);
	this->session = session;
}

}
}
