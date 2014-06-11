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
bool Partitioner::getShardIDForRecord(Record * record, string coreName, ShardId & destination){

    const CoreInfo_t *indexDataContainerConf = configurationManager->getCoreInfo(coreName);

    unsigned valueToHash = getRecordValueToHash(record);

    unsigned totalNumberOfShards = indexDataContainerConf->getNumberOfPrimaryShards();

    ShardId resultShardId = convertUnsignedToCoreShardInfo(hash(valueToHash , totalNumberOfShards), indexDataContainerConf);
    // find out whether shard is still within reach
    unsigned nodeId = configurationManager->getNodeId(resultShardId);
    if (!configurationManager->isValidNode(nodeId)){
        return false;
    }
    destination = resultShardId;
    return true;

}

bool Partitioner::getShardIDForRecord(string primaryKeyStringValue, string coreName, ShardId & destination){

    const CoreInfo_t *indexDataContainerConf = configurationManager->getCoreInfo(coreName);

    unsigned valueToHash = getRecordValueToHash(primaryKeyStringValue);

    unsigned totalNumberOfShards = indexDataContainerConf->getNumberOfPrimaryShards();

    ShardId resultShardId = convertUnsignedToCoreShardInfo(hash(valueToHash , totalNumberOfShards), indexDataContainerConf);
    // find out whether shard is still within reach
    unsigned nodeId = configurationManager->getNodeId(resultShardId);
    if (!configurationManager->isValidNode(nodeId)){
        return false;
    }
    destination = resultShardId;
    return true;
}

/*
 *	Returns all valid shardIds for a broadcast
 */
void Partitioner::getShardIDsForBroadcast(string coreName, vector<ShardId> & broadcastShardIDs){
	const CoreInfo_t *indexDataContainerConf = configurationManager->getCoreInfo(coreName);
	vector<ShardId> allShardIds = indexDataContainerConf->getShardsVector();
	for(vector<ShardId>::iterator shardIdItr = allShardIds.begin(); shardIdItr != allShardIds.end(); ++shardIdItr){
        unsigned nodeId = configurationManager->getNodeId(*shardIdItr);
        if (!configurationManager->isValidNode(nodeId)){
            continue;
        }
        broadcastShardIDs.push_back(*shardIdItr);
	}
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


ShardId Partitioner::convertUnsignedToCoreShardInfo(unsigned coreShardIndex, const CoreInfo_t *indexDataContainerConf){
    return indexDataContainerConf->getPrimaryShardId(coreShardIndex);
}

}
}


