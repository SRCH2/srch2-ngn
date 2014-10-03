#ifndef __SERVER_HTTP_JSON_RESPONSE_CONSTANTS_H_
#define __SERVER_HTTP_JSON_RESPONSE_CONSTANTS_H_

#include "string"
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>
#include "server/util/CustomizableJsonWriter.h"
#include "wrapper/WrapperConstants.h"

using namespace std;

namespace srch2 {
namespace httpwrapper {

static const pair<string, string> global_internal_record("srch2_internal_record_123456789", "record");
static const pair<string, string> global_internal_snippet("srch2_internal_snippet_123456789", "snippet");
static const pair<string, string> internal_data[] = { global_internal_record, global_internal_snippet};

// The below objects are used to format the global_customized_writer.
// This global_customized_writer is created once and then can be used multiple times.
static const vector<pair<string, string> > global_internal_skip_tags(internal_data, internal_data+2);
static const ::CustomizableJsonWriter global_customized_writer (&global_internal_skip_tags);
static Json::Value nullJsonValue(Json::nullValue);

static const char * JSON_MESSAGE = "message";
static const char * JSON_ERROR = "error";
static const char * JSON_WARNING = "warning";
static const char * JSON_DETAILS = "details";
static const char * JSON_LOG= "log";

static const char * HTTP_INVALID_REQUEST_MESSAGE = "The request has an invalid or missing argument. See Srch2 API documentation for details.";

static const char* c_message_code = "message_code";
static const char* c_message = "message";
static const char* c_error = "error";
static const char* c_warning = "warning";
static const char* c_info = "info";
static const char* c_action = "action";
static const char* c_action_insert = "insert";
static const char* c_action_delete = "delete";
static const char* c_action_update = "update";
static const char* c_action_save   = "save";
static const char* c_action_export = "export";
static const char* c_action_commit = "commit";
static const char* c_action_merge = "merge";
static const char* c_action_reset_logger = "reset_logger";
static const char* c_logger_file = "logger_file";

static const char* c_rid = "rid";
static const char* c_core_name = "core_name";
static const char* c_reason = "reason";
static const char* c_detail = "details";
static const char* c_resume = "resume";

static const char* c_status = "status";
static const char* c_failed = "failed";
static const char* c_success = "success";



}
}


#endif // __SERVER_HTTP_JSON_RESPONSE_CONSTANTS_H_
