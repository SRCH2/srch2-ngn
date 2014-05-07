#ifndef __PROCESSOR_UTIL_H_
#define __PROCESSOR_UTIL_H_

#include "string"
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>

using namespace std;

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

}
}

#endif // __PROCESSOR_UTIL_H_
