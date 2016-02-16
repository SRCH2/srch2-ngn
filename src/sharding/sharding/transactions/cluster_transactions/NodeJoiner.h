/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__
#define __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommitNotification.h"
#include "../../notifications/LockingNotification.h"
#include "../../notifications/MetadataReport.h"
#include "../Transaction.h"
#include "../TransactionSession.h"
#include <stdlib.h>
#include <time.h>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class AtomicLock;
class AtomicRelease;
class AtomicMetadataCommit;

/*
 * This class (which is kind of state for a state machine)
 * joins a new node to the existing cluster. The protocol starts
 * with entry() and continues with a sequence of handle() functions.
 * When this operation finishes, node is in "joined" state (and for example
 * as a result, wouldn't bounce any notifications.
 */
/*
 * NOTE : The mutex of metadata manager and lock manager is locked/unlocked in handle functions of this class.
 */


class NodeJoiner : public WriteviewTransaction, public NodeIteratorListenerInterface{
public:

	static void join();


	~NodeJoiner();

private:

	NodeJoiner();

	SP(Transaction) getTransaction() ;

	ShardingTransactionType getTransactionType();
	void run();

	void lock();

	// coming back from lock
	void consume(bool booleanResult);

	void readMetadata();

	// coming back from readMetadata
	void end_(map<NodeOperationId, SP(ShardingNotification) > & replies);

	void commit();

	void release();

	void finalizeWork(Transaction::Params * arg);

	void getOlderNodesList(vector<NodeId> & olderNodes);

	string getName() const {return "node-joiner";};

private:
	NodeOperationId selfOperationId;
	NodeId randomNodeToReadFrom;

	enum CurrentOperation{
	    PreStart,
	    Lock,
	    ReadMetadata,
	    Commit,
	    Release
	};
	CurrentOperation currentOperation;
	bool releaseModeFlag;
	AtomicLock * locker;
	AtomicRelease * releaser;
	SP(MetadataReport::REQUEST) readMetadataNotif;
	SP(CommitNotification) commitNotification;
	MetadataChange * metadataChange;
	AtomicMetadataCommit * committer;

};

}
}

#endif // __SHARDING_SHARDING_NEW_NODE_JOIN_OPERATION_H__
