#include "Partitioner.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


//###########################################################################
//                       Partitioner
//###########################################################################


Partitioner::Partitioner(ConfigManager * configurationManager){
    this->configurationManager = configurationManager;
}

/*
 * Hash implementation :
 * 1. Uses getRecordValueToHash to find the value to be hashed for this record
 * 2. Uses TransportationManager to get total number of shards to know the hash space
 * 3. Uses hash(...) to choose which shard should be responsible for this record
 * 4. Returns the information of corresponding Shard (which can be discovered from SM)
 */
const Shard * Partitioner::getShardIDForRecord(Record * record, unsigned coreId, boost::shared_ptr<const Cluster> clusterReadview){

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCoreById(coreId);

    unsigned valueToHash = getRecordValueToHash(record);

    unsigned totalNumberOfShards = clusterReadview->getCoreTotalNumberOfPrimaryShards(coreId);
    Logger::console("Total number of shards to be used in partitioned : %d", totalNumberOfShards);

    return convertUnsignedToCoreShardInfo(hash(valueToHash , totalNumberOfShards), coreId, clusterReadview);

}

const Shard * Partitioner::getShardIDForRecord(string primaryKeyStringValue, unsigned coreId, boost::shared_ptr<const Cluster> clusterReadview){

    const CoreInfo_t *indexDataContainerConf = clusterReadview->getCoreById(coreId);

    unsigned valueToHash = getRecordValueToHash(primaryKeyStringValue);

    unsigned totalNumberOfShards = indexDataContainerConf->getNumberOfPrimaryShards();

    return convertUnsignedToCoreShardInfo(hash(valueToHash , totalNumberOfShards), coreId, clusterReadview);
}

/*
 *	Returns all valid shardIds for a broadcast
 */
void Partitioner::getShardIDsForBroadcast(unsigned coreId,
		boost::shared_ptr<const Cluster> clusterReadview,
		vector<const Shard *> & broadcastShards){

    clusterReadview->addCorePrimaryShards(coreId, broadcastShards);
}


/*
 * Uses Configuration file and the given expression to
 * calculate the record corresponding value to be hashed
 * for example if this value is just ID for each record we just return
 * the value of ID
 */
unsigned Partitioner::getRecordValueToHash(Record * record){

    // When the record is being parsed, configuration is used to compute the hashable value of this
    // record. It will be saved in record.
    string primaryKey = record->getPrimaryKey();
    return hashDJB2(primaryKey.c_str());
}


unsigned Partitioner::getRecordValueToHash(string primaryKeyStringValue){
    return hashDJB2(primaryKeyStringValue.c_str());
}

/*
 * Uses a hash function to hash valueToHash to a value in range [0,hashSpace)
 * and returns this value
 */
unsigned Partitioner::hash(unsigned valueToHash, unsigned hashSpace){
    // use a hash function
    // now simply round robin
    return valueToHash % hashSpace;
}


const Shard * Partitioner::convertUnsignedToCoreShardInfo(unsigned coreShardIndex, unsigned coreId, boost::shared_ptr<const Cluster> clusterReadview){
	vector<const Shard *> corePrimaryShards;
	clusterReadview->addCorePrimaryShards(coreId, corePrimaryShards);
	ASSERT(coreShardIndex < corePrimaryShards.size());
    return corePrimaryShards.at(coreShardIndex);
}

}
}


