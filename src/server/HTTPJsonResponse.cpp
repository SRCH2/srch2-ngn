#include "HTTPJsonResponse.h"
#include "sstream"
#include "wrapper/URLParser.h"
#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/CustomizableJsonWriter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "util/Assert.h"
#include "util/Logger.h"

using namespace srch2::instantsearch;

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

// The below functions are the helper functions to format the HTTP response
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

void response_to_invalid_request2 (evhttp_request *req, Json::Value &response){
    response["error"] = HTTP_INVALID_REQUEST_MESSAGE;
    bmhelper_evhttp_send_reply2(req, HTTP_BADREQUEST, "INVALID REQUEST", global_customized_writer.write(response));
    srch2::util::Logger::error(HTTP_INVALID_REQUEST_MESSAGE);
}

// This helper function is to wrap a Json::Value into a Json::Array and then return the later object.
Json::Value wrap_with_json_array2(Json::Value value){
    Json::Value array(Json::arrayValue);
    array.append(value);
    return array;
}


Json::Value HTTPJsonResponse::getJsonSingleMessage(const HTTPJsonMessageCode code){
	Json::Value msgValue(Json::objectValue);
	msgValue[c_message_code] = (unsigned) (code);
	switch (code) {
	case HTTP_JSON_Parse_Error:
	case HTTP_JSON_All_Shards_Down_Error:
	case HTTP_JSON_PK_Exists_Error:
	case HTTP_JSON_Doc_Limit_Reached_Error:
	case HTTP_JSON_Update_Failed_Error:
	case HTTP_JSON_Recover_Failed_Error:
	case HTTP_JSON_Delete_Record_Not_Found_Error:
	case HTTP_JSON_ResetLogger_Reopen_Failed_Error:
	case HTTP_JSON_Search_Res_Format_Wrong_Error:
	case HTTP_JSON_Cluster_Not_Ready_Error:
		msgValue[c_error] = getJsonSingleMessageStr(code);
		break;
	case HTTP_JSON_Existing_Record_Update_Info:
	case HTTP_JSON_Merge_Already_Done_Info:
	case HTTP_JSON_Commit_Already_Done_Info:
		msgValue[c_info] = getJsonSingleMessageStr(code);
		break;
	case HTTP_JSON_Node_Timeout_Warning:
		msgValue[c_warning] = getJsonSingleMessageStr(code);
		break;
	default:
		ASSERT(false);
		break;
	}
	return msgValue;
}

const string HTTPJsonResponse::getJsonSingleMessageStr(const HTTPJsonMessageCode code){
	switch (code) {
	case HTTP_JSON_Parse_Error:
		return "JSON object parse error";
	case HTTP_JSON_All_Shards_Down_Error:
		return "No data shard is available for this request.";
	case HTTP_JSON_Node_Timeout_Warning:
		return "Node timeout.";
	case HTTP_JSON_PK_Exists_Error:
		return "The record with same primary key already exists";
	case HTTP_JSON_Doc_Limit_Reached_Error:
		return "Document limit reached. Email support@srch2.com for account upgrade.";
	case HTTP_JSON_Existing_Record_Update_Info:
		return "Existing record updated successfully.";
	case HTTP_JSON_Update_Failed_Error:
		return "Record update failed.";
	case HTTP_JSON_Recover_Failed_Error:
		return "Existing record with the same primary key deleted but failed to recover.";
	case HTTP_JSON_Delete_Record_Not_Found_Error:
		return "No record with given primary key";
	case HTTP_JSON_ResetLogger_Reopen_Failed_Error:
		return "The logger file repointing failed. Could not create new logger file";
	case HTTP_JSON_PK_NOT_PROVIDED:
		return "Primary key not provided.";
	case HTTP_JSON_Search_Res_Format_Wrong_Error:
		return "The indexed data failed to export to disk, The request need to set search-response-format to be 0 or 2";
	case HTTP_JSON_Merge_Already_Done_Info:
		return "No records require merge at this time. Merge is up-to-date.";
	case HTTP_JSON_Commit_Already_Done_Info:
		return "Commit is finished. No need to commit anymore.";
	case HTTP_JSON_Cluster_Not_Ready_Error:
		return "Cluster is not ready to accept this request now, please try again later.";
	default:
		ASSERT(false);
		return "Unknown error.";
	}
}

void HTTPJsonResponse::appendDetails(Json::Value & destinationRoot, const Json::Value & sourceRoot, const char * destListRootName ){
	if(sourceRoot == nullJsonValue || destinationRoot == nullJsonValue ||
			sourceRoot.type() != Json::arrayValue ){
		ASSERT(false);
		return;
	}
	if(destinationRoot.get(destListRootName, nullJsonValue) == nullJsonValue){
		destinationRoot[destListRootName] = Json::Value(Json::arrayValue);
	}
	if(destinationRoot[destListRootName].type() != Json::arrayValue){
		ASSERT(false);
		return;
	}
	for(int i = 0 ; i < sourceRoot.size(); ++i){
		destinationRoot[destListRootName].append(sourceRoot[i]);
	}
}

