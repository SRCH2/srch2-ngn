#ifndef __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_

#include "sharding/configuration/ConfigManager.h"

#include <instantsearch/Record.h>
#include "wrapper/ParsedParameterContainer.h"
#include "wrapper/URLParser.h"

#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/Logger.h"
#include "util/CustomizableJsonWriter.h"

#include "core/highlighter/Highlighter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

/**
 * Create evbuffer. If failed, send 503 response.
 * @param req request
 * @return buffer
 */
evbuffer *create_buffer2(evhttp_request *req) ;

/*
 * TODO : the digit 2 in front of these names is only for linking.
 * The reason is we have these functions in HTTPRequestHandler and we can't remove them from there now ...
 */
void bmhelper_check_add_callback2(evbuffer *buf, const evkeyvalq &headers,
        const string &out_payload) ;


void bmhelper_add_content_length2(evhttp_request *req, evbuffer *buf) ;

void bmhelper_evhttp_send_reply2(evhttp_request *req, int code,
        const char *reason, const string &out_payload,
        const evkeyvalq &headers) ;

void bmhelper_evhttp_send_reply2(evhttp_request *req, int code,
        const char *reason, const string &out_payload) ;

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


	virtual ~ResultAggregatorAndPrint(){};

};

}
}


#endif // __SHARDING_PROCESSOR_RESULTS_AGGREGATOR_AND_PRINT_H_
