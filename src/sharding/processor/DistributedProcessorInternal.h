#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_


#include "sharding/configuration/ConfigManager.h"
#include "sharding/configuration/CoreInfo.h"
#include "sharding/configuration/ShardingConstants.h"
#include "sharding/sharding/metadata_manager/Shard.h"
#include "sharding/sharding/metadata_manager/Cluster.h"

#include <instantsearch/Record.h>
#include <instantsearch/LogicalPlan.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

#include "serializables/SerializableSearchResults.h"
#include "serializables/SerializableCommandStatus.h"
#include "sharding/sharding/notifications/CommandStatusNotification.h"
#include "sharding/sharding/notifications/CommandNotification.h"
#include "serializables/SerializableGetInfoResults.h"

#include "server/HTTPJsonResponse.h"

using namespace std;
using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class ConfigManager;
class Srch2Server;
class SearchCommand;
class CommandStatus;
class InsertUpdateCommand;
class DeleteCommand;
class GetInfoCommand;



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
    SearchCommandResults * internalSearchCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, SearchCommand * searchData);


    /*
     * This call back is always called for insert and update, it will use
     * internalInsertCommand and internalUpdateCommand
     */
    CommandStatus * internalInsertUpdateCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, InsertUpdateCommand * insertUpdateData);

    /*
     * 1. Receives a delete request from a shard and makes sure this
     *    shard is the correct reponsible of this record using Partitioner
     * 2. Uses core execute this delete query
     * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
     */
    CommandStatus *  internalDeleteCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, DeleteCommand * deleteData);



    /*
     * 1. Receives a GetInfo request from a shard
     * 2. Uses core to get info
     * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
     */
    GetInfoCommandResults * internalGetInfoCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, GetInfoCommand * getInfoData);


    /*
     * This call back function is called for serialization. It uses internalSerializeIndexCommand
     * and internalSerializeRecordsCommand for our two types of serialization.
     */
    SP(CommandStatusNotification) internalSerializeCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
    		ShardCommandCode commandCode, const string & fileName);

    /*
     * 1. Receives a ResetLog request from a shard
     * 2. Uses core to reset log
     * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
     */
    SP(CommandStatusNotification) internalResetLogCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview);


    /*
     * Receives a commit command and commits the index
     */
    SP(CommandStatusNotification) internalCommitCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview);

    SP(CommandStatusNotification) internalMergeCommand(const NodeTargetShardInfo & target,
    		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, bool mainAction = true, bool flagValue = true);


    SP(CommandStatusNotification) resolveShardCommand(SP(CommandNotification) notif);

private:

    ConfigManager * configurationManager;

    struct ShardSearchArgs{
    	ShardSearchArgs(LogicalPlan * logicalPlan, Srch2Server * server, const string & shardIdentifier){
    		this->logicalPlan = logicalPlan;
    		this->server = server;
    		this->shardResults = new SearchCommandResults::ShardResults(shardIdentifier);
    	}
    	~ShardSearchArgs(){
    		if(logicalPlan != NULL){
    			delete logicalPlan;
    		}
    	}
    	LogicalPlan * logicalPlan;
    	Srch2Server * server;
    	SearchCommandResults::ShardResults * shardResults;
    };

    static void * searchInShardThreadWork(void *);


    struct ShardInsertUpdateArgs{
    	ShardInsertUpdateArgs(Record * record, Srch2Server * server, string shardIdentifier):record(record){
    		this->server = server;
    		this->shardResults = new CommandStatus::ShardResults(shardIdentifier);
    	}
    	~ShardInsertUpdateArgs(){
    		if(record != NULL){
    			delete record;
    		}
    	}
    	Record * record;
    	Srch2Server * server;
    	CommandStatus::ShardResults * shardResults;
    };

    struct ShardDeleteArgs{
    	ShardDeleteArgs(const string primaryKey, unsigned shardingKey, Srch2Server * server, string shardIdentifier):primaryKey(primaryKey){
    		this->shardingKey = shardingKey;
    		this->server = server;
    		this->shardResults = new CommandStatus::ShardResults(shardIdentifier);
    	}
    	const string primaryKey;
    	unsigned shardingKey;
    	Srch2Server * server;
    	CommandStatus::ShardResults * shardResults;
    };

    static void insertInShard(const Record * record, Srch2Server * server, Json::Value & messages , bool & statusValue);
    static void * insertInShardThreadWork(void *);
    static void updateInShard(const Record * record, Srch2Server * server, Json::Value & messages , bool & statusValue);
    static void * updateInShardThreadWork(void * args);

    static void deleteInShard(const string primaryKey, unsigned shardingKey,
    		Srch2Server * server, Json::Value & messages, bool & statusValue);
    static void * deleteInShardThreadWork(void * args);

//    GetInfoShardResult
    struct ShardGetInfoArgs{
    	ShardGetInfoArgs(Srch2Server * server,  ShardId * shardId){
    		this->server = server;
    		shardResult = new GetInfoCommandResults::ShardResults(shardId);
    	}
    	Srch2Server * server;
    	GetInfoCommandResults::ShardResults * shardResult ;
    };
    static void getInfoInShard(Srch2Server * server,GetInfoCommandResults::ShardResults * shardResult);
    static void * getInfoInShardThreadWork(void * args);


    struct ShardSerializeArgs{
    	ShardSerializeArgs(const string dataFileName, Srch2Server * server,
    			ShardId * shardIdentifier,
    			const string & shardIndexDirectory):
    				dataFileName(dataFileName),
    				shardIndexDirectory(shardIndexDirectory){
    		this->server = server;
    		this->shardResults = new CommandStatusNotification::ShardStatus(shardIdentifier);
    	}
    	const string dataFileName;
    	const string shardIndexDirectory;
    	Srch2Server * server;
    	CommandStatusNotification::ShardStatus * shardResults;
    };

    static void * serializeIndexInShardThreadWork(void * args);
    static void * serializeRecordsInShardThreadWork(void * args);

	enum MergeActionType{
		DoMerge,
		SetMergeON,
		SetMergeOFF
	};
    struct ShardStatusOnlyArgs{
    	ShardStatusOnlyArgs(Srch2Server * server, ShardId * shardId, MergeActionType mergeAction = DoMerge){
    		this->server = server;
    		this->shardResults = new CommandStatusNotification::ShardStatus(shardId);
    		// 0 is merge
    		// 1 is set ON
    		// 2 is set OFF
    		this->mergeAction = mergeAction;

    	}
    	Srch2Server * server;
    	CommandStatusNotification::ShardStatus * shardResults;
    	MergeActionType mergeAction;
    };

    void handleStatusOnlyCommands();

    static void * resetLogInShardThreadWork(void * args);
    static void * commitInShardThreadWork(void * args);
    static void * mergeInShardThreadWork(void * args);
};


}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSR_INTERNAL_H_