void HTTPJsonResponse::mergeMessageLists(Json::Value & destinationList, const Json::Value & sourceList){

	if(sourceList == nullJsonValue){
		ASSERT(false);
		return;
	}

	if(destinationList == nullJsonValue){
		destinationList = Json::Value(Json::arrayValue);
	}

	int recordDetailsInitialSize = destinationList.size();
	for(int j = 0; j < sourceList.size(); ++j){
		bool exists = false;
		for(int i = 0 ; i < recordDetailsInitialSize; ++i){
			if(destinationList[i].get(c_message_code,nullJsonValue) == nullJsonValue ||
					sourceList[j].get(c_message_code,nullJsonValue) == nullJsonValue){
				continue;
			}
			if(destinationList[i][c_message_code].asUInt() == sourceList[j][c_message_code].asUInt() ){
				if((HTTPJsonMessageCode)(destinationList[i][c_message_code].asUInt()) == HTTP_JSON_Custom_Error){
					if(destinationList[i][c_error].asString().compare(sourceList[j][c_error].asString()) != 0){
						continue;
					}
				}

				exists = true;
				j = sourceList.size(); // to break the outer loop
				break;
			}
		}
		if(! exists){
			destinationList.append(sourceList[j]);
		}
	}
}

HTTPJsonResponse::HTTPJsonResponse(evhttp_request* req) :
		jsonResponse(Json::objectValue) {
	ASSERT(req != NULL);
	this->req = req;
	this->code = HTTP_OK;
	this->reason = NULL;
	this->headers = NULL;
}

HTTPJsonResponse::~HTTPJsonResponse() {
	if (this->headers != NULL) {
		bmhelper_evhttp_send_reply2(req, code, reason,
				global_customized_writer.write(getRoot()), *headers);
	} else {
		bmhelper_evhttp_send_reply2(req, code, reason,
				global_customized_writer.write(getRoot()));
	}
}

void HTTPJsonResponse::finalizeInvalid() {
	this->getRoot()[JSON_ERROR] = HTTP_INVALID_REQUEST_MESSAGE;
	this->code = HTTP_BADREQUEST;
	this->reason = "Invalid Request";
	srch2::util::Logger::error (HTTP_INVALID_REQUEST_MESSAGE);
}

void HTTPJsonResponse::finalizeError(const string& msg, int code) {
	if (msg.compare("") != 0) {
		this->getRoot()[JSON_ERROR] = msg;
	}
	this->code = code;
	switch (this->code) {
	case HTTP_BADREQUEST:
		this->reason = "Invalid Request";
		break;
	case HTTP_NOCONTENT:
		this->reason = "No Content";
		break;
	case HTTP_NOTFOUND:
		this->reason = "Not Found";
		break;
	case HTTP_BADMETHOD:
		this->reason = "Bad Method";
		break;
	default:
		this->reason = "Unknown";
		break;
	}
	srch2::util::Logger::error("Error : %s", msg.c_str());
}

void HTTPJsonResponse::finalizeOK(const string& details) {
	if (details.compare("") != 0) {
		this->getRoot()[JSON_DETAILS] = details;
	}
	this->code = HTTP_OK;
	this->reason = "OK";
}

void HTTPJsonResponse::addWarning(const string& warnings) {
	if (warnings.compare("") == 0) {
		return;
	}
	addWarning(Json::Value(warnings));
}

void HTTPJsonResponse::addWarning(const Json::Value& warningNode) {
	if (warningNode == nullJsonValue) {
		return;
	}
	if (this->getRoot().get(JSON_WARNING, nullJsonValue) == nullJsonValue) {
		this->getRoot()[JSON_WARNING] = Json::Value(Json::arrayValue);
	}
	this->getRoot()[JSON_WARNING].append(warningNode);
}

void HTTPJsonResponse::addError(const string & error){
	if (error.compare("") == 0) {
		return;
	}
	addError(Json::Value(error));
}
void HTTPJsonResponse::addError(const Json::Value & errorNode){
	if (errorNode == nullJsonValue) {
		return;
	}
	if (this->getRoot().get(JSON_ERROR, nullJsonValue) == nullJsonValue) {
		this->getRoot()[JSON_ERROR] = Json::Value(Json::arrayValue);
	}
	this->getRoot()[JSON_ERROR].append(errorNode);
}

void HTTPJsonResponse::addMessage(const string& msg) {
	if (msg.compare("") == 0) {
		return;
	}
	if (this->getRoot().get(JSON_MESSAGE, nullJsonValue) == nullJsonValue) {
		this->getRoot()[JSON_MESSAGE] = Json::Value(Json::arrayValue);
	}
	this->getRoot()[JSON_MESSAGE].append(Json::Value(msg));
}

