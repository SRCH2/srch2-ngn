#ifndef __SHARDING_SHARDING_EXT_INSERT_UPDATE_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_EXT_INSERT_UPDATE_COMMAND_HTTP_H__

#include "./ExternalInsertUpdateCommand.h"
#include "../../metadata_manager/Shard.h"
#include "../../state_machine/State.h"
#include "../../state_machine/notifications/Notification.h"
#include "../../state_machine/notifications/CommandStatusNotification.h"
#include "../../state_machine/notifications/InsertUpdateNotification.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class ExternalInsertUpdateCommandHttp : public Transaction, public CommandStatusAggregationCallbackInterface{
public:
	static void insert(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		ExternalInsertUpdateCommandHttp * insertCmd = new ExternalInsertUpdateCommandHttp(clusterReadview, req, coreId);
		ShardManager::getStateMachine()->registerTransaction(insertCmd);
		if( ! insertCmd->run()){
			ShardManager::getStateMachine()->removeTransaction(insertCmd->getTID());
		}
		return;
	}
public:
	ShardingTransactionType getTransactionType(){
		return ShardingTransactionType_InsertUpdateCommand;
	}


	ExternalInsertUpdateCommandHttp(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			evhttp_request *req, unsigned coreId){
		this->indexDataContainerConf = clusterReadview->getCore(coreId);
		this->clusterReadview = clusterReadview;
		this->req = req;
		this->coreName = indexDataContainerConf->getName();
		this->brokerSideInformationJson = boost::shared_ptr<HTTPJsonRecordOperationResponse > (new HTTPJsonRecordOperationResponse(req));
	}

	~ExternalInsertUpdateCommandHttp(){};

	bool run(){
		parse();
	}


	bool parse(){
	    // it must be an insert query
	    ASSERT(req->type == EVHTTP_REQ_PUT);
	    if(req->type != EVHTTP_REQ_PUT){
	        brokerSideInformationJson->finalizeInvalid();
	        return false;
	    }

	    size_t length = EVBUFFER_LENGTH(req->input_buffer);

	    if (length == 0) {
	        brokerSideInformationJson->finalizeError("Http body is empty.");
	        return false;
	    }



	    // Parse example data
	    Json::Value root;
	    Json::Reader reader;
	    const char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);
	    bool parseSuccess = reader.parse(post_data, root, false);

	    if (parseSuccess == false) {
	        brokerSideInformationJson->finalizeError("JSON object parse error");
	        return false;
	    }

	    Schema * storedSchema = Schema::create();
	    RecordSerializerUtil::populateStoredSchema(storedSchema, indexDataContainerConf->getSchema());
	    RecordSerializer recSerializer = RecordSerializer(*storedSchema);


	    vector<Record *> recordsToInsert;
	    if(root.type() == Json::arrayValue) { // The input is an array of JSON objects.
	        // Iterates over the sequence elements.
	        for ( int index = 0; index < root.size(); ++index ) {

	            /*
	             * SerializableInsertUpdateCommandInput destructor will deallocate Record objects
	             */
	            Record *record = new Record(indexDataContainerConf->getSchema());

	            Json::Value defaultValueToReturn = Json::Value("");
	            const Json::Value doc = root.get(index,
	                    defaultValueToReturn);

	            Json::FastWriter writer;
	            std::stringstream errorStream;
	            if(JSONRecordParser::_JSONValueObjectToRecord(record, doc,
	                    indexDataContainerConf, errorStream, recSerializer) == false){

	            	Json::Value recordJsonResponse = HTTPJsonRecordOperationResponse::getRecordJsonResponse(record->getPrimaryKey(), c_action_insert, false , coreName);
	            	HTTPJsonRecordOperationResponse::addRecordError(recordJsonResponse, HTTP_JSON_Custom_Error, errorStream.str());
	            	brokerSideInformationJson->addRecordShardResponse(recordJsonResponse);

	                delete record;
	            }else{
	                // record is ready to insert
	                recordsToInsert.push_back(record);
	            }

	        }
	    } else {  // only one json object needs to be inserted

	        /*
	         * SerializableInsertUpdateCommandInput destructor will deallocate Record objects
	         */
	        Record *record = new Record(indexDataContainerConf->getSchema());

	        const Json::Value doc = root;
	        Json::FastWriter writer;
	        std::stringstream errorStream;
	        if(JSONRecordParser::_JSONValueObjectToRecord(record, root,
	                indexDataContainerConf, errorStream , recSerializer) == false){

	        	Json::Value recordJsonResponse = HTTPJsonRecordOperationResponse::getRecordJsonResponse(record->getPrimaryKey(), c_action_insert, false , coreName);
	        	HTTPJsonRecordOperationResponse::addRecordError(recordJsonResponse, HTTP_JSON_Custom_Error, errorStream.str());
	        	brokerSideInformationJson->addRecordShardResponse(recordJsonResponse);
	            record->clear();
	            delete storedSchema;
	            delete record;
	        }else{
				// record is ready to insert
				recordsToInsert.push_back(record);
	        }
	    }
	    delete storedSchema;


		brokerSideInformationJson->finalizeOK();
	    if(recordsToInsert.size() == 0){
	        return false;
	    }

	    inserter = new ExternalInsertUpdateCommand(this, recordsToInsert, InsertUpdateNotification::DP_INSERT, indexDataContainerConf->getCoreId());
		if(this->isDeleteTopDown()){
			return false;
		}
	    inserter->start();

    	return true;
	}

	void receiveStatus(map<NodeId, vector<CommandStatusNotification::ShardStatus *> > shardsStatus){
		for(map<NodeId, vector<CommandStatusNotification::ShardStatus *> >::iterator nodeItr = shardsStatus.begin();
				nodeItr != shardsStatus.end(); ++nodeItr){
			NodeId nodeId = nodeItr->first;
			for(unsigned i = 0 ; i < nodeItr->second.size(); ++i){
				CommandStatusNotification::ShardStatus * shardStatus = nodeItr->second.at(i);
				Json::Value recordShardResponse =
						HTTPJsonRecordOperationResponse::getRecordJsonResponse(sentInsetUpdateRequest->getRecord()->getPrimaryKey(),
						(sentInsetUpdateRequest->getInsertOrUpdate() == WriteCommandNotification::DP_INSERT?c_action_insert:c_action_update),
						shardStatus->statusValue , coreName);
				HTTPJsonResponse::appendDetails(recordShardResponse, shardStatus->messages);
				this->brokerSideInformationJson->addRecordShardResponse(recordShardResponse);
				break;
			}

		}
	}

	TRANS_ID lastCallback(void * args) {

	}


private:
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
	boost::shared_ptr<HTTPJsonRecordOperationResponse > brokerSideInformationJson ;
	evhttp_request *req;
	const CoreInfo_t * indexDataContainerConf;
    string coreName;

    ExternalInsertUpdateCommand * inserter;
};



}
}

#endif // __SHARDING_SHARDING_EXT_INSERT_UPDATE_COMMAND_HTTP_H__
