#ifndef __SHARDING_PROCESSOR_PARTITIONER_H_
#define __SHARDING_PROCESSOR_PARTITIONER_H_

using namespace std;
#include <string>
#include "instantsearch/Record.h"
#include "sharding/configuration/ConfigManager.h"

using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {


class Partitioner {
public:

    Partitioner(ConfigManager * configurationManager);

    /*
     * Hash implementation :
     * 1. Uses getRecordValueToHash to find the value to be hashed for this record
     * 2. Uses SynchronizationManager to get total number of shards to know the hash space
     * 3. Uses hash(...) to choose which shard should be responsible for this record
     * 4. Returns the information of corresponding Shard (which can be discovered from SM)
     */
    ShardId getShardIDForRecord(Record * record, string coreName);
    // TODO : if the shard hash value of a record must be calculated by
    // evaluating an expression given in configuration file, primaryKeyStringValue is not enough
    // as the input of this method, this will change later ...
    ShardId getShardIDForRecord(string primaryKeyStringValue, string coreName);


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

    ShardId convertUnsignedToCoreShardInfo(unsigned coreShardIndex, const CoreInfo_t *indexDataContainerConf);

    // computes the hash value of a string
    unsigned hashDJB2(const char *str) const
    {
        unsigned hash = 5381;
        unsigned c;
        do
        {
            c = *str;
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }while(*str++);
        return hash;
    }

private:
    ConfigManager * configurationManager;
};

}
}


#endif // __SHARDING_PROCESSOR_PARTITIONER_H_