void HTTPJsonResponse::setHeaders(evkeyvalq* headers) {
	this->headers = headers;
}

Json::Value& HTTPJsonResponse::getRoot() {
	return this->jsonResponse;
}

const char * HTTPJsonRecordOperationResponse::c_items = "items";

Json::Value HTTPJsonRecordOperationResponse::getRecordJsonResponse(
		const string& primaryKey, const char* action, bool status,
		const string& coreName) {
	Json::Value res(Json::objectValue);
	res[c_rid] = primaryKey;
	res[c_action] = action;
	res[c_status] = status;
	res[c_core_name] = coreName;
	return res;
}

void HTTPJsonRecordOperationResponse::addRecordError(Json::Value& recordRoot,
		const HTTPJsonMessageCode code, const string& message) {
	if (recordRoot.get(c_detail, nullJsonValue) == nullJsonValue) {
		recordRoot[c_detail] = Json::Value(Json::arrayValue);
	}
	if (code == HTTP_JSON_Custom_Error) {
		Json::Value errValue(Json::objectValue);
		errValue[c_message_code] = (unsigned) (code);
		errValue[c_error] = message;
		recordRoot[c_detail].append(errValue);
	}else{
		recordRoot[c_detail].append(HTTPJsonResponse::getJsonSingleMessage(code));
	}
}

void HTTPJsonRecordOperationResponse::addRecordShardResponse(Json::Value recordShardResponse){

	// first, extract new information
	Json::Value pkValue = recordShardResponse.get(c_rid, nullJsonValue);
	if(pkValue == nullJsonValue){
		ASSERT(false);
		return;
	}
	const string primaryKey = pkValue.asString();

	Json::Value actionValue = recordShardResponse.get(c_action, nullJsonValue);
	if(actionValue == nullJsonValue){
		ASSERT(false);
		return;
	}
	const string action = actionValue.asString();

	Json::Value statusValue = recordShardResponse.get(c_status, nullJsonValue);
	if(statusValue == nullJsonValue){
		ASSERT(false);
		return;
	}
	const bool status = statusValue.asBool();

	Json::Value coreNameValue = recordShardResponse.get(c_core_name, nullJsonValue);
	if(coreNameValue == nullJsonValue){
		ASSERT(false);
		return;
	}
	const string coreName = coreNameValue.asString();


	Json::Value newRecordDetails = recordShardResponse.get(c_detail, nullJsonValue);

	Json::Value & recordRoot = findItemRoot(primaryKey, action);

	Json::Value & root = getRoot();

	if(recordRoot == nullJsonValue){
		if(newRecordDetails == nullJsonValue){
			recordShardResponse[c_detail] = Json::Value(Json::arrayValue);
		}
		root.append(recordShardResponse);
		return;
	}

	// aggregate details messages
	if(newRecordDetails != nullJsonValue){
		HTTPJsonResponse::mergeMessageLists(recordRoot[c_detail], newRecordDetails);
	}


	// rewrite the core name
	recordRoot[c_core_name] = coreName;

	// update status
	recordRoot[c_status] = recordRoot[c_status].asBool() && status;

}


Json::Value & HTTPJsonRecordOperationResponse::getRoot(){
	return HTTPJsonResponse::getRoot()[c_items];
}

Json::Value & HTTPJsonRecordOperationResponse::findItemRoot(const string & primaryKey, const string & action){

	Json::Value & root = getRoot();

	for(int i = 0 ; i < root.size(); ++i){
		if(root[i][c_rid].compare(primaryKey) == 0 &&
				root[i][c_action].compare(action) == 0 ){
			return root[i];
		}
	}
	return nullJsonValue;
}


void HTTPJsonShardOperationResponse::addShardResponse(const char * action, const bool status, const Json::Value & shardMessages) {

	if(getRoot().get(c_action, nullJsonValue) == nullJsonValue){
		getRoot()[c_action] = Json::Value(action);
	}else{
		ASSERT(getRoot()[c_action].asString().compare(string(action)) == 0);
	}

	if(getRoot().get(c_status,nullJsonValue) == nullJsonValue){
		getRoot()[c_status] = Json::Value(status);
	}else{
		getRoot()[c_status] = getRoot()[c_status].asBool() && status;
	}

	if(shardMessages == nullJsonValue){
		return;
	}
	if(getRoot().get(c_detail, nullJsonValue) == nullJsonValue){
		getRoot()[c_detail] = shardMessages;
	}else{
		HTTPJsonResponse::mergeMessageLists(getRoot()[c_detail],shardMessages);
	}
}

Json::Value & HTTPJsonShardOperationResponse::getRoot(){
	return HTTPJsonResponse::getRoot();
}

}
}
