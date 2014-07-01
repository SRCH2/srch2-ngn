#include "CommandAggregator.h"


namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


// refreshes this object to be used for another command session
// cluster objects are deleted here.
void CommandResultsSynchronizer::referesh(){
	ASSERT(isAvailable());
	// 1. Make it unavailable to others by calling wait()
	wait();

	// 2. Delete cluster info
	for(std::map<NodeId, Cluster *>::iterator resultItr = resultClusterInfos.begin(); resultItr != resultClusterInfos.end(); ++resultItr){
		ASSERT(resultItr->second != NULL);
		if(resultItr->second != NULL){
			delete resultItr->second;
		}
	}

	// 3. Clear all result containers
	resultClusterInfos.clear();
	resultCommandCodes.clear();
	resultTransactionIds.clear();

	// 4. call release to make it available to others
	release();
}

// makes the content available to ShardManger
void CommandResultsSynchronizer::release(){
	isAvailableLock.lock();
	Logger::console("Result synchronizer is released with %d nodes replied.", resultTransactionIds.size());
	isAvailableFlag = true;
	isAvailableLock.unlock();
}

// makes the content unavailbale to ShardManager until CommnadAggregator calls release()
void CommandResultsSynchronizer::wait(){
	isAvailableLock.lock();
	isAvailableFlag = false;
	isAvailableLock.unlock();
}

// Tells ShardManager (or any caller) whether the content is readible or not.
bool CommandResultsSynchronizer::isAvailable(){
	isAvailableLock.lock();
	bool result = isAvailableFlag ;
	isAvailableLock.unlock();
	return result;
}

// Returns the CommandCode of reply of this node
// Returns false if this node timedout
bool CommandResultsSynchronizer::getReplyCommandCode(NodeId nodeId, SHMRequestReport::CommandCode & code){
	// it must always be available when these getter functions are called
	if(! isAvailable()){
		ASSERT(false);
		return false;
	}
	std::map<NodeId, SHMRequestReport::CommandCode>::iterator codeItr = resultCommandCodes.find(nodeId);
	if(codeItr == resultCommandCodes.end()){
		return false;// timedout
	}

	code = codeItr->second;

	return true;
}
// Returns the transaction id of reply of this node
// returns false if this node timedout
bool CommandResultsSynchronizer::getReplyTransactionId(NodeId nodeId, unsigned & transactionId){
	// it must always be available when these getter functions are called
	if(! isAvailable()){
		ASSERT(false);
		return false;
	}
	std::map<NodeId, unsigned>::iterator transIdItr = resultTransactionIds.find(nodeId);
	if(transIdItr == resultTransactionIds.end()){
		return false;// timedout
	}

	transactionId = transIdItr->second;

	return true;
}

// Returns the cluster info of reply of this node
// returns false if this node timedout
bool CommandResultsSynchronizer::getReplyClusterInfo(NodeId nodeId, Cluster* & clusterInfo){
	// it must always be available when these getter functions are called
	if(! isAvailable()){
		ASSERT(false);
		return false;
	}
	std::map<NodeId, Cluster *>::iterator clusterInfoItr = resultClusterInfos.find(nodeId);
	if(clusterInfoItr == resultClusterInfos.end()){
		return false;// timedout
	}

	clusterInfo = clusterInfoItr->second;

	return true;
}

