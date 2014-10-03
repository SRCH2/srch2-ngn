#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_


#include "sharding/processor/aggregators/ResponseAggregator.h"
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
#include "server/HTTPJsonResponse.h"


namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class Request, class Response>
class DistributedProcessorAggregator : public ResponseAggregatorInterface<Request, Response>{
public:


	DistributedProcessorAggregator(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			unsigned coreId):ResponseAggregatorInterface<Request,Response>(clusterReadview){
		this->coreId = coreId;
	}

	unsigned getCoreId(){
		return this->coreId;
	}

    virtual ~DistributedProcessorAggregator(){
    	for(unsigned i = 0 ; i < requestObjs.size() ; ++i){
    		if(requestObjs.at(i) != NULL){
    			delete requestObjs.at(i);
    		}
    	}
    };

    void addRequestObj(Request * requestObj){
    	this->requestObjs.push_back(requestObj);
    }
    vector<Request *> getRequestObjs(){
    	return this->requestObjs;
    }

private:
    unsigned coreId;
    vector<Request *> requestObjs;
};

}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_
