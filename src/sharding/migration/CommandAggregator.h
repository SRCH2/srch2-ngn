#ifndef __SHARDING_MIGRATION_COMMAND_AGGREGATOR_H__
#define __SHARDING_MIGRATION_COMMAND_AGGREGATOR_H__

#include "sharding/routing/ResponseAggregator.h"
#include "sharding/migration/ShardManagerAggregator.h"
#include "sharding/routing/PendingMessages.h"
#include "sharding/migration/ShardManagerCommand.h"

#include <map>

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

class Cluster;

// This class is a container which provides the results of a CommandAggregator
// to ShardManager.
// When CommandAggregator is done with it's work, it prepares the CommandResults
// and calls release() to make it available to ShardManager which periodically calls
// isAvailable() to see whether it's unblocked yet or not.
class CommandResultsSynchronizer{
public:

	// refreshes this object to be used for another command session
	// cluster objects are deleted here.
	void referesh();

	// makes the content available to ShardManger
	void release();

	// makes the content unavailbale to ShardManager until CommnadAggregator calls release()
	void wait();

	// Tells ShardManager (or any caller) whether the content is readible or not.
	bool isAvailable();

	// Returns the CommandCode of reply of this node
	// Returns false if this node timedout
	bool getReplyCommandCode(NodeId nodeId, SHMRequestReport::CommandCode & code);

	// Returns the transaction id of reply of this node
	// returns false if this node timedout
	bool getReplyTransactionId(NodeId nodeId, unsigned & transactionId);

	// Returns the cluster info of reply of this node
	// returns false if this node timedout
	bool getReplyClusterInfo(NodeId nodeId, Cluster* & clusterInfo);

	// adds a new result to the CommandResults object
	// returns false if this node already has some results in the maps
	// if force is true it replaces the existing node results (if any) and always returns true
	bool addNodeResults(const NodeId & nodeId, const unsigned & transId, const SHMRequestReport::CommandCode & code, Cluster * clusterInfo, bool force = false);

private:

	boost::mutex isAvailableLock;
	bool isAvailableFlag;

	// the results of a command
	std::map<NodeId, unsigned> resultTransactionIds;
	std::map<NodeId, SHMRequestReport::CommandCode> resultCommandCodes;
	std::map<NodeId, Cluster *> resultClusterInfos;
	// TODO : we must also have a place for metadata objects

};


class CommandAggregator: public ShardManagerAggregator<SHMRequestReport, SHMRequestReport>{
public:


	CommandAggregator(SHMRequestReport * request, CommandResultsSynchronizer * synchronizer);
	/*
     * This function is always called by Pending Message Framework as the first call back function
     */
    void preProcess(ResponseAggregatorMetadata metadata);
    /*
     * This function is called by Pending Message Framework if a timeout happens, The call to
     * this function must be between preProcessing(...) and callBack()
     */
    void processTimeout(PendingMessage<SHMRequestReport, SHMRequestReport> * message,ResponseAggregatorMetadata metadata);

    /*
     * The callBack function used by Pending Message Framework
     */
    void callBack(PendingMessage<SHMRequestReport, SHMRequestReport> * message);
    void callBack(vector<PendingMessage<SHMRequestReport, SHMRequestReport> * > messages);

    /*
     * The last call back function called by Pending Message Framework in all cases.
     * Example of call back call order for search :
     * 1. preProcessing()
     * 2. timeoutProcessing() [only if some shard times out]
     * 3. aggregateSearchResults()
     * 4. finalize()
     */
    void finalize(ResponseAggregatorMetadata metadata);


private:

    // should not be deleted in the aggregator because it's deleted in RM
    const SHMRequestReport * request;

    CommandResultsSynchronizer * synchronizer;
};

}
}


#endif // __SHARDING_MIGRATION_COMMAND_AGGREGATOR_H__
