#include "sharding/routing/Multiplexer.h"


namespace srch2 {
namespace httpwrapper {

Multiplexer::Multiplexer(ConfigManager& configManager, CoreShardInfo& coreShardInfo) :
  configManager(configManager), coreShardInfo(coreShardInfo), coreInfo(*configManager.getCoreInfo(coreShardInfo.coreName)) {

	// prepare the destination shardIds
	destinations = coreInfo.getShardsVector();

	// iteration is disabled by default
	destinationsIterator = (unsigned)-1;
}

size_t Multiplexer::size() {
  return destinations.size();
}

// initializes the object and makes it ready for an iteration
void Multiplexer::initIteration(){
	destinationsIterator = 0;
}
// checks to see if iteration is finished.
// if returns true, user can call getShardId()
bool Multiplexer::hasMore(){
	if(destinationsIterator >= destinations.size()){
		// also disable the iterator
		destinationsIterator = (unsigned)-1;
		return false;
	}
	return true;
}

// after calling this function, iterator is ready to get next
// shard by calling getNextShardId
void Multiplexer::nextIteration(){
	if(hasMore() == false){
		return;
	}
	++destinationsIterator;
}
ShardId Multiplexer::getNextShardId(){
	if(hasMore() == false){
		return ShardId();
	}
	return destinations.at(destinationsIterator);
}

}}
