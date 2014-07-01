#ifndef __SHARDING_PROCESSOR_MIGRATION_SHARDMANAGER_AGGREGATOR_H_
#define __SHARDING_PROCESSOR_MIGRATION_SHARDMANAGER_AGGREGATOR_H_


#include "sharding/routing/ResponseAggregator.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


template <class Request, class Response>
class ShardManagerAggregator : public ResponseAggregatorInterface<Request, Response>{

	// NOTE: this class is a top class in hierarchy of aggregators in ShardManager.
	//       architecture wise it is similar DistributedProcessorAggregator
public:
	ShardManagerAggregator(Cluster * cluster): ResponseAggregatorInterface<Request, Response>(cluster){};

};

}
}

#endif // __SHARDING_PROCESSOR_MIGRATION_SHARDMANAGER_AGGREGATOR_H_
