#ifndef __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__

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
class AclCommandHttpHandler: public ReadviewTransaction, public ConsumerInterface {
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
private:
    AclCommandHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId /*, and maybe other arguments */):ReadviewTransaction(clusterReadview){
    	this->req = req;
    	this->coreInfo = clusterReadview->getCore(coreId);
    	unsigned aclCoreId = this->coreInfo->getAttributeAclCoreId();
		this->aclCoreInfo = clusterReadview->getCore(aclCoreId);
    	ASSERT(this->aclCoreInfo != NULL);
    	ASSERT(this->coreInfo != NULL);
        initSession();
        aclCommand = NULL;

    }
    /*
     * Must be implemented for all Transaction classes to initialize the session object.
     */
    void initSession(){
        TransactionSession * session = new TransactionSession();
        // used to save Json messages throughout the process, json messages
        // can be printed to HTTP channel by using the print method of this class.
        session->response = new JsonResponseHandler();
        this->setSession(session);
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
    	Json::Value response(Json::objectValue);
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
    	        AclActionType action;
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

    	        			vector<string> atributeList;
    	        			vector<string> roleIdList;
    	        			bool  status = processSingleJSONAttributeAcl(doc, action, apiName,
    	        					aclAttributeResponses[index], roleIdList, atributeList);
    	        			if (status == false) {
    	        				// there is an error with the current acl entry. Do not error out
    	        				// because we want to process other valid entries.
    	        				atributeList.clear();
    	        				roleIdList.clear();
    	        				//attributeAclDataForApiLayer->insert(make_pair(roleIdList, atributeList));
    	        			} else {
    	        				atleastOnValidEntry = true;
    	        				//attributeAclDataForApiLayer->push_back(make_pair(roleIdList, atributeList));
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
    	        		vector<string> atributeList;
    	        		vector<string> roleIdList;
    	        		bool  status = processSingleJSONAttributeAcl(doc, action, apiName,
    	        				aclAttributeResponses[0], roleIdList, atributeList);
    	        		if (status == false) {
    	        			responseObject->addError(aclAttributeResponses[0]);
    	        			responseObject->finalizeOK();
    	        			return;
    	        		} else {
    	        			//attributeAclDataForApiLayer->push_back(make_pair(roleIdList, atributeList));
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

    	//*** USE: attributeAclDataForApiLayer, action, aclCoreInfo as an argument for ACLCommand API  ***.

//        aclCommand = new AclCommand(this/*, and maybe other arguments */);//TODO
    	/*
			// WriteCommand(ConsumerInterface * consumer, //this
			//      	 map<string, vector<string> >, // map from primaryKey to list of roleIds
			//	 ClusterACLOperation_Type aclOperationType,
			//	 const CoreInfo_t * coreInfo);
			// WriteCommand(ConsumerInterface * consumer, //this
			//      	 vector<string> , // list of attributes
			//      	 vector<string> , // list of roleIds
			//	 ClusterACLOperation_Type aclOperationType,
			//	 const CoreInfo_t * coreInfo);
    	 */

        aclCommand->produce();
        return;
    }

    bool processSingleJSONAttributeAcl(const Json::Value& doc, AclActionType action,
    		const string& apiName, Json::Value& aclAttributeResponse, vector<string>& roleIds,
        	vector<string>& attributeList) const{

    	Json::Value attributesToAdd = doc.get("attributes", Json::Value(Json::arrayValue));
    	Json::Value attributesRoles = doc.get("roleId", Json::Value(Json::arrayValue));

    	if (attributesToAdd.type()  != Json::arrayValue) {
    		std::stringstream log_str;
    		log_str << "API : " << apiName << ", Error: 'attributes' key is not an array in request JSON.";
    		aclAttributeResponse = log_str.str();
    		return false;
    	}
    	if (attributesToAdd.size() == 0) {
    		std::stringstream log_str;
    		log_str << "API : " << apiName << ", Error: 'attributes' key is empty or missing in request JSON.";
    		aclAttributeResponse = log_str.str();
    		return false;
    	}
    	if (attributesRoles.type() != Json::arrayValue) {
    		std::stringstream log_str;
    		log_str << "API : " << apiName << ", Error: 'roleId' key is not an array in request JSON.";
    		aclAttributeResponse = log_str.str();
    		return false;
    	}
    	if (attributesRoles.size() == 0) {
    		std::stringstream log_str;
    		log_str << "API : " << apiName << ", Error: 'roleId' key is empty or missing in request JSON.";
    		aclAttributeResponse = log_str.str();
    		return false;
    	}
    	vector<string> invalidAttributeNames;
    	for (unsigned i = 0; i < attributesToAdd.size(); ++i) {
    		Json::Value defaultValueToReturn = Json::Value("");
    		const Json::Value attribute = attributesToAdd.get(i, defaultValueToReturn);
    		if (attribute.type() != Json::stringValue){
    			std::stringstream log_str;
    			log_str << "API : " << apiName << ", Error: 'attributes' key's element at index "<< i << " is not convertible to string";
    			aclAttributeResponse = log_str.str();
    			return false;
    		}
    		string tempString = attribute.asString();
    		boost::algorithm::trim(tempString);
    		if (tempString.size() != 0) {
    			if (tempString == "*" || coreInfo->getSchema()->isValidAttribute(tempString)) {
    				attributeList.push_back(tempString);
    			} else {
    				invalidAttributeNames.push_back(tempString);
    			}
    		}
    	}

    	if (attributeList.size() == 0) {
    		// All elements in the attribute list are either empty or have bogus value.
    		std::stringstream log_str;
    		log_str << "API : " << apiName << ", Error: 'attributes' key's elements are not valid.";
    		aclAttributeResponse = log_str.str();
    		return false;
    	}

    	// We have some valid attribute names in attributes list. Check whether there are some invalid
    	// name as well. If there are invalid attribute names, then generate warning log message and proceed
    	if (invalidAttributeNames.size() > 0) {
    		std::stringstream log_str;
    		if (invalidAttributeNames.size() > 1)
    			log_str << "API : " << apiName << ", Warning: 'attributes' key has bad attributes = '";
    		else
    			log_str << "API : " << apiName << ", Warning: 'attributes' key has bad attribute = '";
    		for (unsigned i = 0; i < invalidAttributeNames.size(); ++i) {
    			if (i)
    				log_str << ", ";
    			log_str << invalidAttributeNames[i];
    		}
    		log_str << "'.";
    		aclAttributeResponse = log_str.str();
    	}

    	for (unsigned i = 0; i < attributesRoles.size(); ++i) {
    		Json::Value defaultValueToReturn = Json::Value("");
    		const Json::Value roleId = attributesRoles.get(i, defaultValueToReturn);

    		switch (roleId.type()) {
    		case Json::stringValue:
    		{
    			string tempString = roleId.asString();
    			boost::algorithm::trim(tempString);
    			if (tempString.size() != 0)
    				roleIds.push_back(tempString);
    			break;
    		}
    		case Json::intValue:
    		{
    			// convert int to string instead of returning error to user
    			stringstream tempString;
    			tempString << roleId.asInt64();
    			roleIds.push_back(tempString.str());
    			break;
    		}
    		case Json::uintValue:
    		{
    			// convert unsigned int to string instead of returning error to user
    			stringstream tempString;
    			tempString << roleId.asUInt64();
    			roleIds.push_back(tempString.str());
    			break;
    		}
    		case Json::realValue:
    		{
    			// convert double to string instead of returning error to user
    			stringstream tempString;
    			tempString << roleId.asDouble();
    			roleIds.push_back(tempString.str());
    			break;
    		}
    		case Json::arrayValue:
    		case Json::objectValue:
    		{
    			// Can't convert to array ..user should fix the input JSON.
    			std::stringstream log_str;
    			log_str << "API : " << apiName << ", Error: 'roleId' key's element at index "<< i << " is not convertible to string";
    			aclAttributeResponse = log_str.str();
    			return false;
    		}
    		default:
    			ASSERT(false);
    		}
    	}

    	if (roleIds.size() == 0) {
    		std::stringstream log_str;
    		log_str << "API : " << apiName << ", Error: 'roleId' key's elements are not valid.";
    		aclAttributeResponse = log_str.str();
    		return false;
    	}

    	return true;
    }

    /*
     * One example of consume callback that will be called by the producer class
     * If another set of arguments is needed for this module, a new consume method must be
     * added to ConsumerInterface, it must be overridden here to process the results of AclCommand
     * and it must be called in finalize method of AclCommand to return back to this consumer.
     */
    void consume(bool booleanResult, vector<JsonMessageCode> & messageCodes){

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
};


}
}



#endif // __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__
