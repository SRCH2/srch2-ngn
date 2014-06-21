#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_


#include "sharding/configuration/ConfigManager.h"
#include "sharding/configuration/ShardingConstants.h"
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using namespace std;

namespace srch2 {
namespace httpwrapper {

class ConfigManager;
class Srch2Server;
class SearchCommand;
class SearchCommandResults;
class InsertUpdateCommand;
class DeleteCommand;
class CommandStatus;
class SerializeCommand;
class ResetLogCommand;
class CommitCommand;
class GetInfoCommand;
class GetInfoCommandResults;



enum DPInternalAPIStatus{
	DPInternal_Srch2ServerNotFound,
	DPInternal_Success
};

class DPInternalRequestHandler {

public:
    // Public API which can be used by other modules
    DPInternalRequestHandler(ConfigManager * configurationManager);


    /*
     * The following methods serve the operations requested by application layers such as
     * InternalMessageBroker in RM.
     *
     */

    /*
     * 1. Receives a search request from a shard
     * 2. Uses core to evaluate this search query
     * 3. Sends the results to the shard which initiated this search query
     */
    SearchCommandResults * internalSearchCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, SearchCommand * searchData);


    /*
     * This call back is always called for insert and update, it will use
     * internalInsertCommand and internalUpdateCommand
     */
    CommandStatus * internalInsertUpdateCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, InsertUpdateCommand * insertUpdateData);

    /*
     * 1. Receives an insert request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this insert query
     * 3. Sends the results to the shard which initiated this insert query (Failure or Success)
     */
    CommandStatus * internalInsertCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, InsertUpdateCommand * insertUpdateData);
    /*
     * 1. Receives a update request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this update query
     * 3. Sends the results to the shard which initiated this update request (Failure or Success)
     */
    CommandStatus * internalUpdateCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, InsertUpdateCommand * insertUpdateData);

    /*
     * 1. Receives a delete request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this delete query
     * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
     */
    CommandStatus *  internalDeleteCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, DeleteCommand * deleteData);



    /*
     * 1. Receives a GetInfo request from a shard
     * 2. Uses core to get info
     * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
     */
    GetInfoCommandResults * internalGetInfoCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, GetInfoCommand * getInfoData);


    /*
     * This call back function is called for serialization. It uses internalSerializeIndexCommand
     * and internalSerializeRecordsCommand for our two types of serialization.
     */
    CommandStatus * internalSerializeCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, SerializeCommand * seralizeData);

    /*
     * 1. Receives a SerializeIndex request from a shard
     * 2. Uses core to do the serialization
     * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
     */
    CommandStatus * internalSerializeIndexCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, SerializeCommand * seralizeData);

    /*
     * 1. Receives a SerializeRecords request from a shard
     * 2. Uses core to do the serialization
     * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
     */
    CommandStatus * internalSerializeRecordsCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, SerializeCommand * seralizeData);

    /*
     * 1. Receives a ResetLog request from a shard
     * 2. Uses core to reset log
     * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
     */
    CommandStatus * internalResetLogCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, ResetLogCommand * resetData);


    /*
     * Receives a commit command and commits the index
     */
    CommandStatus * internalCommitCommand(boost::shared_ptr<Srch2Server> serverSharedPtr, CommitCommand * resetData);


    /*
     * The following methods provide an API to register/allocate/delete/load/create and other operations on
     * indices.
     * NOTE: As of June 16th, since our core codebase is wrapped and accessed from sharding layers through Srch2Server
     * objects, indices and processing are combined in the Srch2Server objects, so DP Internal shouldn't be viewed as
     * an Index Manager. Index Managers tend to be a container for indices while this module is more of a wrapper on the API
     * provided by the core codebase.
     */
    boost::shared_ptr<Srch2Server> registerAndInitializeSrch2Server(const ShardId correspondingShardId,
    		const CoreInfo_t * coreInfo);





    DPInternalAPIStatus untrackSrch2Server(boost::shared_ptr<Srch2Server> srch2Server);


private:
    DPInternalAPIStatus bootstrapSrch2Server(boost::shared_ptr<Srch2Server> srch2Server);



    ConfigManager * configurationManager;
    // this lock is used to protect srch2Server maps and other multi-shard-related structures
    boost::shared_mutex isWritableMapLock;
    // map to know if a server should accept insert/delete/update operations or not
    // srch2Server id => is writable, true by defualt which means it accepts all requests
    map< unsigned , bool > srch2ServersIsWritable;

    unsigned maxServerId;
};


}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
