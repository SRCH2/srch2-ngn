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
#ifndef __SHARDING_PROCESSOR_SEARCH_COMMAND_RESULTS_NOTIFICATION_H_
#define __SHARDING_PROCESSOR_SEARCH_COMMAND_RESULTS_NOTIFICATION_H_
#include "sharding/configuration/ShardingConstants.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>
#include "sharding/sharding/notifications/Notification.h"


namespace srch2is = srch2::instantsearch;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class SearchCommandResults : public ShardingNotification{
public:

	struct ShardResults{
	public:
		ShardResults(const string & shardIdentifier):
			shardIdentifier(shardIdentifier), searcherTime(0){
		}
		const string shardIdentifier;
	    QueryResults queryResults;
	    QueryResultFactory resultsFactory;
	    // extra information to be added later
	    uint32_t searcherTime;


	    //serializes the object to a byte array and places array into the region
	    //allocated by given allocator
	    void* serialize(void * buffer);

	    unsigned getNumberOfBytes() const;

	    //given a byte stream recreate the original object
	    static ShardResults * deserialize(void* buffer);

	};

    SearchCommandResults(){
    }

    ~SearchCommandResults();

    void addShardResults(SearchCommandResults::ShardResults * shardResults);

    vector<ShardResults *> & getShardResults();

    void* serializeBody(void * buffer) const;

    unsigned getNumberOfBytesBody() const;

    //given a byte stream recreate the original object
    void * deserializeBody(void * buffer);

    ShardingMessageType messageType() const;

    bool resolveNotification(SP(ShardingNotification) _notif);

private:
    vector<ShardResults *> shardResults;
};


}
}

#endif // __SHARDING_PROCESSOR_SEARCH_COMMAND_RESULTS_NOTIFICATION_H_


