#ifndef __SERVER_HTTP_JSON_RESPONSE_H_
#define __SERVER_HTTP_JSON_RESPONSE_H_

#include "./util/CustomizableJsonWriter.h"
#include "./HTTPJsonResponseConstants.h"
#include "wrapper/WrapperConstants.h"
#include "core/operation/IndexHealthInfo.h"
#include "sharding/processor/serializables/SerializableGetInfoResults.h"
#include "sharding/transactions/Transaction.h"
#include "sharding/transactions/TransactionSession.h"
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


class JsonResponseHandler{
public:

	static Json::Value getJsonSingleMessage(const JsonMessageCode code);
	static const string getJsonSingleMessageStr(const JsonMessageCode code);
	static const string getCustomMessageStr(const JsonMessageCode code, const string variablePart);
	static void mergeMessageLists(Json::Value & destinationList, const Json::Value & sourceList);

	static void appendDetails(Json::Value & destinationRoot, const Json::Value & sourceRoot, const char * destListRootName = c_detail);

	JsonResponseHandler(evhttp_request *req);
	JsonResponseHandler();
	virtual ~JsonResponseHandler(){
//		printHTTP(req, headers);
		if(jsonResponse != NULL){
			delete jsonResponse;
		}
	};
	void printHTTP(evhttp_request *req = NULL, evkeyvalq * headers = NULL);

	void setResponseAttribute(const char * attributeName, const Json::Value & resAttr);

	// puts the current jsonResponse in the vector and sets it to this new root
	void setRoot(Json::Value * root);

	void finalizeInvalid();

	void finalizeError(const string & msg = "", int code = HTTP_BADREQUEST);

	void finalizeOK(const string & details = "");

	void addWarning(const string & warnings);
	void addWarning(const Json::Value & warningNode);

	void addError(const string & error);
	void addError(const Json::Value & errorNode);

	void addMessage(const string & msg);

	void setHeaders(evkeyvalq * headers){
		this->headers = headers;
	}

	bool hasMultiRoots(vector<const Json::Value *> & roots){
		if(multiRoots.size() > 0 && jsonResponse != NULL){
			roots.insert(roots.begin(), multiRoots.begin(), multiRoots.end());
			roots.push_back(jsonResponse);
			return true;
		}
		return false;
	}
	virtual Json::Value & getRoot();

protected:
	// HTTP reply properties
	int code;
    const char *reason;
    Json::Value * jsonResponse;

    // in the case that we have more than one root and all of them
    // are going to be direct children of actual printed core (array).
    std::vector<Json::Value *> multiRoots;

    evhttp_request *req;
    evkeyvalq * headers;


};

class JsonRecordOperationResponse : public JsonResponseHandler{
public:
	static const char * c_items;


public:
	JsonRecordOperationResponse(evhttp_request *req):JsonResponseHandler(req){};
	JsonRecordOperationResponse():JsonResponseHandler(){
		(*jsonResponse)[c_items] = Json::Value(Json::arrayValue);
	};
	static Json::Value getRecordJsonResponse(const string & primaryKey, const char * action, bool status, const string & coreName);
	static void addRecordError(Json::Value & responseRoot, const JsonMessageCode code, const string & message = "");
	static void addRecordMessage(Json::Value & responseRoot, const Json::Value & msgObj);
	static void addRecordMessages(Json::Value & responseRoot, const vector<JsonMessageCode> & msgCode);

	virtual ~JsonRecordOperationResponse(){};

	virtual Json::Value & getRoot();

	void addRecordShardResponse(Json::Value recordShardResponse);

private:

    Json::Value & findItemRoot(const string & primaryKey, const string & action);
};


class ShardOperationJsonResponse : public JsonResponseHandler{
public:

	ShardOperationJsonResponse(evhttp_request *req):JsonResponseHandler(req){};
	ShardOperationJsonResponse(){};
	virtual Json::Value & getRoot();


	// shardMessages is [{"error":"...",...},{"error":"...",...},{"error":"...",...}]
	void addShardResponse(const char * action, const bool status, const Json::Value & shardMessages);

private:

};


class GetInfoJsonResponse : public JsonResponseHandler{
public:

	GetInfoJsonResponse(evhttp_request *req):JsonResponseHandler(req){};
	virtual Json::Value & getRoot();

	Json::Value & getCoresRoot();
	Json::Value & getNodesRoot();
	Json::Value & getNodeShardsRoot();

	void addCoreInfo(const CoreInfo_t * coreInfo,
			const srch2::instantsearch::IndexHealthInfo & info,
			const vector<std::pair<GetInfoCommandResults::ShardResults * , srch2::instantsearch::IndexHealthInfo > > & shardsInfo,
			const vector<std::pair<GetInfoCommandResults::ShardResults * , srch2::instantsearch::IndexHealthInfo > > & partitionsInfo,
			const vector<std::pair<GetInfoCommandResults::ShardResults * , srch2::instantsearch::IndexHealthInfo > > & nodeShardsInfo,
			const vector<std::pair<GetInfoCommandResults::ShardResults * , srch2::instantsearch::IndexHealthInfo > > & allShardResults,
			bool debugRequest = false);

	void addShardResultGroup(Json::Value & coreInfoJsonRoot , bool debugRequest,
			const vector<std::pair<GetInfoCommandResults::ShardResults * , srch2::instantsearch::IndexHealthInfo > > & nodeShardsInfo);

private:

};

}
}

#endif // __SERVER_HTTP_JSON_RESPONSE_H_
