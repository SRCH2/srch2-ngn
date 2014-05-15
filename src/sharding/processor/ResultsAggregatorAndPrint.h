#ifndef __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_


#include "sharding/configuration/ConfigManager.h"

#include <instantsearch/Record.h>
#include "wrapper/ParsedParameterContainer.h"
#include "util/CustomizableJsonWriter.h"
#include "util/Logger.h"

#include "core/highlighter/Highlighter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "sharding/processor/ProcessorUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

struct ResultsAggregatorAndPrintMetadata{

};

template <class Request, class Response>
class ResultAggregatorAndPrint {
public:


	/*
	 * This function is always called by RoutingManager as the first call back function
	 */
	virtual void preProcessing(ResultsAggregatorAndPrintMetadata metadata){};
	/*
	 * This function is called by RoutingManager if a timeout happens, The call to
	 * this function must be between preProcessing(...) and callBack()
	 */
	virtual void timeoutProcessing(ShardId * shardInfo,
			Request * sentRequest, ResultsAggregatorAndPrintMetadata metadata){};

	/*
	 * The callBack function used by routing manager
	 */
	virtual void callBack(Response * responseObject){};
	virtual void callBack(vector<Response *> responseObjects){};

	/*
	 * The last call back function called by RoutingManager in all cases.
	 * Example of call back call order for search :
	 * 1. preProcessing()
	 * 2. timeoutProcessing() [only if some shard times out]
	 * 3. aggregateSearchResults()
	 * 4. finalize()
	 */
	virtual void finalize(ResultsAggregatorAndPrintMetadata metadata){};

	void addRequestObject(Request * req){
		// request objects are added sequentially so we don't use lock
		// in fact, all request objects are added in the beginning and by one single thread
		ASSERT(req != NULL);
		if(req != NULL){
			this->requestObjects.push_back(req);
		}
	};
	void addResponseObject(Response * responseObject){
		boost::unique_lock< boost::shared_mutex > lock(_access);
		ASSERT(responseObject != NULL);
		if(responseObject != NULL){
			responseObjects.push_back(responseObject);
		}
	};
	void addResponseObjects(vector<Response *> responseObjects){
		boost::unique_lock< boost::shared_mutex > lock(_access);
		this->responseObjects.insert(this->responseObjects.end(), responseObjects.begin(), responseObjects.end());
	};

	unsigned getNumberOfRequests(){
		boost::unique_lock< boost::shared_mutex > lock(_access);
		return this->requestObjects.size();
	}

	virtual ~ResultAggregatorAndPrint(){
		// delete request objects
		for(typename vector<Request *>::iterator reqIter = requestObjects.begin();
				reqIter != requestObjects.end(); ++reqIter){
			ASSERT(*reqIter != NULL);
			if(*reqIter != NULL){
				delete *reqIter;
			}
		}
		// delete response objects
		for(typename vector<Response *>::iterator resIter = responseObjects.begin();
				resIter != responseObjects.end(); ++resIter){
			ASSERT(*resIter != NULL);
			if(*resIter != NULL){
				delete *resIter;
			}
		}
	};

private:
	// we need lock because multiple threads can access responseObjects together
	mutable boost::shared_mutex _access;
	vector<Request *> requestObjects;
	vector<Response *> responseObjects;
};

}
}


#endif // __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
