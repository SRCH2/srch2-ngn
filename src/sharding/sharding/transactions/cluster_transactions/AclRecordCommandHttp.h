#ifndef __SHARDING_SHARDING_ACL_RECORD_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_ACL_RECORD_COMMAND_HTTP_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"

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
class AclRecordCommandHttpHandler: public ReadviewTransaction, public ConsumerInterface {
public:

    static void runCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId, srch2is::RecordAclCommandType commandType){

    	SP(AclRecordCommandHttpHandler) aclCommandHttpHandler =
    			SP(AclRecordCommandHttpHandler)( new AclRecordCommandHttpHandler(
        		clusterReadview, req, coreId, commandType)); //
        Transaction::startTransaction(aclCommandHttpHandler);
        return ;
    }

    ~AclRecordCommandHttpHandler(){
    	if(aclCommand != NULL){
    		delete aclCommand;
    	}
    	if(req != NULL){
    		delete req;
    	}
    }
private:
    AclRecordCommandHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId, srch2is::RecordAclCommandType commandType /*, and maybe other arguments */):ReadviewTransaction(clusterReadview){
    	this->req = req;
    	this->coreInfo = clusterReadview->getCore(coreId);
    	ASSERT(this->coreInfo != NULL);
        aclCommand = NULL;
        recordAclCommandType = commandType;

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
    		return ;
    	}

    	std::map< string, vector<string> > * recordAclDataForApiLayer =
    	    			new std::map< string, vector<string> >();

    	Json::Value response(Json::arrayValue);

    	if(coreInfo->getHasRecordAcl()){ // this core has record Acl

    		size_t length = EVBUFFER_LENGTH(req->input_buffer);

    		if (length == 0) {
    			responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Empty_Body));
    			responseObject->finalizeOK();
    			return ;
    		}

    		const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);

    		// Parse example data
    		Json::Value root;
    		Json::Reader reader;
    		bool parseSuccess = reader.parse(post_data, root, false);

    		if (parseSuccess == false) {
    			responseObject->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Parse_Error));
    			responseObject->finalizeOK();
    			return ;
    		}else{
    			if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
    				response.resize(root.size());
    				for ( int index = 0; index < root.size(); ++index ) {
    					Json::Value defaultValueToReturn = Json::Value("");
    					const Json::Value doc = root.get(index,
    							defaultValueToReturn);
        				vector<string> roleIds;
    					string primaryKeyID;
    					std::stringstream log_str;
    					// extract all the role ids from the query
    					bool success = JSONRecordParser::_extractResourceAndRoleIds(roleIds, primaryKeyID,
    							doc, coreInfo, log_str);
    					response[index] = log_str.str();
    					if (success) {
    						prepareAclDataForApiLayer(*recordAclDataForApiLayer, primaryKeyID, roleIds);
    					}
    				}
    			}else{ // The input is only one JSON object.
    				response.resize(1);
    				const Json::Value doc = root;
    				vector<string> roleIds;
    				string primaryKeyID;
    				std::stringstream log_str;
    				// extract all the role ids from the query
    				bool success = JSONRecordParser::_extractResourceAndRoleIds(roleIds, primaryKeyID, doc, coreInfo, log_str);
    				if(!success){
    					responseObject->addMessage("error:" + coreInfo->getName() + " does not have record-based access control.");
    					responseObject->finalizeOK();
    					return ;
    				} else {
    					response[0] = log_str.str();
    					prepareAclDataForApiLayer(*recordAclDataForApiLayer, primaryKeyID, roleIds);
    				}
    			}
    		}

    	}else{
			responseObject->addMessage("error:" + coreInfo->getName() + " does not have record-based access control.");
			responseObject->finalizeOK();
			return ;
    	}

        // When query parameters are parsed successfully, we must create and run AclCommand class and get back
        // its response in a 'consume' callback function.

    	aclCommand = new WriteCommand(this, *recordAclDataForApiLayer, recordAclCommandType, coreInfo);

        aclCommand->produce();

        delete recordAclDataForApiLayer;

        return;
    }

    void prepareAclDataForApiLayer(std::map< string, vector<string> > &recordAclDataForApiLayer,
    		const string primaryKey, const vector<string>& roleIdsList) {
    	std::map< string, vector<string> >::iterator iter =
    			recordAclDataForApiLayer.find(primaryKey);
    	if (iter != recordAclDataForApiLayer.end()) {
    		vector<string> unionList;
    		std::set_union(iter->second.begin(), iter->second.end(), roleIdsList.begin(),
    				roleIdsList.end(), back_inserter(unionList));
    		iter->second.assign(unionList.begin(), unionList.end());
    	} else {
    		recordAclDataForApiLayer.insert(make_pair(primaryKey, roleIdsList));
    	}
    }
    /*
     * One example of consume callback that will be called by the producer class
     * If another set of arguments is needed for this module, a new consume method must be
     * added to ConsumerInterface, it must be overridden here to process the results of AclCommand
     * and it must be called in finalize method of AclCommand to return back to this consumer.
     */
    void consume(const map<string, bool> & results,
			map<string, map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > > & messageCodes){
    	// TODO : must use this consume function to print to HTTP channel
    }

    void finalizeWork(Transaction::Params * params){
		this->getTransaction()->getSession()->response->printHTTP(req);
    }

    /*
     * This function must be overridden for each transaction class so that producers can use the
     * transaction and it's getSession() inteface.
     */
    Transaction * getTransaction() {
        return this;
    }

    ShardingTransactionType getTransactionType(){
        return ShardingTransactionType_RecordAclCommandCode; // returns the unique type identifier of this transaction
    }

    string getName() const {return "record-acl-command-http" ;};


private:
    WriteCommand * aclCommand;
    evhttp_request *req;
    const CoreInfo_t * coreInfo;
    srch2::instantsearch::RecordAclCommandType recordAclCommandType;
};


}
}



#endif // __SHARDING_SHARDING_ACL_RECORD_COMMAND_HTTP_H__
