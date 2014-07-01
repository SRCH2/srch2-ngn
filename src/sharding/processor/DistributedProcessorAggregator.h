#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_


#include "sharding/routing/ResponseAggregator.h"


#include "sharding/configuration/ConfigManager.h"

#include <instantsearch/Record.h>
#include "wrapper/ParsedParameterContainer.h"
#include "util/CustomizableJsonWriter.h"
#include "util/Logger.h"

#include "core/highlighter/Highlighter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "sharding/processor/ProcessorUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class Request, class Response>
class DistributedProcessorAggregator : public ResponseAggregatorInterface<Request, Response>{
public:


	DistributedProcessorAggregator(boost::shared_ptr<const Cluster> clusterReadview, unsigned coreId):ResponseAggregatorInterface<Request,Response>(clusterReadview){
		this->coreId = coreId;
	}

	unsigned getCoreId(){
		return this->coreId;
	}

    virtual ~DistributedProcessorAggregator(){};


private:
    unsigned coreId;
};

}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_
