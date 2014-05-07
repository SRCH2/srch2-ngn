#include "ProcessorUtil.h"
#include "sstream"
#include "wrapper/URLParser.h"
#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/CustomizableJsonWriter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

namespace srch2 {
namespace httpwrapper {

/**
 * Create evbuffer. If failed, send 503 response.
 * @param req request
 * @return buffer
 */
evbuffer *create_buffer2(evhttp_request *req) {
    evbuffer *buf = evbuffer_new();
    if (!buf) {
        fprintf(stderr, "Failed to create response buffer\n");
        evhttp_send_reply(req, HTTP_SERVUNAVAIL,
                "Failed to create response buffer", NULL);
        return NULL;
    }
    return buf;
}

void bmhelper_check_add_callback2(evbuffer *buf, const evkeyvalq &headers,
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


void bmhelper_add_content_length2(evhttp_request *req, evbuffer *buf) {
    size_t length = EVBUFFER_LENGTH(buf);
    std::stringstream length_str;
    length_str << length;
    evhttp_add_header(req->output_headers, "Content-Length",
            length_str.str().c_str());
}

void bmhelper_evhttp_send_reply2(evhttp_request *req, int code,
        const char *reason, const string &out_payload,
        const evkeyvalq &headers) {
    evbuffer *returnbuffer = create_buffer2(req);
    bmhelper_check_add_callback2(returnbuffer, headers, out_payload);
    bmhelper_add_content_length2(req, returnbuffer);
    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}

void bmhelper_evhttp_send_reply2(evhttp_request *req, int code,
        const char *reason, const string &out_payload) {
    evbuffer *returnbuffer = create_buffer2(req);

    evbuffer_add_printf(returnbuffer, "%s", out_payload.c_str());
    bmhelper_add_content_length2(req, returnbuffer);

    evhttp_send_reply(req, code, reason, returnbuffer);
    evbuffer_free(returnbuffer);
}


}
}
