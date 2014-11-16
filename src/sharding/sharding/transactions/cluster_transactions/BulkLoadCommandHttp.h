/*
 * BulkLoadHttpCommand.h
 *
 *  Created on: Nov 11, 2014
 *      Author: srch2
 */

#ifndef __SHARDING_SHARDING_BULK_LOAD_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_BULK_LOAD_COMMAND_HTTP_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"

#include "BulkLoadCommand.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class BulkLoadCommandHttpHandler: public ReadviewTransaction, public ConsumerInterface {
public:

    static void runCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId, BulkLoadType type){

        SP(BulkLoadCommandHttpHandler) bulkLoadCommandHttpHandler =
        		SP(BulkLoadCommandHttpHandler)(new BulkLoadCommandHttpHandler(clusterReadview,
        				req, coreId, type));
        Transaction::startTransaction(bulkLoadCommandHttpHandler);
        return ;
    }

    ~BulkLoadCommandHttpHandler(){
    	finalize();
    	delete bulkLoader;
        delete req;
    }

private:
    BulkLoadCommandHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId, BulkLoadType type):ReadviewTransaction(clusterReadview){
    	this->req = req;
    	this->coreInfo = clusterReadview->getCore(coreId);
    	unsigned aclCoreId = this->coreInfo->getAttributeAclCoreId();
    	ASSERT(this->coreInfo != NULL);
        bulkLoader = NULL;
        bulkLoadType = type;
        totalFailedCount = 0;
        totalSuccessCount = 0;
    }

    /*
     * The main work of BulkLoadCommandHttpHandler starts in this function
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

    	evkeyvalq header;
    	evhttp_parse_query(req->uri, &header);
    	const char * filename = evhttp_find_header(&header, "file");

    	string filenameStr(filename);

        // When query parameters are parsed successfully, we must create and run AclCommand class and get back
        // its response in a 'consume' callback function.
    	bulkLoader = new BulkLoadCommand(this, filename, bulkLoadType, coreInfo);
    	bulkLoader->init();
    	bulkLoader->produce();
        return;
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

    	switch (bulkLoadType) {
    	case RecordBulkLoad:
    	{
        	for(map<string, bool>::const_iterator recItr = results.begin(); recItr != results.end(); ++recItr){
        		if (recItr->second == false) {
        			++ totalFailedCount;
        			if(messageCodes.find(recItr->first) != messageCodes.end()){
        				MessageCodes &primaryKeyMessageCode = messageCodes[recItr->first];
        				for(MessageCodes::iterator shardItr =
        						primaryKeyMessageCode.begin(); shardItr != primaryKeyMessageCode.end(); ++shardItr){
        					vector<JsonMessageCode>& msgCode = shardItr->second;
        					for (unsigned i = 0 ; i < msgCode.size(); ++i){
        						Logger::console("Record Bulkload error : %s",
        								JsonResponseHandler::getJsonSingleMessageStr(msgCode.at(i)).c_str());
        					}
        				}
        			} else {
        				ASSERT(false);
        			}

        		} else {
        			++totalSuccessCount;
        		}
        	}
        	break;
    	}
    	case AclRecordBulkLoad:
    	{
    		const vector<AclRecordBatchInfo>& recordAclBatchInfo = bulkLoader->getRecordAclBatchInfo();
        	for(unsigned i = 0; i < recordAclBatchInfo.size(); ++i){
        		const string& pk =  recordAclBatchInfo[i].primaryKey;
        		bool httpLayerSuccess = recordAclBatchInfo[i].httpLayerSuccess;
        		if (httpLayerSuccess) {
        			map<string, bool>::const_iterator iter = results.find(pk);
        			if ( iter != results.end()) {
        				if (iter->second) {
        					++totalSuccessCount;
        				} else {
        					++totalFailedCount;
        					Logger::error("Record ACL bulkload error at line %d : "
        							"roleIds were not inserted for the primary key due to shard error",
        							recordAclBatchInfo[i].lineNumber);
        				}
        			} else {
        				ASSERT(false);
        			}
        		} else {
        			++totalFailedCount;
        			Logger::error("Record ACL bulkload error at line %d : %s",
        					recordAclBatchInfo[i].lineNumber, recordAclBatchInfo[i].httpLayerMsg.c_str());
        		}
        	}
        	break;
    	}
    	case AclAttributeBulkLoad:
    	{
    		const vector<AclAttributeBatchInfo>& attributeAclBatchInfo = bulkLoader->getAttributeAclBatchInfo();
    		for (unsigned i = 0; i < attributeAclBatchInfo.size(); ++i) {
    			if (attributeAclBatchInfo[i].httpLayerSuccess) {
    				unsigned failedRoleIds = 0;
    				stringstream failedRoleIdsStr;
    				const vector<string>& roleIds = attributeAclBatchInfo[i].roleIds;
    				for (unsigned j = 0; j < roleIds.size(); ++j) {
    					map<string, bool>::const_iterator iter = results.find(roleIds[j]);
    					if( iter != results.end()){
    						if (!iter->second) {
    							++failedRoleIds;
    							failedRoleIdsStr << roleIds[j] << ", ";
    						}
    					} else { ASSERT(false); }
    				}

    				if (failedRoleIds == roleIds.size()) {
    					++totalFailedCount;
    					Logger::error("Attribute ACL bulkload error at line %d : "
    							"All roleIds were not inserted successfully due to shard error",
    							attributeAclBatchInfo[i].lineNumber);
    				} else {
    					++totalSuccessCount;
    					Logger::warn("Attribute ACL bulkload error at line %d : "
    							"Following roleId(s) is/were not inserted successfully due to shard error = %s",
    							attributeAclBatchInfo[i].lineNumber, failedRoleIdsStr.str().c_str());
    				}
    			} else {
    				++totalFailedCount;
    				Logger::error("Attribute ACL bulkload error at line %d : %s",
    						attributeAclBatchInfo[i].lineNumber, attributeAclBatchInfo[i].httpLayerMsg.c_str());
    			}
    		}
    		break;
    	}
    	}

    	if (!bulkLoader->isBulkLoadDone()) {
    		bulkLoader->produce();
    		return;
    	}

		Json::Value recordShardResponse;

    	switch (bulkLoadType) {
    	case RecordBulkLoad:
    	{
    		recordShardResponse["type"] = "Record";
    		Logger::info("Record bulkload status: success = %u, failed = %u", totalSuccessCount,
    				totalFailedCount);
    		break;
    	}
    	case AclRecordBulkLoad:
    	{
    		recordShardResponse["type"] = "Record Acl";
    		Logger::info("Record ACL bulkload status: success = %u, failed = %u", totalSuccessCount,
    				totalFailedCount);
    		break;
    	}
    	case AclAttributeBulkLoad:
    	{
    		recordShardResponse["type"] = "Attribute Acl";
    		Logger::info("Attribute ACL bulkload status: success = %u, failed = %u", totalSuccessCount,
    				totalFailedCount);
    		break;
    	}
    	}
		recordShardResponse["success count"] = totalSuccessCount;
		recordShardResponse["failed count"] = totalFailedCount;
		JsonRecordOperationResponse * responseChannel = (JsonRecordOperationResponse *) this->getSession()->response;
		responseChannel->addRecordShardResponse(recordShardResponse);

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

    string getName() const { return "bulkload-command-http"; };


private:
    BulkLoadCommand * bulkLoader;
    evhttp_request *req;
    const CoreInfo_t * coreInfo;
    BulkLoadType bulkLoadType;

    unsigned totalFailedCount;
    unsigned totalSuccessCount;

};

}
}
#endif /* __SHARDING_SHARDING_BULK_LOAD_COMMAND_HTTP_H__ */
