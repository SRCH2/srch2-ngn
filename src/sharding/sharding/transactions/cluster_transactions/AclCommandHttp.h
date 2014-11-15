#ifndef __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__

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
 * Saves the indices and the cluster metadata on all nodes in the cluster.
 * NOTE : this operation assumes all shards are locked in S mode
 * 1. request all nodes to save their indices
 * 2. When all nodes saved their indices, request all nodes to save their cluster metadata
 * 3. When all nodes acked metadata save, write the metadata on disk and done.
 */
class AclCommandHttpHandler: public ReadviewTransaction, public NodeIteratorListenerInterface {
public:

    static void runCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId){

        SP(AclCommandHttpHandler) aclCommandHttpHandler =
        		SP(AclCommandHttpHandler)(new AclCommandHttpHandler(clusterReadview, req, coreId)); //
        Transaction::startTransaction(aclCommandHttpHandler);
        return ;
    }

    ~AclCommandHttpHandler(){
        if(aclCommand != NULL){
            delete aclCommand;
        }
        if(req != NULL){
        	delete req;
        }
    }

    static void prepareAclDataForApiLayer(std::map< string, vector<string> > &attributeAclDataForApiLayer,
    		const vector<string> &roleIdList, const vector<string>& attributeList) {

    	for (unsigned i = 0; i < roleIdList.size(); ++i) {
    		std::map< string, vector<string> >::iterator iter =
    				attributeAclDataForApiLayer.find(roleIdList[i]);
    		if (iter != attributeAclDataForApiLayer.end()) {
    			vector<string> unionList;
    			std::set_union(iter->second.begin(), iter->second.end(), attributeList.begin(),
    					attributeList.end(), back_inserter(unionList));
    			iter->second.assign(unionList.begin(), unionList.end());
    		} else {
    			attributeAclDataForApiLayer.insert(make_pair(roleIdList[i], attributeList));
    		}
    	}
    }

