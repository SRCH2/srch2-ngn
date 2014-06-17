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


/*
 * The level of availability of a shard.
 * The order of these enums must be from lowest available to highest available
 */
enum Srch2ServerAccessAvailabilty{
	DPInternal_NonAvailable,
	DPInternal_ReadAvailable,
	DPInternal_ReadWriteAvailable
};

enum DPInternalAPIStatus{
	DPInternal_Srch2ServerNotFound,
	DPInternal_Success
};

class DPInternalRequestHandler;
class Srch2ServerAccess{
	friend class DPInternalRequestHandler;
public:

	Srch2ServerAccess(const ShardId correspondingShardId, CoreInfo_t * coreInfo );
	~Srch2ServerAccess(){};

	boost::shared_ptr<Srch2Server> getSrch2Server(Srch2ServerAccessAvailabilty requestedAvailability, bool & available);

	CoreInfo_t * getCoreInfo();

private:

	void setAvailability(Srch2ServerAccessAvailabilty availability);

	// TODO : just a placeholder for future for now
	ShardId correspondingShardId;
	CoreInfo_t * coreInfo;
	// Srch2Server needs to be shared pointer so that we can easily delete a shard and it gets
	// deallocated when all search requests are gone.
	boost::shared_ptr<Srch2Server> srch2Server;
	mutable boost::shared_mutex availabilityLock;
	// This flag tells us for which operations this pointer is available,
	// for example, when the index is loading, this pointer is not available
	// for any operations. getSrch2Server always
	// checks the request access level less than this flag before returning the pointer.
	Srch2ServerAccessAvailabilty availability;
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
    SearchCommandResults * internalSearchCommand(Srch2ServerHandle serverHandle, SearchCommand * searchData);


    /*
     * This call back is always called for insert and update, it will use
     * internalInsertCommand and internalUpdateCommand
     */
    CommandStatus * internalInsertUpdateCommand(Srch2ServerHandle serverHandle, InsertUpdateCommand * insertUpdateData);

    /*
     * 1. Receives an insert request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this insert query
     * 3. Sends the results to the shard which initiated this insert query (Failure or Success)
     */
    CommandStatus * internalInsertCommand(Srch2ServerHandle serverHandle, InsertUpdateCommand * insertUpdateData);
    /*
     * 1. Receives a update request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this update query
     * 3. Sends the results to the shard which initiated this update request (Failure or Success)
     */
    CommandStatus * internalUpdateCommand(Srch2ServerHandle serverHandle, InsertUpdateCommand * insertUpdateData);

    /*
     * 1. Receives a delete request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this delete query
     * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
     */
    CommandStatus *  internalDeleteCommand(Srch2ServerHandle serverHandle, DeleteCommand * deleteData);



    /*
     * 1. Receives a GetInfo request from a shard
     * 2. Uses core to get info
     * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
     */
    GetInfoCommandResults * internalGetInfoCommand(Srch2ServerHandle serverHandle, GetInfoCommand * getInfoData);


    /*
     * This call back function is called for serialization. It uses internalSerializeIndexCommand
     * and internalSerializeRecordsCommand for our two types of serialization.
     */
    CommandStatus * internalSerializeCommand(Srch2ServerHandle serverHandle, SerializeCommand * seralizeData);

    /*
     * 1. Receives a SerializeIndex request from a shard
     * 2. Uses core to do the serialization
     * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
     */
    CommandStatus * internalSerializeIndexCommand(Srch2ServerHandle serverHandle, SerializeCommand * seralizeData);

    /*
     * 1. Receives a SerializeRecords request from a shard
     * 2. Uses core to do the serialization
     * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
     */
    CommandStatus * internalSerializeRecordsCommand(Srch2ServerHandle serverHandle, SerializeCommand * seralizeData);

    /*
     * 1. Receives a ResetLog request from a shard
     * 2. Uses core to reset log
     * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
     */
    CommandStatus * internalResetLogCommand(Srch2ServerHandle serverHandle, ResetLogCommand * resetData);


    /*
     * Receives a commit command and commits the index
     */
    CommandStatus * internalCommitCommand(Srch2ServerHandle serverHandle, CommitCommand * resetData);


    /*
     * The following methods provide an API to register/allocate/delete/load/create and other operations on
     * indices.
     * NOTE: As of June 16th, since our core codebase is wrapped and accessed from sharding layers through Srch2Server
     * objects, indices and processing are combined in the Srch2Server objects, so DP Internal shouldn't be viewed as
     * an Index Manager. Index Managers tend to be a container for indices while this module is more of a wrapper on the API
     * provided by the core codebase.
     */
    Srch2ServerHandle registerSrch2Server(const ShardId correspondingShardId, CoreInfo_t * coreInfo);


    DPInternalAPIStatus bootstrapSrch2Server(Srch2ServerHandle handle);


    DPInternalAPIStatus deleteSrch2Server(Srch2ServerHandle handle);


private:
    ConfigManager * configurationManager;

    // this lock is used to protect srch2Server maps and other multi-shard-related structures
    boost::shared_mutex globalIndexLock;
    // this map contains the 1-to-1 mapping between Srch2ServerHandles and Srch2ServerAccess objects
    // (actual pointers to the core). This map is updated mostly by Migration Manager and Used by
    // Routing Manager
    map< Srch2ServerHandle , Srch2ServerAccess * > srch2Servers;
};


}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
