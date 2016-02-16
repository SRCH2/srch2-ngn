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
#ifndef __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__
#define __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__

#include "../../state_machine/State.h"
#include "../../state_machine/ConsumerProducer.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"
#include "../Transaction.h"
#include "core/util/Logger.h"
#include "core/util/Assert.h"

#include <iostream>
#include <ctime>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


class AclAttributeReadCommand: public ProducerInterface, public NodeIteratorListenerInterface {
public:

	// calls consume(const vector<string> & attributeIds, const vector<JsonMessageCode> & messages); from the consumer
	AclAttributeReadCommand(ConsumerInterface * consumer,
			const string & roleId,
			const CoreInfo_t * coreInfo);
	~AclAttributeReadCommand(){};

	SP(Transaction) getTransaction();

	void produce();

	void readListOfAttributes(NodeTargetShardInfo & target);

	// response which contains the list of attributes comes to this function
	void end(map<NodeId, SP(ShardingNotification) > & replies);

	void finalize(bool status, const vector<unsigned>& listOfSearchableAttributes,
			const vector<unsigned>& listOfRefiningAttributes);

	void finalize();

	string getName() const {return "acl-attribute-read-command";};
private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	const CoreInfo_t * coreInfo;
	const string roleId;
	NodeTargetShardInfo target;

	vector<JsonMessageCode> messageCodes;

};


}
}

#endif // __SHARDING_SHARDING_ACL_ATTR_READ_COMMAND_H__