private:
    AclCommandHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId ):ReadviewTransaction(clusterReadview){
    	this->req = req;
    	this->coreInfo = clusterReadview->getCore(coreId);
    	unsigned aclCoreId = this->coreInfo->getAttributeAclCoreId();
		this->aclCoreInfo = clusterReadview->getCore(aclCoreId);
    	ASSERT(this->aclCoreInfo != NULL);
    	ASSERT(this->coreInfo != NULL);
        aclCommand = NULL;
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

    	std::map< string, vector<string> > * attributeAclDataForApiLayer =
    			new std::map< string, vector<string> >();
    	switch (req->type) {
    	    case EVHTTP_REQ_PUT: {
    	        size_t length = EVBUFFER_LENGTH(req->input_buffer);

    	        if (length == 0) {
    	        	responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Empty_Body));
    	        	responseObject->finalizeOK();
    	        	return;
    	        }

    	        // Identify the type of access control request.
    	        // req->uri should be "/aclAttributeRoleDelete" or "/aclAttributeRoleAppend"
    	        // or "/aclAttributeRoleReplace" for default core
    	        // Otherwise it should be /corename/aclAttributeRoleDelete etc.
    	        string uriString = req->uri;
    	        string apiName;
    	        string corename = this->coreInfo->getName();
    	        if (corename == ConfigManager::defaultCore) {
    	        	corename.clear();
    	        } else {
    	        	corename = "/" + corename;
    	        }
    	        if (uriString == corename + "/aclAttributeRoleReplace") {
    	        	action = ACL_REPLACE;
    	        	apiName = "aclAttributeRoleReplace";
    	        }
    	        else if (uriString == corename + "/aclAttributeRoleDelete") {
    	        	apiName = "aclAttributeRoleDelete";
    	        	action = ACL_DELETE;
    	        }
    	        else if (uriString == corename + "/aclAttributeRoleAppend") {
    	        	apiName = "aclAttributeRoleAppend";
    	        	action = ACL_APPEND;
    	        }
    	        else {
    	        	responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Core_Does_Not_Exist));
    	        	responseObject->finalizeOK();
    	        	return;
    	        }
    	        // get input JSON
    	        const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

    	        std::stringstream log_str;
    	        Json::Value root;
    	        Json::Reader reader;
    	        bool parseSuccess = reader.parse(post_data, root, false);
    	        bool error = false;
            	Json::Value aclAttributeResponses(Json::arrayValue);
    	        if (parseSuccess == false) {
    	        	responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Parse_Error));
    	        	responseObject->finalizeOK();
    	            return;
    	        } else {
    	        	if (root.type() == Json::arrayValue) {
    	        		aclAttributeResponses.resize(root.size());
    	        		bool atleastOnValidEntry = false;
    	        		//the record parameter is an array of json objects
    	        		for(Json::UInt index = 0; index < root.size(); index++) {
    	        			Json::Value defaultValueToReturn = Json::Value("");
    	        			const Json::Value doc = root.get(index,
    	        					defaultValueToReturn);

    	        			vector<string> attributeList;
    	        			vector<string> roleIdList;
    	        			bool  status = AttributeAccessControl::processSingleJSONAttributeAcl(
    	        					doc, apiName, aclAttributeResponses[index], roleIdList,
    	        					attributeList, *coreInfo->getSchema());
    	        			if (status == false) {
    	        				// there is an error with the current acl entry. Do not error out
    	        				// because we want to process other valid entries.
    	        				attributeList.clear();
    	        				roleIdList.clear();
    	        			} else {
    	        				atleastOnValidEntry = true;
    	        				prepareAclDataForApiLayer(*attributeAclDataForApiLayer,
    	        						roleIdList, attributeList);
    	        				// if the response is empty then add success message.
    	        				if (aclAttributeResponses[index].asString().size() == 0){
    	        					stringstream ss;
    	        					ss << "API : " << apiName << ", Success";
    	        					aclAttributeResponses[index] = ss.str();
    	        				}
    	        			}
    	        		}

    	        		if (!atleastOnValidEntry) {
    	        			// all entries in the array were invalid.
    	        			for(Json::UInt index = 0; index < root.size(); index++) {
    	        				responseObject->addError(aclAttributeResponses[index]);
    	        			}
    	        			responseObject->finalizeOK();
    	        			return;
    	        		}
    	        	} else {
    	        		aclAttributeResponses.resize(1);
    	        		// the record parameter is a single json object
    	        		const Json::Value doc = root;
    	        		vector<string> attributeList;
    	        		vector<string> roleIdList;
    	        		bool  status = AttributeAccessControl::processSingleJSONAttributeAcl(doc,
    	        				apiName, aclAttributeResponses[0], roleIdList, attributeList,
    	        				*coreInfo->getSchema());
    	        		if (status == false) {
    	        			responseObject->addError(aclAttributeResponses[0]);
    	        			responseObject->finalizeOK();
    	        			return;
    	        		} else {
    	        			prepareAclDataForApiLayer(*attributeAclDataForApiLayer,
    	        					roleIdList, attributeList);
    	        			// if the response is empty then add success message.
    	        			if (aclAttributeResponses[0].asString().size() == 0){
    	        				stringstream ss;
    	        				ss << "API : " << apiName << ", Success";
    	        				aclAttributeResponses[0] = ss.str();
    	        			}
    	        		}
    	        	}
    	        }
    	        break;
    	    }
    	    default:
    	    	this->getSession()->response->finalizeInvalid();
    	    	return;

    	}

        // When query parameters are parsed successfully, we must create and run AclCommand class and get back
        // its response in a 'consume' callback function.
    	aclCommand = new WriteCommand(this, *attributeAclDataForApiLayer, action, aclCoreInfo);

        aclCommand->produce();

        delete attributeAclDataForApiLayer;
        return;
    }


    void aclReplaceDeletePhase(const vector<string> & attributes, unsigned aclCoreId){
    	SP(AclAttributeReplaceNotification) request  = SP(AclAttributeReplaceNotification)(
    			new AclAttributeReplaceNotification(attributes, aclCoreId));
    	ConcurrentNotifOperation * deleteNotifSender = new ConcurrentNotifOperation(request, ShardingAclAttrReadACKMessageType, this);

    	// This is where this message broadcasting starts ...
    	ShardManager::getStateMachine()->registerOperation(deleteNotifSender);
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

		string actionName;
		switch (action) {
			case ACL_REPLACE:
				actionName = string(c_action_acl_attribute_replace);
				break;
			case ACL_APPEND:
				actionName = string(c_action_acl_attribute_append);
				break;
			case ACL_DELETE:
				actionName = string(c_action_acl_attribute_delete);
				break;
		}

    	JsonResponseHandler * responseObject = this->getTransaction()->getSession()->response;
    	for (unsigned i = 0; i < httpPrintInfoInfoArr.size(); ++i) {

			Json::Value recordShardResponse(Json::objectValue);
			recordShardResponse[c_action] = actionName;
			recordShardResponse[c_status] = httpPrintInfoInfoArr[i].httpLayerSuccess;
			recordShardResponse[c_core_name] = coreInfo->getName();

    		if (httpPrintInfoInfoArr[i].httpLayerSuccess) {
    			const vector<string>& roleIds = httpPrintInfoInfoArr[i].roleIds;
    			bool success = true;
    			for (unsigned j = 0; j < roleIds.size(); ++j) {
    				map<string, bool>::const_iterator iter = results.find(roleIds[i]);
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
    			}
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
        return ShardingTransactionType_AttributeAclCommandCode; // returns the unique type identifier of this transaction
    }

    string getName() const {return "attribute-acl-command-http" ;};


private:
    WriteCommand * aclCommand;
    evhttp_request *req;
    const CoreInfo_t * coreInfo;
    const CoreInfo_t * aclCoreInfo;
	AclActionType action;
	struct HTTPPrintInfo{
		bool httpLayerSuccess;
		string httpLayerMsg;
		vector<string> roleIds;
	};
	vector<HTTPPrintInfo> httpPrintInfoInfoArr;
};


}
}



#endif // __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__
