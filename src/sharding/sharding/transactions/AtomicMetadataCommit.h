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
#ifndef __SHARDING_SHARDING_METADATA_MANAGER_H__
#define __SHARDING_SHARDING_METADATA_MANAGER_H__


#include "../state_machine/State.h"
#include "../notifications/Notification.h"
#include "../notifications/CommitNotification.h"
#include "../metadata_manager/ResourceMetadataChange.h"
#include "../metadata_manager/ResourceMetadataManager.h"
#include "../../configuration/ShardingConstants.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class AtomicRelease;
class AtomicLock;

class AtomicMetadataCommit : public ProducerInterface, public NodeIteratorListenerInterface{
public:

	AtomicMetadataCommit(const vector<NodeId> & exceptions,
			MetadataChange * metadataChange, ConsumerInterface * consumer, const bool skipLock = false);
	AtomicMetadataCommit(const NodeId & exception,
			MetadataChange * metadataChange, ConsumerInterface * consumer, const bool skipLock = false);
	AtomicMetadataCommit(MetadataChange * metadataChange,
			const vector<NodeId> & participants, ConsumerInterface * consumer, const bool skipLock = false);
	~AtomicMetadataCommit();

	SP(Transaction) getTransaction();

	void produce();

	void lock(); // locks metadata
	void consume(bool granted);


	void commit();
	void end_(map<NodeOperationId , SP(ShardingNotification)> & replies) ; // receives notification when commit it done.


	void release(); // unlocks metadata
	string getName() const {return "metadata-commit";};
private:
	// control members
	vector<NodeId> participants;
	NodeOperationId selfOperationId;

	bool skipLock;

	string currentAction;
	AtomicLock * atomicLock;
	AtomicRelease * atomicRelease;
	SP(CommitNotification) commitNotification;
	MetadataChange * metadataChange;

	void finalize(bool result);
};


}
}

#endif // __SHARDING_SHARDING_METADATA_MANAGER_H__
