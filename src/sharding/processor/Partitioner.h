#ifndef __SHARDING_PROCESSOR_PARTITIONER_H_
#define __SHARDING_PROCESSOR_PARTITIONER_H_


namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {



class Partitioner {
public:

	Partitioner(RoutingManager * transportManager, ConfigManager * configurationManager){
		this->routingManager = transportManager;
		this->configurationManager = configurationManager;
	}

	/*
	 * Hash implementation :
	 * 1. Uses getRecordValueToHash to find the value to be hashed for this record
	 * 2. Uses SynchronizationManager to get total number of shards to know the hash space
	 * 3. Uses hash(...) to choose which shard should be responsible for this record
	 * 4. Returns the information of corresponding Shard (which can be discovered from SM)
	 */
	CoreShardInfo getShardIDForRecord(Record * record);
	CoreShardInfo getShardIDForRecord(string primaryKeyStringValue);


private:
	/*
	 * Uses Configuration file and the given expression to
	 * calculate the record corresponding value to be hashed
	 * for example if this value is just ID for each record we just return
	 * the value of ID
	 */
	unsigned getRecordValueToHash(Record * record);
	unsigned getRecordValueToHash(string primaryKeyStringValue);
	/*
	 * Uses a hash function to hash valueToHash to a value in range [0,hashSpace)
	 * and returns this value
	 */
	unsigned hash(unsigned valueToHash, unsigned hashSpace);

	CoreShardInfo convertUnsignedToCoreShardInfo(unsigned coreShardIndex);


private:
	RoutingManager * routingManager;
	ConfigManager * configurationManager;
};

}
}


#endif // __SHARDING_PROCESSOR_PARTITIONER_H_
