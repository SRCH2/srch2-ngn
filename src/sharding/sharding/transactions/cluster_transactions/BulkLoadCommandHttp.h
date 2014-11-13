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

    	if (!bulkLoader->isBulkLoadDone()) {
    		bulkLoader->produce();
    		return;
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

    string getName() const { return "bulkload-command-http"; };


private:
    BulkLoadCommand * bulkLoader;
    evhttp_request *req;
    const CoreInfo_t * coreInfo;
    BulkLoadType bulkLoadType;
};

}
}
#endif /* __SHARDING_SHARDING_BULK_LOAD_COMMAND_HTTP_H__ */
