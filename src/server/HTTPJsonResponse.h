#ifndef __SERVER_HTTP_JSON_RESPONSE_H_
#define __SERVER_HTTP_JSON_RESPONSE_H_

#include "./util/CustomizableJsonWriter.h"
#include "./HTTPJsonResponseConstants.h"
#include "wrapper/WrapperConstants.h"
#include "core/operation/IndexHealthInfo.h"
#include "string"
#include <event.h>
#include <evhttp.h>
#include <event2/http.h>
using namespace std;

namespace srch2 {
namespace httpwrapper {

class CoreInfo_t;

/**
 * Create evbuffer. If failed, send 503 response.
 * @param req request
 * @return buffer
 */
evbuffer *create_buffer2(evhttp_request *req) ;

// The below functions are the helper functions to format the HTTP response
void bmhelper_check_add_callback2(evbuffer *buf, const evkeyvalq &headers,
        const string &out_payload) ;

void bmhelper_add_content_length2(evhttp_request *req, evbuffer *buf) ;

void bmhelper_evhttp_send_reply2(evhttp_request *req, int code,
        const char *reason, const string &out_payload,
        const evkeyvalq &headers) ;

void bmhelper_evhttp_send_reply2(evhttp_request *req, int code,
        const char *reason, const string &out_payload) ;

void response_to_invalid_request2 (evhttp_request *req, Json::Value &response);

// This helper function is to wrap a Json::Value into a Json::Array and then return the later object.
Json::Value wrap_with_json_array2(Json::Value value);


class HTTPJsonResponse{
public:

	static Json::Value getJsonSingleMessage(const HTTPJsonMessageCode code);
	static const string getJsonSingleMessageStr(const HTTPJsonMessageCode code);
	static void mergeMessageLists(Json::Value & destinationList, const Json::Value & sourceList);

	static void appendDetails(Json::Value & destinationRoot, const Json::Value & sourceRoot, const char * destListRootName = c_detail);

	HTTPJsonResponse(evhttp_request *req);

	virtual ~HTTPJsonResponse();

	void setResponseAttribute(const char * attributeName, const Json::Value & resAttr);

	void finalizeInvalid();

	void finalizeError(const string & msg = "", int code = HTTP_BADREQUEST);

	void finalizeOK(const string & details = "");

	void addWarning(const string & warnings);
	void addWarning(const Json::Value & warningNode);

	void addError(const string & error);
	void addError(const Json::Value & errorNode);

	void addMessage(const string & msg);

	void setHeaders(evkeyvalq * headers);

protected:
	evhttp_request *req;

	// HTTP reply properties
	int code;
    char *reason;
    Json::Value jsonResponse;
    evkeyvalq * headers;

	virtual Json::Value & getRoot();

};

class HTTPJsonRecordOperationResponse : public HTTPJsonResponse{
public:
	static const char * c_items;


public:
	static Json::Value getRecordJsonResponse(const string & primaryKey, const char * action, bool status, const string & coreName);
	static void addRecordError(Json::Value & responseRoot, const HTTPJsonMessageCode code, const string & message = "");

	HTTPJsonRecordOperationResponse(evhttp_request *req):HTTPJsonResponse(req){
		HTTPJsonResponse::getRoot()[c_items] = Json::Value(Json::arrayValue);
	};
	virtual ~HTTPJsonRecordOperationResponse(){};

	virtual Json::Value & getRoot();

	void addRecordShardResponse(Json::Value recordShardResponse);

private:

    Json::Value & findItemRoot(const string & primaryKey, const string & action);
};


class HTTPJsonShardOperationResponse : public HTTPJsonResponse{
public:

	HTTPJsonShardOperationResponse(evhttp_request *req):HTTPJsonResponse(req){};
	virtual ~HTTPJsonShardOperationResponse(){};

	virtual Json::Value & getRoot();


	// shardMessages is [{"error":"...",...},{"error":"...",...},{"error":"...",...}]
	void addShardResponse(const char * action, const bool status, const Json::Value & shardMessages);

private:

};


class HTTPJsonGetInfoResponse : public HTTPJsonResponse{
public:

	HTTPJsonGetInfoResponse(evhttp_request *req):HTTPJsonResponse(req){};
	virtual ~HTTPJsonGetInfoResponse(){};

	virtual Json::Value & getRoot();

	Json::Value & getCoresRoot();

	void addCoreInfo(const CoreInfo_t * coreInfo,
			const srch2::instantsearch::IndexHealthInfo & info,
			const vector<std::pair< string , srch2::instantsearch::IndexHealthInfo> > & shardsInfo,
			const vector<srch2::instantsearch::IndexHealthInfo> & partitionsInfo,
			const vector<std::pair< string , srch2::instantsearch::IndexHealthInfo> > & nodeShardsInfo);

private:

};

}
}

#endif // __SERVER_HTTP_JSON_RESPONSE_H_
