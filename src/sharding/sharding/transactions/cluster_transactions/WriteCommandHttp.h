/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SHARDING_SHARDING_TRANSACTIONS_CLUSTER_TRANSACTIONS_WRITE_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_TRANSACTIONS_CLUSTER_TRANSACTIONS_WRITE_COMMAND_HTTP_H__

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
	/*
	 * API used to insert record (records) given in request req, into data source coreId.
	 */
	static void insert(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		SP(WriteCommandHttp) insertUpdateCmd =
				SP(WriteCommandHttp)(new WriteCommandHttp(clusterReadview, req, coreId, Insert_ClusterRecordOperation_Type));
		Transaction::startTransaction(insertUpdateCmd);
		return;
	}

	/*
	 * API used to update record (records) given in request req, into data source coreId.
	 */
	static void update(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		SP(WriteCommandHttp) insertUpdateCmd =
				SP(WriteCommandHttp)(new WriteCommandHttp(clusterReadview, req, coreId, Update_ClusterRecordOperation_Type));
		Transaction::startTransaction(insertUpdateCmd);
		return;
	}

	/*
	 * API used to delete record (records) given in request req, from data source coreId.
	 */
	static void deleteRecord(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		SP(WriteCommandHttp) deleteCmd =
				SP(WriteCommandHttp)(new WriteCommandHttp(clusterReadview, req, coreId, Delete_ClusterRecordOperation_Type));
		Transaction::startTransaction(deleteCmd);
		return;
	}

	/*
	 * Destructor of WriteCommandHttp is called when all shared pointer references to
	 * this transaction object are destroyed.
	 * The task of this ReadviewTransaction is finalized in the destructor (so for example,
	 * writing the response to the http channel is done here ...).
	 */
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
		// what kind of operation is this transaction expected to do
		this->commandCode = code;
		// the data source (core) related to this record operation
		this->indexDataContainerConf = this->getReadview()->getCore(coreId);
		// if data source cannot be determined, exit.
		if(this->indexDataContainerConf == NULL){
			Logger::sharding(Logger::Detail, "InsertUpdate| core id %d not found in cores.", coreId);
			return;
		}
		// the HTTP request including the record operation info
		this->req = req;
		// The reference to WriteCommand object which is the main performer of this record operation
		this->inserter = NULL;
		// The name of this data source, for easier access
		this->coreName = indexDataContainerConf->getName();
	}

	/*
	 * the starting point of http record operation
	 */
	void run(){
		// if data source is not usable, abort.
		if(this->indexDataContainerConf == NULL){
	        this->getSession()->response->finalizeInvalid();
			return;
		}

		// do parsing based on the type of operation
	    if(commandCode == Delete_ClusterRecordOperation_Type){
	    	// parse delete record request
	    	if(! parseDelete()){
	    		return;
	    	}
			inserter = new WriteCommand(this, primaryKeysToDelete, indexDataContainerConf);
	    }else {
	    	// parse insert/update record operation request (docs RESTful API)
	    	if(! parseInsertUpdate()){
	    		return;
	    	}
			inserter = new WriteCommand(this, recordsToInsert, commandCode, indexDataContainerConf);
	    }

	    // Use the WriteCommand object to perform the requested record write operation
	    inserter->produce();
	}

	SP(Transaction) getTransaction(){
		return this->sharedPointer;
	}

	/*
	 * Parses the HTTP request in the case of insert/update.
	 * Returns true if parse is successful, and false otherwise
	 */
	bool parseInsertUpdate(){
	    // it must be an insert query
	    ASSERT(req->type == EVHTTP_REQ_PUT);

	    // RESTful API conditions
	    if(req->type != EVHTTP_REQ_PUT){
	    	this->getSession()->response->finalizeInvalid();
	        return false;
	    }
	    size_t length = EVBUFFER_LENGTH(req->input_buffer);

	    if (length == 0) {
	    	this->getSession()->response->finalizeError(JsonResponseHandler::getJsonSingleMessageStr(HTTP_JSON_Empty_Body));
	        return false;
	    }



	    // Parse the request and extract the working records
	    // and store them in recordsToInsert structure.
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
	            // record info parse failed
	            if(record == NULL){
	            	return false;
	            }else{
	                // record is ready to insert
	                recordsToInsert.push_back(record);
	            }
	        }
	    } else {  // only one json object needs to be inserted

            Record * record = parseRecord(root, recSerializer);
            // record info parse failed
            if(record == NULL){
            	return false;
            }else{
                // record is ready to insert
                recordsToInsert.push_back(record);
            }

	    }
	    delete storedSchema;


	    this->getSession()->response->finalizeOK();

	    // if no record is given to work on, parsing has not been successful
	    if(recordsToInsert.empty()){
	        return false;
	    }
	    return true;
	}

	/*
	 * Parse the delete record HTTP request.
	 * Returns true if parsing is successful, and false otherwise
	 */
	bool parseDelete(){

		// RESTful request HTTP conditions
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
	    //TODO : we can parse more than one primary key
	    //       it can be a richer RESTful API
	    if (pKeyParamName){
	        this->getSession()->response->finalizeOK();
	        size_t sz;
	        char *pKeyParamName_cstar = evhttp_uridecode(pKeyParamName, 0, &sz);
	        const std::string primaryKeyStringValue = string(pKeyParamName_cstar);
	        free(pKeyParamName_cstar);

	        if(primaryKeyStringValue.compare("") != 0){
	        	primaryKeysToDelete.push_back(primaryKeyStringValue);
	        }else{
	        	// primary key is not usable, parse failed.
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

	/*
	 * The result of work (performed by WriteCommand object) is reported back to this module
	 * through a call to the consume method.
	 * 'results' is a map from primary keys to the boolean overal status of operation
	 * 'messageCodes' is the container of messages that were produced through the work for each primary key
	 * messageCodes is expected to have one entry for each involved primary key
	 */
	void consume(const map<string, bool> & results,
				map<string, map<ShardId * ,vector<JsonMessageCode>, ShardPtrComparator > > & messageCodes){


		// iterate on primary keys
		// and prepare the json response for each record
		for(map<string, bool>::const_iterator recItr = results.begin(); recItr != results.end(); ++recItr){

			// translate the operation type to a string representing the task
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

			// Json value containing the result information of this operation for one single record
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
			// add record response to the final HTTP JSON response.
			responseChannel->addRecordShardResponse(recordShardResponse);
		}
	};

	// Helper function which parses the information of a single record out of the
	// HTTP request.
	Record * parseRecord(const Json::Value & doc, RecordSerializer & recSerializer){
		// empty record object
        Record *record = new Record(indexDataContainerConf->getSchema());

        // list of roles that may have been included in the request (in case the request has ACL involved)
        vector<string> roleIds;
        // check if there is roleId in the query or not
		std::stringstream log_str;
        if( JSONRecordParser::_extractRoleIds(roleIds, doc, indexDataContainerConf, log_str) ){
        	// but if the data source isn't using ACL, there is no point in including roleIds
        	if(indexDataContainerConf->getHasRecordAcl()){
        		// add role ids to the record object
        		record->setRoleIds(roleIds);
        	}else{
        		// going to abort, add proper messages to the json response handler objects
        		Logger::error("error: %s does not have record-based access control.",indexDataContainerConf->getName().c_str());
        		this->getSession()->response->addError(Json::Value(indexDataContainerConf->getName() +
        				JsonResponseHandler::getJsonSingleMessageStr(HTTP_JSON_Request_ACL_Not_Available)));
        		this->getSession()->response->addMessage(log_str.str());
        		this->getSession()->response->finalizeInvalid();
        		delete record;
        		return NULL;
        	}
        }

        // parse the actual record
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

	// initialize the transaction session object
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
	// what kind of operation is this transaction expected to do
	ClusterRecordOperation_Type commandCode;
	// the metadata readview version used in this record operation transaction
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	// the HTTP request including the record operation info
	evhttp_request *req;
	// the core on which this record write operation must be performed
	const CoreInfo_t * indexDataContainerConf;
	// The name of this data source, for easier access
    string coreName;
    // list of records to be inserted/updated.
    // NOTE: this vector remains empty in case of delete
    vector<Record *> recordsToInsert;
    // list of record primary keys to be deleted.
    // NOTE: this vector remains empty in cases of insert and update
    vector<string> primaryKeysToDelete;
    // The reference to WriteCommand object which is the main performer of this record operation
    WriteCommand * inserter;
};



}
}

#endif // __SHARDING_SHARDING_TRANSACTIONS_CLUSTER_TRANSACTIONS_WRITE_COMMAND_HTTP_H__
