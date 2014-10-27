#ifndef __SHARDING_SHARDING_CLUSTER_TRANS_ACL_COMMAND_H__
#define __SHARDING_SHARDING_CLUSTER_TRANS_ACL_COMMAND_H__

#include "../../state_machine/State.h"
#include "../../notifications/Notification.h"
#include "../../metadata_manager/Shard.h"
#include "../Transaction.h"
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
class AclCommand: public ProducerInterface, public NodeIteratorListenerInterface {
public:


    AclCommand(ConsumerInterface * consumer/* Arguments needed for ACL command execution like AclCommandCode (append or delete or add or ...)*/): ProducerInterface(consumer){

    }

    ~AclCommand(){

    }

    /*
     * This method is called from the user of this class to start its task.
     * If this module does not connects to state-machine for distributed messaging (through AtomicLock, AtomicRelease, or any
     *  of the node iterator classes) then this->getTransaction()->setUnattached() must be called so that the caller deallocates
     *  this module.
     *  Otherwise, we will just continue with using the distributed moduel results by processing them in callback functions like
     *  consume(...) (for AtomicLock, AtomicRelease, ShardCommand and ...) or end(...) (for node iterator classes.)
     */
    void produce(){
        //TODO
        // Start ACL work here.
        /*
         * Notes :
         * 1. How to add a new type of sharding notification :
         *     a. Add the notification class by using the DummyNotification example which can be
         *        found in Notification.h
         *     b. Register this new notification in ShardManager in resolveMessage() big switch statement to
         *        make it deserialize the notification and call resolveNotification() which we implemented inside
         *        the notification itself.
         * 2. How to do locking :
         *     AtomicLock and AtomicRelease Producers are created for locking purposes. "this" must pass to the
         *        constructor as the Consumer from which the "consume" function will be called to return the results.
         * 3. How to access session information:
         *     this->getTransaction()->getSession() returns a TransactionSession pointer which gives access to any kind of
         *     data that we want to use throughout processing this command. For example, clusteReadview of this communication
         *     session, or the response object to write and return messages.
         *
         */
        /*
         * Example of a code which is not going to wait for response from distributed modules :
         */
        if(this->getTransaction() != NULL){
            this->getTransaction()->setUnattached();
        }
    }
    void finalize(/* Optional arguments to finalize method.
                    This method must at the end if all core-paths of this calss*/){
        if(this->getConsumer() != NULL){
            // calling consume from the user of this class to give back the final result
            this->getConsumer()->consume(false/* If a new header is needed for consume callback, it must be added
                                                   in the ConsumerInterface.*/);
        }
    }

    // control methods
    Transaction * getTransaction(){
        if(this->getConsumer() == NULL){
            return NULL;
        }
        return this->getConsumer()->getTransaction();
    }
    string getName() const {return "ACL-command";};
};

}
}
#endif // __SHARDING_SHARDING_CLUSTER_TRANS_ACL_COMMAND_H__
