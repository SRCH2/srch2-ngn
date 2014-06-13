#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_

#include <string>

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
class DPInternalRequestHandler {

public:
    // Public API which can be used by other modules
    DPInternalRequestHandler(ConfigManager * configurationManager);

    /*
     * 1. Receives a search request from a shard
     * 2. Uses core to evaluate this search query
     * 3. Sends the results to the shard which initiated this search query
     */
    SearchCommandResults * internalSearchCommand(Srch2Server * server, SearchCommand * searchData);


    /*
     * This call back is always called for insert and update, it will use
     * internalInsertCommand and internalUpdateCommand
     */
    CommandStatus * internalInsertUpdateCommand(Srch2Server * server, InsertUpdateCommand * insertUpdateData);

    /*
     * 1. Receives an insert request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this insert query
     * 3. Sends the results to the shard which initiated this insert query (Failure or Success)
     */
    CommandStatus * internalInsertCommand(Srch2Server * server, InsertUpdateCommand * insertUpdateData);
    /*
     * 1. Receives a update request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this update query
     * 3. Sends the results to the shard which initiated this update request (Failure or Success)
     */
    CommandStatus * internalUpdateCommand(Srch2Server * server, InsertUpdateCommand * insertUpdateData);

    /*
     * 1. Receives a delete request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this delete query
     * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
     */
    CommandStatus *  internalDeleteCommand(Srch2Server * server, DeleteCommand * deleteData);



    /*
     * 1. Receives a GetInfo request from a shard
     * 2. Uses core to get info
     * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
     */
    GetInfoCommandResults * internalGetInfoCommand(Srch2Server * server, GetInfoCommand * getInfoData);


    /*
     * This call back function is called for serialization. It uses internalSerializeIndexCommand
     * and internalSerializeRecordsCommand for our two types of serialization.
     */
    CommandStatus * internalSerializeCommand(Srch2Server * server, SerializeCommand * seralizeData);

    /*
     * 1. Receives a SerializeIndex request from a shard
     * 2. Uses core to do the serialization
     * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
     */
    CommandStatus * internalSerializeIndexCommand(Srch2Server * server, SerializeCommand * seralizeData);

    /*
     * 1. Receives a SerializeRecords request from a shard
     * 2. Uses core to do the serialization
     * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
     */
    CommandStatus * internalSerializeRecordsCommand(Srch2Server * server, SerializeCommand * seralizeData);

    /*
     * 1. Receives a ResetLog request from a shard
     * 2. Uses core to reset log
     * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
     */
    CommandStatus * internalResetLogCommand(Srch2Server * server, ResetLogCommand * resetData);


    /*
     * Receives a commit command and commits the index
     */
    CommandStatus * internalCommitCommand(Srch2Server * server, CommitCommand * resetData);


private:
    ConfigManager * configurationManager;
};


}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
