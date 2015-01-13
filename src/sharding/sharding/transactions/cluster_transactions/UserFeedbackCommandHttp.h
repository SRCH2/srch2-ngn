/*
 * UserFeedbackCommandHttp.h
 *
 *  Created on: Jan 12, 2015
 *      Author: srch2
 */

#ifndef __SHARDING_USERFEEDBACKCOMMANDHTTP_H__
#define __SHARDING_USERFEEDBACKCOMMANDHTTP_H__

#include "../../state_machine/State.h"
#include "../../state_machine/StateMachine.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"

#include "../../state_machine/node_iterators/ConcurrentNotifOperation.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 *  HTTP handler transaction for user feedback.
 */
class UserFeedbackHttpHandler: public ReadviewTransaction, public NodeIteratorListenerInterface {
public:

    static void runCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId){

        SP(UserFeedbackHttpHandler) feedbackHttpHandler =
        		SP(UserFeedbackHttpHandler)(new UserFeedbackHttpHandler(clusterReadview, req, coreId)); //
        Transaction::startTransaction(feedbackHttpHandler);
        return ;
    }

    ~UserFeedbackHttpHandler(){
    	finalize();
        delete userFeedbackCommand;
        delete req;
    }


private:

    UserFeedbackHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId ):ReadviewTransaction(clusterReadview){
    	this->req = req;
    	this->coreInfo = clusterReadview->getCore(coreId);
        userFeedbackCommand = NULL;
    }

    // API to process single feedback update request.
    // feedback request format: {query = "" , recordId = "" }
    bool processSingleFeedback(const Json::Value& doc,
    		std::map< string, vector<string> > * dataForApiLayer,
    		Json::Value& feedbackResponse, string& queryId) {

    	Json::Value query = doc.get("query", Json::Value());
    	Json::Value recordId = doc.get("recordId", Json::Value());

    	if (query.type()  != Json::stringValue) {
    		std::stringstream log_str;
    		log_str << "API : feedback, Error: 'query' key is missing in request JSON.";
    		feedbackResponse = log_str.str();
    		return false;
    	}
    	string queryStringTemp = query.asString();
    	boost::algorithm::trim(queryStringTemp);
    	if (queryStringTemp.size() == 0) {
    		std::stringstream log_str;
    		log_str << "API : feedback, Error: 'query' key is empty in request JSON.";
    		feedbackResponse = log_str.str();
    		return false;
    	}

    	if (queryStringTemp.size() > srch2is::Trie::TRIE_MAX_DEPTH) {
    		std::stringstream log_str;
    		log_str << "API : feedback, Error: feedback query longer than " <<
    				Trie::TRIE_MAX_DEPTH << " is not allowed";
    		feedbackResponse = log_str.str();
    		return false;
    	}

    	// cleanup user provided query before inserting into queryIndex
    	evkeyvalq dummyHeader;
    	QueryParser qp(dummyHeader);
    	// convert encoded query to normal query . e.g  "trip%20advisor" -> "trip advisor"
    	decodeString(queryStringTemp.c_str(), qp.originalQueryString);
    	// remove local parameter , fuzzy, boost etc.
    	string queryString = qp.fetchCleanQueryString();

    	if (recordId.type() != Json::stringValue) {
    		std::stringstream log_str;
    		log_str << "API : feedback, Error: 'recordId' key is missing in request JSON.";
    		feedbackResponse = log_str.str();
    		return false;
    	}
    	string recordIdString = recordId.asString();
    	boost::algorithm::trim(recordIdString);
    	if (recordIdString.size() == 0) {
    		std::stringstream log_str;
    		log_str << "API : feedback, Error: 'recordId' key is empty in request JSON.";
    		feedbackResponse = log_str.str();
    		return false;
    	}

    	const std::map< string, vector<string> >::iterator iter = dataForApiLayer->find(recordIdString);
    	if (iter == dataForApiLayer->end()) {
    		vector<string> queryStringsArr;
    		queryStringsArr.push_back(queryString);
    		dataForApiLayer->insert(make_pair(recordIdString, queryStringsArr));
    	} else {
    		iter->second.push_back(queryString);
    	}

    	queryId = recordIdString;

    	//      THIS WILL GO TO INTENRAL/...
//    	Json::Value timestamp = doc.get("timestamp", Json::Value());
//    	unsigned secondSinceEpoch;
//    	if (timestamp.type() == Json::stringValue) {
//    		string timestampString = timestamp.asString();
//    		boost::algorithm::trim(timestampString);
//    		bool valid = srch2is::DateAndTimeHandler::verifyDateTimeString(timestampString,
//    				srch2is::DateTimeTypePointOfTime);
//    		if (!valid) {
//    			std::stringstream log_str;
//    			log_str << "API : feedback, Error: 'timestamp' key is invalid in request JSON.";
//    			feedbackResponse = log_str.str();
//    			return false;
//    		}
//    		secondSinceEpoch = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(timestampString);
//    	} else {
//    		secondSinceEpoch = time(NULL);
//    	}
//    	INDEXLOOKUP_RETVAL retVal = server->indexer->lookupRecord(recordIdString);
//    	if (retVal != LU_PRESENT_IN_READVIEW_AND_WRITEVIEW) {
//    		std::stringstream log_str;
//    		log_str << "API : feedback, Error: 'recordId' key is invalid in request JSON.";
//    		feedbackResponse = log_str.str();
//    		return false;
//    	}
//    	server->indexer->getFeedbackIndexer()->addFeedback(queryString, recordIdString, secondSinceEpoch);
    	return true;
    }

	void initSession(){
		this->setSession(new TransactionSession());
		this->getSession()->response = new JsonRecordOperationResponse();
	}

    /*
     * The main work of AclCommandHttpHandler starts in this function
     * Example of this work : parsing req object and get HTTP req information.
     *
     */
    void run(){
    	JsonResponseHandler * responseObject = this->getTransaction()->getSession()->response;
    	if(coreInfo == NULL){
    		responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Core_Does_Not_Exist));
    		responseObject->finalizeOK();
    		return;
    	}

    	// map of record_id -> [query1, query2 ...]
    	std::map< string, vector<string> > * feedbackDataForApiLayer =
    			new std::map< string, vector<string> >();


    	if (!coreInfo->isUserFeedbackEnabled()){
    		Logger::console("User feedback is NOT enabled in the config file.");
    		responseObject->addError(string("User feedback is NOT enabled in the config file."));
    		responseObject->finalizeOK();
    		return;
    	}

    	Json::Value response(Json::objectValue);
    	const char *feedbackData = NULL;

    	switch (req->type) {
    	    case EVHTTP_REQ_PUT:
    	    {
    	        size_t length = EVBUFFER_LENGTH(req->input_buffer);

    	        if (length == 0) {
    	        	responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Empty_Body));
    	        	responseObject->finalizeOK();
    	        	return;
    	        }

    	        // get input JSON
    	        feedbackData = (char *) EVBUFFER_DATA(req->input_buffer);

    	        break;
    	    }
    	    case EVHTTP_REQ_GET:
    	    {
    	    	evkeyvalq headers;
    	    	evhttp_parse_query(req->uri, &headers);
    	    	feedbackData = evhttp_find_header(&headers, "data");
    	    	if (feedbackData == NULL) {
    	    		responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Empty_Body));
    	    		responseObject->finalizeOK();
    	    		return;
    	    	}
    	    	break;
    	    }
    	    default:
    	    	this->getSession()->response->finalizeInvalid();
    	    	return;

    	}

    	if (feedbackData == NULL)
    		return;

        std::stringstream log_str;
        Json::Value root;
        Json::Reader reader;
        bool parseSuccess = reader.parse(feedbackData, root, false);
        bool error = false;
    	Json::Value feedbackResponse(Json::arrayValue);
        if (parseSuccess == false) {
        	responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Parse_Error));
        	responseObject->finalizeOK();
        	return;
        } else {
        	if (root.type() == Json::arrayValue) {
        		feedbackResponse.resize(root.size());
        		//the record parameter is an array of json objects
        		for(Json::UInt index = 0; index < root.size(); index++) {
        			Json::Value defaultValueToReturn = Json::Value("");
        			const Json::Value doc = root.get(index,
        					defaultValueToReturn);

        			HTTPPrintInfo pi = { true , "", ""};
        			bool  status = processSingleFeedback(doc, feedbackDataForApiLayer,
        					feedbackResponse[index], pi.recordId);
        			if (status == false) {
        				pi.httpLayerSuccess = false;
        				pi.httpLayerMsg = feedbackResponse[index].asString();
        			}
        			httpPrintInfoInfoArr.push_back(pi);
        		}
        	} else {
        		feedbackResponse.resize(1);
        		// the record parameter is a single json object
        		const Json::Value doc = root;
        		HTTPPrintInfo pi = { true , "", ""};
        		bool  status = processSingleFeedback(doc, feedbackDataForApiLayer,
        				feedbackResponse[0], pi.recordId);
        		if (status == false) {
        			responseObject->addError(feedbackResponse[0]);
        			responseObject->finalizeOK();
        			return;
        		} else {

        			httpPrintInfoInfoArr.push_back(pi);
        		}
        	}
        }

        // When query parameters are parsed successfully, we must create and run AclCommand class and get back
        // its response in a 'consume' callback function.
    	userFeedbackCommand = new WriteCommand(this, *feedbackDataForApiLayer, coreInfo);

    	userFeedbackCommand->produce();

        delete feedbackDataForApiLayer;
        return;
    }

    void end(map<NodeId, SP(ShardingNotification) > & replies){
    	// the input is a map from nodeId to the shardingNotification which was sent from that node to us
    	// as the reply of our request.
    	if(replies.empty()){
    		// no reply came back because all participants died before replying ...
    		// maybe we want to abort this transaction ?
    	}
    	// TODO : do whatever you want with replies ...
    }

    /*
     * One example of consume callback that will be called by the producer class
     * If another set of arguments is needed for this module, a new consume method must be
     * added to ConsumerInterface, it must be overridden here to process the results of AclCommand
     * and it must be called in finalize method of AclCommand to return back to this consumer.
     */
    void consume(const map<string, bool> & results,
			map<string, map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > > & messageCodes){
    	typedef map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > MessageCodes;

    	JsonResponseHandler * responseObject = this->getTransaction()->getSession()->response;
    	for (unsigned i = 0; i < httpPrintInfoInfoArr.size(); ++i) {

			Json::Value recordShardResponse(Json::objectValue);
			recordShardResponse[c_rid] = httpPrintInfoInfoArr[i].recordId; // temp
			recordShardResponse[c_action] = "feedback";
			recordShardResponse[c_status] = httpPrintInfoInfoArr[i].httpLayerSuccess;
			recordShardResponse[c_core_name] = coreInfo->getName();

    		if (httpPrintInfoInfoArr[i].httpLayerSuccess) {
    			const string& recordId = httpPrintInfoInfoArr[i].recordId;
    			bool success = true;
    			map<string, bool>::const_iterator iter = results.find(recordId);
    			if ( iter != results.end()) {
    				success &= iter->second;
    				if(messageCodes.find(iter->first) != messageCodes.end()){
    					MessageCodes &primaryKeyMessageCode = messageCodes[iter->first];
    					for(MessageCodes::iterator shardItr =
    							primaryKeyMessageCode.begin(); shardItr != primaryKeyMessageCode.end(); ++shardItr){
    						JsonRecordOperationResponse::addRecordMessages(recordShardResponse, shardItr->second);
    					}
    				} else { ASSERT(false); }
    			} else { ASSERT(false);}

    			recordShardResponse[c_status] = success;
    		} else {
    			JsonRecordOperationResponse::addRecordError(recordShardResponse,
    					HTTP_JSON_Custom_Error, httpPrintInfoInfoArr[i].httpLayerMsg);
    		}

    		JsonRecordOperationResponse * responseChannel = (JsonRecordOperationResponse *) this->getSession()->response;
    		responseChannel->addRecordShardResponse(recordShardResponse);
    	}
    }

    void finalizeWork(Transaction::Params * params){
		this->getTransaction()->getSession()->response->printHTTP(req);
    }

    /*
     * This function must be overridden for each transaction class so that producers can use the
     * transaction and it's getSession() inteface.
     */
    SP(Transaction) getTransaction() {
        return sharedPointer;
    }

    ShardingTransactionType getTransactionType(){
        return ShardingTransactionType_FeedbackCommandCode; // returns the unique type identifier of this transaction
    }

    string getName() const {return "user-feedback-command-http" ;};


private:
    WriteCommand * userFeedbackCommand;
    evhttp_request *req;
    const CoreInfo_t * coreInfo;

    struct HTTPPrintInfo{
    	bool httpLayerSuccess;
    	string httpLayerMsg;
    	string recordId;
    };
    vector<HTTPPrintInfo> httpPrintInfoInfoArr;
};


}
}

#endif /* __SHARDING_USERFEEDBACKCOMMANDHTTP_H__ */
