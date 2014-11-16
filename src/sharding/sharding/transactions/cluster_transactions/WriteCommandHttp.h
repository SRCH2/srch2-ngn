#ifndef __SHARDING_SHARDING_WRITE_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_WRITE_COMMAND_HTTP_H__

#include "WriteCommand.h"
#include "../../metadata_manager/Shard.h"
#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../notifications/CommandStatusNotification.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class WriteCommandHttp : public ReadviewTransaction, public ConsumerInterface{
public:
	static void insert(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		SP(WriteCommandHttp) insertUpdateCmd =
				SP(WriteCommandHttp)(new WriteCommandHttp(clusterReadview, req, coreId, Insert_ClusterRecordOperation_Type));
		Transaction::startTransaction(insertUpdateCmd);
		return;
	}
	static void update(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		SP(WriteCommandHttp) insertUpdateCmd =
				SP(WriteCommandHttp)(new WriteCommandHttp(clusterReadview, req, coreId, Update_ClusterRecordOperation_Type));
		Transaction::startTransaction(insertUpdateCmd);
		return;
	}

	static void deleteRecord(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		SP(WriteCommandHttp) deleteCmd =
				SP(WriteCommandHttp)(new WriteCommandHttp(clusterReadview, req, coreId, Delete_ClusterRecordOperation_Type));
		Transaction::startTransaction(deleteCmd);
		return;
	}
	~WriteCommandHttp(){
		finalize();
		for(unsigned i = 0 ; i < recordsToInsert.size(); ++i){
			if(recordsToInsert.at(i) != NULL){
				delete recordsToInsert.at(i);
			}
		}
		if(inserter != NULL){
			delete inserter;
		}
	};
