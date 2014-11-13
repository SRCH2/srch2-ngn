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
		ShardResults(const string & shardIdentifier):shardIdentifier(shardIdentifier){
		}
		const string shardIdentifier;
	    QueryResults queryResults;
	    QueryResultFactory resultsFactory;
	    // extra information to be added later
	    unsigned searcherTime;


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


