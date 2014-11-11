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
            evhttp_request *req, unsigned coreId /*And maybe other argumets like ACL command type which can be passed
                                                    from Srch2ServerExternalGateway::cb_coreSpecificOperations or
                                                    from Srch2ServerExternalGateway::cb_globalOperations ...*/){

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
    	ASSERT(this->coreInfo != NULL);
    	// initializas the session object that can be accessed through this communication
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
    	if(coreInfo == NULL){
    		this->getTransaction()->getSession()->response->addError(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Core_Does_Not_Exist));
    		this->getTransaction()->getSession()->response->finalizeOK();
    		// to make the caller of this function deallocate this object
    		// because it's not going to wait for a notification from another node as it was
    		// expected.
    		return;
    	}
        //return false; // returns false if when we return from this function
                       // no callback function is supposed to be called and we can be
                       // lost. When it returns false, startTransaction() will just delete this transaction
                       // to end its work.

        // When query parameters are parsed successfully, we must create and run AclCommand class and get back
        // its response in a 'consume' callback function.
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
        return ShardingTransactionType_AclCommandCode; // returns the unique type identifier of this transaction
    }

    string getName() const {return "acl-command-http" ;};


private:
    WriteCommand * aclCommand;
    evhttp_request *req;
    const CoreInfo_t * coreInfo;
};


}
}



#endif // __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__