private:
	WriteCommandHttp(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId,
			ClusterRecordOperation_Type code = Insert_ClusterRecordOperation_Type): ReadviewTransaction(clusterReadview){
		ASSERT(this->getSession() == NULL);
		this->commandCode = code;
		this->indexDataContainerConf = this->getReadview()->getCore(coreId);
		if(this->indexDataContainerConf == NULL){
			Logger::sharding(Logger::Detail, "InsertUpdate| core id %d not found in cores.", coreId);
			return;
		}
		this->req = req;
		this->inserter = NULL;
		this->coreName = indexDataContainerConf->getName();
	}

	void run(){
		if(this->indexDataContainerConf == NULL){
	        this->getSession()->response->finalizeInvalid();
			return;
		}

	    if(commandCode == Delete_ClusterRecordOperation_Type){
	    	if(! parseDelete()){
	    		return;
	    	}
			inserter = new WriteCommand(this, primaryKeysToDelete, indexDataContainerConf);
	    }else {
	    	if(! parseInsertUpdate()){
	    		return;
	    	}
			inserter = new WriteCommand(this, recordsToInsert, commandCode, indexDataContainerConf);
	    }
	    inserter->produce();
	}

	SP(Transaction) getTransaction(){
		return this->sharedPointer;
	}

	bool parseInsertUpdate(){
	    // it must be an insert query
	    ASSERT(req->type == EVHTTP_REQ_PUT);
	    if(req->type != EVHTTP_REQ_PUT){
	    	this->getSession()->response->finalizeInvalid();
	        return false;
	    }

	    size_t length = EVBUFFER_LENGTH(req->input_buffer);

	    if (length == 0) {
	    	this->getSession()->response->finalizeError(JsonResponseHandler::getJsonSingleMessageStr(HTTP_JSON_Empty_Body));
	        return false;
	    }



	    // Parse example data
	    Json::Value root;
	    Json::Reader reader;
	    const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);
	    bool parseSuccess = reader.parse(post_data, root, false);

	    if (parseSuccess == false) {
	    	this->getSession()->response->finalizeError(JsonResponseHandler::getJsonSingleMessageStr(HTTP_JSON_Parse_Error));
	        return false;
	    }

	    Schema * storedSchema = Schema::create();
	    RecordSerializerUtil::populateStoredSchema(storedSchema, indexDataContainerConf->getSchema());
	    RecordSerializer recSerializer = RecordSerializer(*storedSchema);


	    if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
	        // Iterates over the sequence elements.
	        for ( int index = 0; index < root.size(); ++index ) {

	            Json::Value defaultValueToReturn = Json::Value("");
	            const Json::Value doc = root.get(index,
	                    defaultValueToReturn);

	            Record * record = parseRecord(doc, recSerializer);
	            if(record == NULL){
	            	return false;
	            }else{
	                // record is ready to insert
	                recordsToInsert.push_back(record);
	            }
	        }
	    } else {  // only one json object needs to be inserted

            Record * record = parseRecord(root, recSerializer);
            if(record == NULL){
            	return false;
            }else{
                // record is ready to insert
                recordsToInsert.push_back(record);
            }

	    }
	    delete storedSchema;


	    this->getSession()->response->finalizeOK();
	    if(recordsToInsert.empty()){
	        return false;
	    }
	    return true;
	}


	bool parseDelete(){
	    ASSERT(req->type == EVHTTP_REQ_DELETE);
	    if(req->type != EVHTTP_REQ_DELETE){
	    	this->getSession()->response->finalizeInvalid();
	        return false;
	    }

	    const CoreInfo_t *indexDataContainerConf = this->indexDataContainerConf;
	    const string coreName = indexDataContainerConf->getName();

	    evkeyvalq headers;
	    evhttp_parse_query(req->uri, &headers);
	    this->getSession()->response->setHeaders(&headers);

	    //set the primary key of the record we want to delete
	    std::string primaryKeyName = indexDataContainerConf->getPrimaryKey();
	    const char *pKeyParamName = evhttp_find_header(&headers, primaryKeyName.c_str());
	    //TODO : we should parse more than primary key later
	    if (pKeyParamName){
	        this->getSession()->response->finalizeOK();
	        size_t sz;
	        char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);
	        const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
	        free(pKeyParamName_cstar);

	        if(primaryKeyStringValue.compare("") != 0){
	        	primaryKeysToDelete.push_back(primaryKeyStringValue);
	        }else{
	        	return false;
	        }
	        return true;
	    }

	    this->getSession()->response->finalizeOK();
	    this->getSession()->response->addError(JsonResponseHandler::getJsonSingleMessageStr(HTTP_JSON_PK_NOT_PROVIDED));
		// Free the objects
		evhttp_clear_headers(&headers);
		return false;
	}

	void consume(const map<string, bool> & results,
				map<string, map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > > & messageCodes){


		// iterate on primary keys
		for(map<string, bool>::const_iterator recItr = results.begin(); recItr != results.end(); ++recItr){
			string actionName;
			switch (commandCode) {
				case Insert_ClusterRecordOperation_Type:
					actionName = string(c_action_insert);
					break;
				case Update_ClusterRecordOperation_Type:
					actionName = string(c_action_update);
					break;
				case Delete_ClusterRecordOperation_Type:
					actionName = string(c_action_delete);
					break;
				case AclRecordAdd_ClusterRecordOperation_Type:
					actionName = string(c_action_acl_record_add);
					break;
				case AclRecordAppend_ClusterRecordOperation_Type:
					actionName = string(c_action_acl_record_append);
					break;
				case AclRecordDelete_ClusterRecordOperation_Type:
					actionName = string(c_action_acl_record_delete);
					break;
				case AclAttrReplace_ClusterRecordOperation_Type:
					actionName = string(c_action_acl_attribute_replace);
					break;
				case AclAttrDelete_ClusterRecordOperation_Type:
					actionName = string(c_action_acl_attribute_delete);
					break;
				case AclAttrAppend_ClusterRecordOperation_Type:
					actionName = string(c_action_acl_attribute_append);
					break;
			}
			Json::Value recordShardResponse =
					JsonRecordOperationResponse::getRecordJsonResponse(recItr->first, actionName.c_str(),
					recItr->second, coreName);
			if(messageCodes.find(recItr->first) == messageCodes.end()){
				ASSERT(false);
				continue;
			}
			map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > & primaryKeyMessageCode = messageCodes[recItr->first];
			for(map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator >::iterator shardItr =
					primaryKeyMessageCode.begin(); shardItr != primaryKeyMessageCode.end(); ++shardItr){
				JsonRecordOperationResponse::addRecordMessages(recordShardResponse, shardItr->second);
			}
			JsonRecordOperationResponse * responseChannel = (JsonRecordOperationResponse *) this->getSession()->response;
			responseChannel->addRecordShardResponse(recordShardResponse);
		}
	};

	Record * parseRecord(const Json::Value & doc, RecordSerializer & recSerializer){
        Record *record = new Record(indexDataContainerConf->getSchema());
        vector<string> roleIds;
        // check if there is roleId in the query or not
		std::stringstream log_str;
        if( JSONRecordParser::_extractRoleIds(roleIds, doc, indexDataContainerConf, log_str) ){
        	if(indexDataContainerConf->getHasRecordAcl()){
        		// add role ids to the record object
        		record->setRoleIds(roleIds);
        	}else{
        		Logger::error("error: %s does not have record-based access control.",indexDataContainerConf->getName().c_str());
        		this->getSession()->response->addError(Json::Value(indexDataContainerConf->getName() +
        				JsonResponseHandler::getJsonSingleMessageStr(HTTP_JSON_Request_ACL_Not_Available)));
        		this->getSession()->response->addMessage(log_str.str());
        		this->getSession()->response->finalizeInvalid();
        		delete record;
        		return NULL;
        	}
        }

        Json::FastWriter writer;
        std::stringstream errorStream;
        if(JSONRecordParser::_JSONValueObjectToRecord(record, doc,
                indexDataContainerConf, errorStream, recSerializer) == false){

        	Json::Value recordJsonResponse = JsonRecordOperationResponse::getRecordJsonResponse(record->getPrimaryKey(), c_action_insert, false , coreName);
        	JsonRecordOperationResponse::addRecordError(recordJsonResponse, HTTP_JSON_Custom_Error, errorStream.str());
        	((JsonRecordOperationResponse *)this->getSession()->response)->addRecordShardResponse(recordJsonResponse);

            delete record;
            return NULL;
        }
        return record;
	}

	void initSession(){
		this->setSession(new TransactionSession());
		this->getSession()->response = new JsonRecordOperationResponse();
	}
	void finalizeWork(Transaction::Params * p){
		this->getSession()->response->printHTTP(req);
	}
	string getName() const{
		return "http-write-command";
	}
	ShardingTransactionType getTransactionType(){
		return ShardingTransactionType_InsertUpdateCommand;
	}
private:
	ClusterRecordOperation_Type commandCode;
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	evhttp_request *req;
	const CoreInfo_t * indexDataContainerConf;
    string coreName;
    vector<Record *> recordsToInsert;
    vector<string> primaryKeysToDelete;
    WriteCommand * inserter;
};



}
}

#endif // __SHARDING_SHARDING_WRITE_COMMAND_HTTP_H__
