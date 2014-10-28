#ifndef __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__
#define __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"

#include "core/util/Logger.h"
#include "core/util/Assert.h"
#include "server/HTTPJsonResponse.h"
#include "AclCommand.h"

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
class AclCommandHttpHandler: public Transaction, public ConsumerInterface {
public:

    static void runCommand(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId /*And maybe other argumets like ACL command type which can be passed
                                                    from Srch2ServerExternalGateway::cb_coreSpecificOperations or
                                                    from Srch2ServerExternalGateway::cb_globalOperations ...*/){

        AclCommandHttpHandler * aclCommandHttpHandler = new AclCommandHttpHandler(clusterReadview, req, coreId); //
        Transaction::startTransaction(aclCommandHttpHandler);
        return ;
    }
private:
    AclCommandHttpHandler(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
            evhttp_request *req, unsigned coreId /*, and maybe other arguments */){
        // initializas the session object that can be accessed through this communication
        initSession();
        this->getSession()->clusterReadview = clusterReadview;
        aclCommand = NULL;

    }

    ~AclCommandHttpHandler(){
        if(aclCommand != NULL){
            delete aclCommand;
        }
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
     */
    bool run(){
        //return false; // returns false if when we return from this function
                       // no callback function is supposed to be called and we can be
                       // lost. When it returns false, startTransaction() will just delete this transaction
                       // to end its work.

        // When query parameters are parsed successfully, we must create and run AclCommand class and get back
        // its response in a 'consume' callback function.
        aclCommand = new AclCommand(this/*, and maybe other arguments */);
        aclCommand->produce();
        if(this->getTransaction() != NULL && ! this->getTransaction()->isAttached){
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
    void consume(bool booleanResult){

    }



    void finalize(){

        // setFinished() must be called at the very last step of execution of every Transaction class
        // to notify state-machine that this Transaction is
        // done and is ok to be deleted now.
        this->setFinished();
    }


    /*
     * This function must be overridden for each transaction class so that producers can use the
     * transaction and it's getSession() inteface.
     */
    Transaction * getTransaction() {
        return this;
    }

    ShardingTransactionType getTransactionType(){
        return ShardingTransactionType_AclCommandCode; // returns the unique type identifier of this transaction
    }

    string getName() const {return "acl-command-http" ;};


private:
    AclCommand * aclCommand;
};


}
}



#endif // __SHARDING_SHARDING_ACL_COMMAND_HTTP_H__