// adds a new result to the CommandResults object
// returns false if this node already has some results in the maps
// if force is true it replaces the existing node results (if any) and always returns true
// it also returns false if this object is available
bool CommandResultsSynchronizer::addNodeResults(const NodeId & nodeId, const unsigned & transId,
		const SHMRequestReport::CommandCode & code,
		Cluster * clusterInfo,
		bool force){

	// it must not be available to others
	if(isAvailable()){
		ASSERT(false);
		return false;
	}

	std::map<NodeId, unsigned>::iterator transIdItr = resultTransactionIds.find(nodeId);
	std::map<NodeId, SHMRequestReport::CommandCode>::iterator codeItr = resultCommandCodes.find(nodeId);
	std::map<NodeId, Cluster *>::iterator clusterInfoItr = resultClusterInfos.find(nodeId);
	if(transIdItr != resultTransactionIds.end() ||
			codeItr != resultCommandCodes.end() ||
			clusterInfoItr != resultClusterInfos.end()){ // already has some results
		if(! force){
			return false;
		}
	}

	if(transIdItr != resultTransactionIds.end()){
		transIdItr->second = transId;
	}else{
		resultTransactionIds.insert(std::make_pair(nodeId, transId));
	}

	if(codeItr != resultCommandCodes.end()){
		codeItr->second = code;
	}else{
		resultCommandCodes.insert(std::make_pair(nodeId, code));
	}

	if(clusterInfoItr != resultClusterInfos.end()){
		clusterInfoItr->second = clusterInfo;
	}else{
		resultClusterInfos.insert(std::make_pair(nodeId, clusterInfo));
	}

	return true;

}








/////////////////////////////////// Command Aggregator ///////////////////////////////////////////
CommandAggregator::CommandAggregator(SHMRequestReport * request, CommandResultsSynchronizer * synchronizer): ShardManagerAggregator<SHMRequestReport, SHMRequestReport>(NULL){
	this->request = request;
	this->synchronizer = synchronizer;
}
/*
 * This function is always called by Pending Message Framework as the first call back function
 */
void CommandAggregator::preProcess(ResponseAggregatorMetadata metadata){
};
/*
 * This function is called by Pending Message Framework if a timeout happens, The call to
 * this function must be between preProcessing(...) and callBack()
 */
void CommandAggregator::processTimeout(PendingMessage<SHMRequestReport, SHMRequestReport> * message,ResponseAggregatorMetadata metadata){
	ASSERT(message != NULL);
	ASSERT(message->getResponseMessage() == NULL);
	ASSERT(message->getResponseObject() == NULL);
	// no need to do anything more, if synchronizer cannot find anything for a node
	// in the maps, it assumes it has timed out.
};

/*
 * The callBack function used by Pending Message Framework
 */
void CommandAggregator::callBack(PendingMessage<SHMRequestReport, SHMRequestReport> * message){

	ASSERT(message != NULL);
	ASSERT(message->getResponseMessage() != NULL);
	ASSERT(message->getResponseObject() != NULL);

	// synchronizer must not be available at this point
	ASSERT(! this->synchronizer->isAvailable());

	if(message == NULL) return;
	NodeId nodeId = message->getNodeId();

	// get response information
	unsigned transactionId = message->getResponseObject()->getTransactionId();
	SHMRequestReport::CommandCode code = message->getResponseObject()->getCommandCode();
	Cluster * clusterInfo = message->getResponseObject()->getClusterWriteview();
	// save information in the result synchronizer
	if(! this->synchronizer->addNodeResults(nodeId, transactionId, code, clusterInfo)){
		// some information is already saved for this nodeId, node has sent more than one reply
		// it's wrong!!!
		Logger::console("ERROR : Node %d has more than one reply." , nodeId);
		ASSERT(false);
		// cluster is not deleted in the command object
		delete clusterInfo;
		return;
	}

};
void CommandAggregator::callBack(vector<PendingMessage<SHMRequestReport, SHMRequestReport> * > messages){

	// synchronizer must not be available at this point
	ASSERT(! this->synchronizer->isAvailable());
	// iterate on all responses and save their information in the resultSynchronizer
	for(unsigned msgIndex = 0 ; msgIndex < messages.size(); ++msgIndex){
		callBack(messages.at(msgIndex));
	}
};

/*
 * The last call back function called by Pending Message Framework in all cases.
 * Example of call back call order for search :
 * 1. preProcessing()
 * 2. timeoutProcessing() [only if some shard times out]
 * 3. aggregateSearchResults()
 * 4. finalize()
 */
void CommandAggregator::finalize(ResponseAggregatorMetadata metadata){
	// Make ShardManger understand that it can continue with reading the results.
	Logger::console("CommandAggregator is releasing the result synchronizer.");
	this->synchronizer->release();
};

}
}
