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
evbuffer *create_buffer(evhttp_request *req) {
    evbuffer *buf = evbuffer_new();
    if (!buf) {
        fprintf(stderr, "Failed to create response buffer\n");
        evhttp_send_reply(req, HTTP_SERVUNAVAIL,
                "Failed to create response buffer", NULL);
        return NULL;
    }
    return buf;
}

void bmhelper_check_add_callback(evbuffer *buf, const evkeyvalq &headers,
        const string &out_payload) {
    const char *jsonpCallBack = evhttp_find_header(&headers,
            URLParser::jsonpCallBackName);
    if (jsonpCallBack) {
        size_t sz;
        char *jsonpCallBack_cstar = evhttp_uridecode(jsonpCallBack, 0, &sz);
        //std::cout << "[" << jsonpCallBack_cstar << "]" << std::endl;

        evbuffer_add_printf(buf, "%s(%s)", jsonpCallBack_cstar,
                out_payload.c_str());

        // libevent uses malloc for memory allocation. Hence, use free
        free(jsonpCallBack_cstar);
    } else {
        evbuffer_add_printf(buf, "%s", out_payload.c_str());
    }
}


void bmhelper_add_content_length(evhttp_request *req, evbuffer *buf) {
    size_t length = EVBUFFER_LENGTH(buf);
    std::stringstream length_str;
    length_str << length;
    evhttp_add_header(req->output_headers, "Content-Length",
            length_str.str().c_str());
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code,
        const char *reason, const string &out_payload,
        const evkeyvalq &headers) {
    evbuffer *returnbuffer = create_buffer(req);
    bmhelper_check_add_callback(returnbuffer, headers, out_payload);
    bmhelper_add_content_length(req, returnbuffer);
    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

void bmhelper_evhttp_send_reply(evhttp_request *req, int code,
        const char *reason, const string &out_payload) {
    evbuffer *returnbuffer = create_buffer(req);

    evbuffer_add_printf(returnbuffer, "%s", out_payload.c_str());
    bmhelper_add_content_length(req, returnbuffer);

    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

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
