#include "DistributedProcessorInternal.h"


#include "util/Logger.h"
#include "util/FileOps.h"
#include "server/Srch2Server.h"

#include "serializables/SerializableGetInfoCommandInput.h"
#include "sharding/sharding/notifications/Write2PCNotification.h"
#include "sharding/sharding/metadata_manager/ResourceMetadataManager.h"
#include "QueryExecutor.h"
#include "instantsearch/LogicalPlan.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>
#include <core/util/Version.h>
#include "wrapper/ParsedParameterContainer.h"
#include "sharding/sharding/ShardManager.h"
#include "index/FeedbackIndex.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {



// DPInternalRequestHandler

DPInternalRequestHandler::DPInternalRequestHandler(ConfigManager * configurationManager){
    this->configurationManager = configurationManager;
}
/*
 * 1. Receives a search request from a shard
 * 2. Uses core to evaluate this search query
 * 3. Sends the results to the shard which initiated this search query
 */
SP(SearchCommandResults) DPInternalRequestHandler::internalSearchCommand(SP(SearchCommand) command){

	const NodeTargetShardInfo & target = command->getTarget();
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview = command->getReadview();
	LogicalPlan * logicalPlan = command->getLogicalPlan();
	if(logicalPlan == NULL){
		return SP(SearchCommandResults)();
	}
	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);
	if(shards.empty()){
		return SP(SearchCommandResults)();
	}
	// 1. create multiple threads to take care of each shard in target.
	vector<ShardSearchArgs *> allShardsSearchArguments;
    pthread_t * shardSearchThreads = new pthread_t[shards.size()];
	struct timespec * tstarts = new timespec[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		//// !!!!!!! WARNING !!!!!!! : if these is a bug it might be because of not cloning logical plan
		ShardSearchArgs * shardSearchArgs = new ShardSearchArgs(logicalPlan, // new LogicalPlan(*logicalPlan)
				shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsSearchArguments.push_back(shardSearchArgs);
		clock_gettime(CLOCK_REALTIME, &tstarts[shardIdx]);
		// 1. give these structures to server along with a copy of logical plan so that
		// it finds the results and gives us the query results in these structures
	    if (pthread_create(&shardSearchThreads[shardIdx], NULL, searchInShardThreadWork , shardSearchArgs) != 0){
	        perror("Cannot create thread for handling local message");
	        return SP(SearchCommandResults)();
	    }
	}

	//2. When all threads are done, aggregate QueryResult objects and in-memory strings
	//        and put them in a new SearchCommand object;
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardSearchThreads[shardIdx], NULL);
		struct timespec tend;
		clock_gettime(CLOCK_REALTIME, &tend);
	    unsigned ts1 = (tend.tv_sec - tstarts[shardIdx].tv_sec) * 1000
	            + (tend.tv_nsec - tstarts[shardIdx].tv_nsec) / 1000000;
	    allShardsSearchArguments.at(shardIdx)->shardResults->searcherTime = ts1;
	}
	delete [] tstarts;
	delete [] shardSearchThreads;
	// at this point all shard searches are done.
	// aggregate query results and etc ...
    // search results to be serialized and sent over the network
    SP(SearchCommandResults) searchResults = SP(SearchCommandResults)(new SearchCommandResults());
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		ShardSearchArgs * shardResultsArgs = allShardsSearchArguments.at(shardIdx);
		searchResults->addShardResults(shardResultsArgs->shardResults);
		delete shardResultsArgs;
	}

    return searchResults;

}

void * DPInternalRequestHandler::searchInShardThreadWork(void * args){
	ShardSearchArgs * shardSearchArgs = (ShardSearchArgs *) args;

    // search in core
    // TODO : is it possible to make executor and planGen singleton ?
    const CoreInfo_t *indexDataContainerConf = shardSearchArgs->server->getCoreInfo();

    /*
     * TODO : we can remove this copy in future to improve the performance.
     */
    LogicalPlan logicalPlanClone(*(shardSearchArgs->logicalPlan));
    QueryExecutor qe(logicalPlanClone, &(shardSearchArgs->shardResults->resultsFactory),
    		shardSearchArgs->server , indexDataContainerConf);
    // in here just allocate an empty QueryResults object, it will be initialized in execute.
    qe.execute(&(shardSearchArgs->shardResults->queryResults));
	//
	return NULL;
}

void DPInternalRequestHandler::insertInShard(const Record * record,
		Srch2Server * server, vector<JsonMessageCode> & messageCodes , bool & statusValue){
    //add the record to the index
    if ( server->getIndexer()->getNumberOfDocumentsInIndex() < server->getCoreInfo()->getDocumentLimit() )
    {
        // Do NOT delete analyzer because it is thread specific. It will be reused for
        // search/update/delete operations.
        srch2::instantsearch::Analyzer * analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->getCoreInfo());
        srch2::instantsearch::INDEXWRITE_RETVAL ret = server->getIndexer()->addRecord(record, analyzer);

        switch( ret )
        {
        case srch2::instantsearch::OP_SUCCESS:
        {
            statusValue = true;
            return;
        }
        case srch2::instantsearch::OP_FAIL:
        {
            messageCodes.push_back(HTTP_JSON_PK_Exists_Error);
            statusValue = false;
            return;
        }
        default:
        	ASSERT(false);
        	return;
        };
    }
    else
    {
        messageCodes.push_back(HTTP_JSON_Doc_Limit_Reached_Error);
        statusValue = false;
        return;
    }
}

void DPInternalRequestHandler::insertInShardTest(const string & primaryKey, Srch2Server * server,
		vector<JsonMessageCode> & messageCodes , bool & statusValue){
    if ( server->getIndexer()->getNumberOfDocumentsInIndex() < server->getCoreInfo()->getDocumentLimit() )
    {
    	statusValue = true;
    }else{
        messageCodes.push_back(HTTP_JSON_Doc_Limit_Reached_Error);
        statusValue = false;
    }
}

void DPInternalRequestHandler::updateInShard(const Record * record,
		Srch2Server * server, vector<JsonMessageCode> & messageCodes , bool & statusValue){
	unsigned deletedInternalRecordId;
	std::string primaryKeyStringValue;
	primaryKeyStringValue = record->getPrimaryKey();

	//delete the record from the index
	bool recordExisted = false;
	switch(server->getIndexer()->deleteRecordGetInternalId(primaryKeyStringValue, deletedInternalRecordId))
	{
	case srch2::instantsearch::OP_FAIL:
	{
		// record to update doesn't exit, will insert it
		break;
	}
	default: // OP_SUCCESS.
	{
		recordExisted = true;
		break;
	}
	};


	/// step 2, insert new record

	//add the record to the index

	if ( server->getIndexer()->getNumberOfDocumentsInIndex() < server->getCoreInfo()->getDocumentLimit() )
	{
		// Do NOT delete analyzer because it is thread specific. It will be reused for
		// search/update/delete operations.
		Analyzer* analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->getCoreInfo());
		srch2::instantsearch::INDEXWRITE_RETVAL ret = server->getIndexer()->addRecord(record, analyzer);
		switch( ret )
		{
		case srch2::instantsearch::OP_SUCCESS:
		{
			if (! recordExisted){
		        messageCodes.push_back(HTTP_JSON_Existing_Record_Update_Info);
			}

			statusValue = true;
			return;
		}
		case srch2::instantsearch::OP_FAIL:
		{
	        messageCodes.push_back(HTTP_JSON_Update_Failed_Error);
			statusValue = false;
			break;
		}
		default:
			ASSERT(false);
			break;
		};
	}
	else
	{
        messageCodes.push_back(HTTP_JSON_Update_Failed_Error);
		statusValue = false;
	}

	/// reaching here means the insert failed, need to resume the deleted old record

	srch2::instantsearch::INDEXWRITE_RETVAL ret = server->getIndexer()->recoverRecord(primaryKeyStringValue, deletedInternalRecordId);

	switch ( ret )
	{
	case srch2::instantsearch::OP_FAIL:
	{
        messageCodes.push_back(HTTP_JSON_Update_Failed_Error);
		statusValue = false;
		return;
	}
	default: // OP_SUCCESS.
	{
        messageCodes.push_back(HTTP_JSON_Update_Failed_Error);
		statusValue = false;
		return;
	}
	};

}


void DPInternalRequestHandler::deleteInShard(const string primaryKey,
		Srch2Server * server, vector<JsonMessageCode> & messageCodes, bool & statusValue){
    //delete the record from the index
    switch(server->getIndexer()->deleteRecord(primaryKey)){
    case OP_FAIL:
    {
        messageCodes.push_back(HTTP_JSON_Delete_Record_Not_Found_Error);
        statusValue = false;
        return;
    }
    default: // OP_SUCCESS.
    {
        statusValue = true;
        return;
    }
    };
}

/*
 * 1. Receives a GetInfo request from a shard
 * 2. Uses core to get info
 * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
 */
GetInfoCommandResults * DPInternalRequestHandler::internalGetInfoCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, GetInfoCommand * getInfoData){


	if(getInfoData == NULL){
        GetInfoCommandResults * getInfoResult =
                new GetInfoCommandResults();
        return getInfoResult;
    }

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardGetInfoArgs *> allShardsGetInfoArguments;
    pthread_t * shardGetInfoThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardGetInfoArgs * shardGetInfoArgs = new ShardGetInfoArgs(shard->getSrch2Server().get(), shard->cloneShardId());
		allShardsGetInfoArguments.push_back(shardGetInfoArgs);
		if (pthread_create(&shardGetInfoThreads[shardIdx], NULL, getInfoInShardThreadWork , shardGetInfoArgs) != 0){
			perror("Cannot create thread for handling local message");
			return NULL;
		}
	}

	GetInfoCommandResults * getInfoResult = new GetInfoCommandResults();

	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardGetInfoThreads[shardIdx], NULL);
		ShardGetInfoArgs * getInfoResults = allShardsGetInfoArguments.at(shardIdx);
		getInfoResults->shardResult->versionInfo = Version::getCurrentVersion();
		getInfoResult->addShardResults(getInfoResults->shardResult);
		delete getInfoResults;
	}
	delete shardGetInfoThreads;

    return getInfoResult;
}


void DPInternalRequestHandler::getInfoInShard(Srch2Server * server,GetInfoCommandResults::ShardResults * shardResult){
    server->getIndexer()->getIndexHealthObj(shardResult->healthInfo);
}
void * DPInternalRequestHandler::getInfoInShardThreadWork(void * args){
	ShardGetInfoArgs * infoArgs = (ShardGetInfoArgs *)args;
	getInfoInShard(infoArgs->server, infoArgs->shardResult);
	//
	return NULL;
}

/*
 * This call back function is called for serialization. It uses internalSerializeIndexCommand
 * and internalSerializeRecordsCommand for our two types of serialization.
 */
SP(CommandStatusNotification) DPInternalRequestHandler::internalSerializeCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
		ShardCommandCode commandCode, const string & fileName){

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardSerializeArgs *> allShardsSerializeArguments;
    pthread_t * shardSerializeThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardSerializeArgs * shardSerializeArgs =
				new ShardSerializeArgs(fileName,
						shard->getSrch2Server().get(), shard->cloneShardId(), shard->getIndexDirectory());
		allShardsSerializeArguments.push_back(shardSerializeArgs);

		if(commandCode == ShardCommandCode_SaveData || commandCode == ShardCommandCode_SaveData_SaveMetadata){ // insert case
			if (pthread_create(&shardSerializeThreads[shardIdx], NULL, serializeIndexInShardThreadWork , shardSerializeArgs) != 0){
				perror("Cannot create thread for handling local message");
				return SP(CommandStatusNotification)();
			}
		}else{
			ASSERT(commandCode == ShardCommandCode_Export);
			if (pthread_create(&shardSerializeThreads[shardIdx], NULL, serializeRecordsInShardThreadWork , shardSerializeArgs) != 0){
				perror("Cannot create thread for handling local message");
				return SP(CommandStatusNotification)();
			}
		}
	}
    SP(CommandStatusNotification) status ;
	if(commandCode == ShardCommandCode_SaveData || commandCode == ShardCommandCode_SaveData_SaveMetadata){ // insert case
		status = SP(CommandStatusNotification)( new CommandStatusNotification(commandCode));
		Logger::info("%s", "{\"save\":\"success\"}");
	}else{
		status = SP(CommandStatusNotification)( new CommandStatusNotification(commandCode));
		Logger::info("%s", "{\"export\":\"success\"}");
	}

	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardSerializeThreads[shardIdx], NULL);
		ShardSerializeArgs * serializeResults = allShardsSerializeArguments.at(shardIdx);
		status->addShardResult(serializeResults->shardResults);
		delete serializeResults;
	}
	delete shardSerializeThreads;
    return status;
}


void * DPInternalRequestHandler::serializeIndexInShardThreadWork(void * args){
	ShardSerializeArgs * serializeIndexArgs = (ShardSerializeArgs *) args;
	serializeIndexArgs->server->getIndexer()->save();
	serializeIndexArgs->shardResults->setStatusValue(true);
    //
    return NULL;
}

void * DPInternalRequestHandler::serializeRecordsInShardThreadWork(void * args){
	ShardSerializeArgs * serializeRecordsArgs = (ShardSerializeArgs *) args;
    string exportedDataFileName = serializeRecordsArgs->shardIndexDirectory + "/" + serializeRecordsArgs->dataFileName;
    if(srch2::util::checkDirExistence(exportedDataFileName.c_str())){
        exportedDataFileName = serializeRecordsArgs->shardIndexDirectory + "/" + string("export_data.json");
    }
    serializeRecordsArgs->server->getIndexer()->exportData(exportedDataFileName);
    serializeRecordsArgs->shardResults->setStatusValue(true);
    //
    return NULL;
}

/*
 * 1. Receives a ResetLog request from a shard
 * 2. Uses core to reset log
 * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
 */
SP(CommandStatusNotification) DPInternalRequestHandler::internalResetLogCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, const string & newLogFilePath){

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardStatusOnlyArgs *> allShardsResetLogsArguments;
    pthread_t * shardResetLogsThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardStatusOnlyArgs * shardResetLogsArgs =
				new ShardStatusOnlyArgs(shard->getSrch2Server().get(), shard->cloneShardId());
		shardResetLogsArgs->newLogFileName = newLogFilePath;
		allShardsResetLogsArguments.push_back(shardResetLogsArgs);

		if (pthread_create(&shardResetLogsThreads[shardIdx], NULL, resetLogInShardThreadWork, shardResetLogsArgs) != 0){
			perror("Cannot create thread for handling local message");
			return SP(CommandStatusNotification)();
		}
	}

    SP(CommandStatusNotification) status = SP(CommandStatusNotification)(new CommandStatusNotification(ShardCommandCode_ResetLogger));

    for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardResetLogsThreads[shardIdx], NULL);
		ShardStatusOnlyArgs * resetResults = allShardsResetLogsArguments.at(shardIdx);
		status->addShardResult(resetResults->shardResults);
		delete resetResults;
	}
	delete shardResetLogsThreads;

    return status;


}


SP(CommandStatusNotification) DPInternalRequestHandler::internalCommitCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardStatusOnlyArgs *> allShardsCommitArguments;
    pthread_t * shardCommitThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardStatusOnlyArgs * shardCommitArgs =
				new ShardStatusOnlyArgs(shard->getSrch2Server().get(), shard->cloneShardId());
		allShardsCommitArguments.push_back(shardCommitArgs);

		if (pthread_create(&shardCommitThreads[shardIdx], NULL, commitInShardThreadWork, shardCommitArgs) != 0){
			perror("Cannot create thread for handling local message");
			return SP(CommandStatusNotification)();
		}
	}

	SP(CommandStatusNotification) status = SP(CommandStatusNotification)(new CommandStatusNotification(ShardCommandCode_Commit));

    for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardCommitThreads[shardIdx], NULL);
		ShardStatusOnlyArgs * commitResults = allShardsCommitArguments.at(shardIdx);
		status->addShardResult(commitResults->shardResults);
		delete commitResults;
	}
	delete shardCommitThreads;

    return status;


}


SP(CommandStatusNotification) DPInternalRequestHandler::internalMergeCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, bool mainAction , bool flagValue ){

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardStatusOnlyArgs *> allShardsMergeArguments;
    pthread_t * shardMergeThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		MergeActionType mergeAction = DoMerge;
		if(! mainAction){
			if(flagValue){
				mergeAction = SetMergeON;
			}else{
				mergeAction = SetMergeOFF;
			}
		}
		ShardStatusOnlyArgs * shardMergeArgs =
				new ShardStatusOnlyArgs(shard->getSrch2Server().get(), shard->cloneShardId(), mergeAction);
		allShardsMergeArguments.push_back(shardMergeArgs);

		if (pthread_create(&shardMergeThreads[shardIdx], NULL, mergeInShardThreadWork, shardMergeArgs) != 0){
			perror("Cannot create thread for handling local message");
			return SP(CommandStatusNotification)();
		}
	}

	SP(CommandStatusNotification) status = SP(CommandStatusNotification)(new CommandStatusNotification(ShardCommandCode_Merge));

    for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardMergeThreads[shardIdx], NULL);
		ShardStatusOnlyArgs * mergeResults = allShardsMergeArguments.at(shardIdx);
		status->addShardResult(mergeResults->shardResults);
		delete mergeResults;
	}
	delete shardMergeThreads;

    return status;


}


void * DPInternalRequestHandler::resetLogInShardThreadWork(void * args){
	ShardStatusOnlyArgs * shardArgs = (ShardStatusOnlyArgs * )args;
	string newLogFilePath = shardArgs->server->getCoreInfo()->getNewHTTPServerAccessLogFile(shardArgs->newLogFileName);
    // create a FILE* pointer to point to the new logger file "logger.txt"
    FILE *logFile = fopen(newLogFilePath.c_str(),"a");

    if (logFile == NULL) {
        srch2::util::Logger::error("Reopen Log file %s failed.",
        		shardArgs->server->getCoreInfo()->getHTTPServerAccessLogFile().c_str());

        Json::Value errValue = JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_ResetLogger_Reopen_Failed_Error);
        errValue[c_logger_file] = shardArgs->server->getCoreInfo()->getHTTPServerAccessLogFile();
        shardArgs->shardResults->messages.append(errValue);
        shardArgs->shardResults->setStatusValue(false);
        Logger::info("%s", global_customized_writer.write(shardArgs->shardResults->messages).c_str());
    } else {
        FILE * oldLogger = srch2::util::Logger::swapLoggerFile(logFile);
        fclose(oldLogger);
        shardArgs->shardResults->setStatusValue(true);
    }

	//
	return NULL;

}
void * DPInternalRequestHandler::commitInShardThreadWork(void * args){
	ShardStatusOnlyArgs * shardArgs = (ShardStatusOnlyArgs * )args;

    //commit the index.
	INDEXWRITE_RETVAL commitReturnValue = shardArgs->server->getIndexer()->commit();
    if ( commitReturnValue == srch2::instantsearch::OP_SUCCESS)
    {
        shardArgs->shardResults->setStatusValue(true);
    }
    else if(commitReturnValue == srch2::instantsearch::OP_NOTHING_TO_DO)
    {
        Json::Value infoValue = JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Commit_Already_Done_Info);
        shardArgs->shardResults->messages.append(infoValue);
    	shardArgs->shardResults->setStatusValue(true);
    }
    else
    {
         shardArgs->shardResults->setStatusValue(false);
    }

	//
	return NULL;
}


void * DPInternalRequestHandler::mergeInShardThreadWork(void * args){
	ShardStatusOnlyArgs * shardArgs = (ShardStatusOnlyArgs * )args;

	if(shardArgs->mergeAction != DoMerge){
		if(shardArgs->mergeAction == SetMergeON){
			shardArgs->server->getIndexer()->enableMerge();
		}else if (shardArgs->mergeAction == SetMergeOFF){
			shardArgs->server->getIndexer()->disableMerge();
		}
        shardArgs->shardResults->setStatusValue(true);
		return NULL;
	}
    //merge the index.
	INDEXWRITE_RETVAL mergeReturnValue = shardArgs->server->getIndexer()->merge();
    if ( mergeReturnValue == srch2::instantsearch::OP_SUCCESS)
    {
        shardArgs->shardResults->setStatusValue(true);
    }
    else if(mergeReturnValue == srch2::instantsearch::OP_NOTHING_TO_DO)
    {
        Json::Value infoValue = JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Merge_Already_Done_Info);
        shardArgs->shardResults->messages.append(infoValue);
    	shardArgs->shardResults->setStatusValue(true);
    }
    else
    {
        Json::Value infoValue = JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Merge_DISABLED);
        shardArgs->shardResults->messages.append(infoValue);
         shardArgs->shardResults->setStatusValue(false);
    }

	//
	return NULL;
}

SP(CommandStatusNotification) DPInternalRequestHandler::resolveShardCommand(SP(CommandNotification) notif){
	if(! notif){
		return SP(CommandStatusNotification)(new CommandStatusNotification(ShardCommandCode_Merge));
	}
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview = notif->getReadview();
	switch (notif->getCommandCode()) {
		case ShardCommandCode_SaveData_SaveMetadata:
		case ShardCommandCode_SaveData:
			Logger::sharding(Logger::Step, "DP-Internal| Saving the shards on disk ... ");
			return internalSerializeCommand(notif->getTarget(), clusterReadview, notif->getCommandCode(), notif->getNewLogFilePath());
		case ShardCommandCode_SaveMetadata:
			Logger::sharding(Logger::Step, "DP-Internal| Saving the metadata on disk ... ");
			ShardManager::getShardManager()->getMetadataManager()->saveMetadata(ShardManager::getShardManager()->getConfigManager());
			return SP(CommandStatusNotification)(new CommandStatusNotification(notif->getCommandCode()));
		case ShardCommandCode_Export:
			Logger::sharding(Logger::Step, "DP-Internal| Exporting shards ...");
			return internalSerializeCommand(notif->getTarget(), clusterReadview, notif->getCommandCode(), notif->getNewLogFilePath());
		case ShardCommandCode_Commit:
			Logger::sharding(Logger::Step, "DP-Internal| Committing shards ...");
			return internalCommitCommand(notif->getTarget(), clusterReadview);
		case ShardCommandCode_Merge:
			Logger::sharding(Logger::Step, "DP-Internal| Doing merge operation");
			return internalMergeCommand(notif->getTarget(), clusterReadview);
		case ShardCommandCode_MergeSetOn:
			Logger::sharding(Logger::Step, "DP-Internal| Setting merge flag to ON");
			return internalMergeCommand(notif->getTarget(), clusterReadview, false, true);
		case ShardCommandCode_MergeSetOff:
			Logger::sharding(Logger::Step, "DP-Internal| Setting merge flag to OFF");
			return internalMergeCommand(notif->getTarget(), clusterReadview, false, false);
		case ShardCommandCode_ResetLogger:
			Logger::sharding(Logger::Step, "DP-Internal| Resetting logger");
			return internalResetLogCommand(notif->getTarget(), clusterReadview, notif->getNewLogFilePath());
	}
	ASSERT(false);
	return SP(CommandStatusNotification)(); // NULL

}

SP(Write2PCNotification::ACK) DPInternalRequestHandler::resolveWrite2PC(SP(Write2PCNotification) notif){


	SP(Write2PCNotification::ACK) result = ShardingNotification::create<Write2PCNotification::ACK>();

	vector<RecordWriteOpHandle *> & recordHanldes = notif->getRecordHandles();

	for(unsigned recIdx = 0; recIdx < recordHanldes.size(); ++recIdx){
		const string & primaryKey = recordHanldes.at(recIdx)->getPrimaryKey();
		Record * recordCloneObj = NULL;
		if(notif->shouldPerformWrite() &&
				(notif->getCommandType() == Insert_ClusterRecordOperation_Type
						|| notif->getCommandType() == Update_ClusterRecordOperation_Type)){
			recordCloneObj = new Record(*(recordHanldes.at(recIdx)->getRecordObj()));
		}
		vector<string> roleIds;
		if(notif->shouldPerformWrite() &&
				(notif->getCommandType() == AclRecordAdd_ClusterRecordOperation_Type
						|| notif->getCommandType() == AclRecordAppend_ClusterRecordOperation_Type
						|| notif->getCommandType() == AclRecordDelete_ClusterRecordOperation_Type
						|| notif->getCommandType() == AclAttrAppend_ClusterRecordOperation_Type
						|| notif->getCommandType() == AclAttrDelete_ClusterRecordOperation_Type
						|| notif->getCommandType() == AclAttrReplace_ClusterRecordOperation_Type)){
			roleIds = recordHanldes.at(recIdx)->getRoleIds();
		}
		vector<string> queries;
		if(notif->shouldPerformWrite() &&
				(notif->getCommandType() == AclRecordAdd_ClusterRecordOperation_Type)) {
			queries = recordHanldes.at(recIdx)->getRoleIds(); // roleIds is actually queries.
		}

		if(primaryKey == ""){
			return SP(Write2PCNotification::ACK)();
		}
		// 0. use target and readview to get Srch2Server pointers
		vector<const Shard *> shards;
		notif->getClusterReadview()->getLocalShardContainer(notif->getTargets().getCoreId())->getShards(notif->getTargets(),shards);
		// prepare the result containers
		for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
			const Shard * shard = shards.at(shardIdx);
			Write2PCNotification::ACK::ShardResult * shardPKResult =
					new Write2PCNotification::ACK::ShardResult(shard->cloneShardId(), false, vector<JsonMessageCode>());
			switch (notif->getCommandType()) {
			case Insert_ClusterRecordOperation_Type:
			{
				if(notif->shouldPerformWrite()){
					insertInShard(recordCloneObj, shard->getSrch2Server().get(), shardPKResult->messageCodes, shardPKResult->statusValue);
				}else{
					insertInShardTest(primaryKey, shard->getSrch2Server().get(), shardPKResult->messageCodes, shardPKResult->statusValue);
				}
				break;
			}
			case Update_ClusterRecordOperation_Type:
			{
				if(notif->shouldPerformWrite()){
					updateInShard(recordCloneObj, shard->getSrch2Server().get(), shardPKResult->messageCodes, shardPKResult->statusValue);
				}else{
					updateInShardTest(primaryKey, shard->getSrch2Server().get(), shardPKResult->messageCodes, shardPKResult->statusValue);
				}
				break;
			}
			case Delete_ClusterRecordOperation_Type:
				if(notif->shouldPerformWrite()){
					deleteInShard(primaryKey ,shard->getSrch2Server().get(), shardPKResult->messageCodes, shardPKResult->statusValue);
				}else{
					deleteInShardTest(primaryKey, shard->getSrch2Server().get(), shardPKResult->messageCodes, shardPKResult->statusValue);
				}
				break;
			case AclRecordAdd_ClusterRecordOperation_Type:
				/*
				 * NOTE :
				 * Here we can use "primaryKey" and "roleIds"
				 * Srch2Server * object : shard->getSrch2Server().get()
				 * Output log messages : must be pushed back to shardPKResult->messageCodes
				 * The final boolean result of this primary key : shardPKResult->statusValue
				 */
				if(notif->shouldPerformWrite()){
					_aclRecordModifyRoles(shard->getSrch2Server()->getIndexer(),
							primaryKey, roleIds, Acl_Record_Add, shardPKResult->messageCodes,
							shardPKResult->statusValue);
				}else{
					shardPKResult->statusValue = true;
				}
				break;
			case AclRecordAppend_ClusterRecordOperation_Type:
				if(notif->shouldPerformWrite()){
					_aclRecordModifyRoles(shard->getSrch2Server()->getIndexer(),
							primaryKey, roleIds, Acl_Record_Append, shardPKResult->messageCodes,
							shardPKResult->statusValue);
				}else{
					shardPKResult->statusValue = true;
				}
				break;
			case AclRecordDelete_ClusterRecordOperation_Type:
				if(notif->shouldPerformWrite()){
					_aclRecordModifyRoles(shard->getSrch2Server()->getIndexer(),
							primaryKey, roleIds, Acl_Record_Delete, shardPKResult->messageCodes,
							shardPKResult->statusValue);
				}else{
					shardPKResult->statusValue = true;
				}
				break;
			case AclAttrReplace_ClusterRecordOperation_Type:
				if(notif->shouldPerformWrite()){
					attributeAclModify(shard->getSrch2Server()->getIndexer(),
							primaryKey, roleIds, ACL_REPLACE, shardPKResult->messageCodes,
							shardPKResult->statusValue);
				}else{
					shardPKResult->statusValue = true;
				}
				break;
			case AclAttrDelete_ClusterRecordOperation_Type:
				if(notif->shouldPerformWrite()){
					attributeAclModify(shard->getSrch2Server()->getIndexer(),
							primaryKey, roleIds, ACL_DELETE, shardPKResult->messageCodes,
							shardPKResult->statusValue);
				}else{
					shardPKResult->statusValue = true;
				}
				break;
			case AclAttrAppend_ClusterRecordOperation_Type:
				if(notif->shouldPerformWrite()){
					attributeAclModify(shard->getSrch2Server()->getIndexer(),
							primaryKey, roleIds, ACL_APPEND, shardPKResult->messageCodes,
							shardPKResult->statusValue);
				}else{
					shardPKResult->statusValue = true;
				}
				break;
			case Feedback_ClusterRecordOperation_Type:
				if(notif->shouldPerformWrite()){
					addFeedback(shard->getSrch2Server()->getIndexer(),
							primaryKey, queries, shardPKResult->messageCodes,
							shardPKResult->statusValue);
				}else{
					shardPKResult->statusValue = true;
				}
				break;
			}
			result->addPrimaryKeyShardResult(primaryKey, shardPKResult);
		}
		if(recordCloneObj != NULL){
			delete recordCloneObj;
		}
	}

	return result;
}

void DPInternalRequestHandler::_aclRecordModifyRoles(Indexer *indexer,
		const string &primaryKeyID, vector<string> &roleIds,
		srch2::instantsearch::RecordAclCommandType commandType,
		vector<JsonMessageCode> & messageCodes, bool & statusValue) {

	srch2::instantsearch::INDEXWRITE_RETVAL returnValue = indexer->aclRecordModifyRoles(
			primaryKeyID, roleIds, commandType);

	switch (commandType) {
	case srch2::instantsearch::Acl_Record_Add:
		switch (returnValue) {
		case srch2::instantsearch::OP_SUCCESS:
			statusValue = true;
			break;
		case srch2::instantsearch::OP_FAIL:
			statusValue = false;
			messageCodes.push_back(HTTP_JSON_PK_Does_Not_Exist);
			break;
		default :
			ASSERT(false);
			break;
		}
		;
		break;
		case srch2::instantsearch::Acl_Record_Append:
			switch (returnValue) {
			case srch2::instantsearch::OP_SUCCESS:
				statusValue = true;
				break;
			case srch2::instantsearch::OP_FAIL:
				statusValue = false;
				messageCodes.push_back(HTTP_JSON_PK_Does_Not_Exist);
				break;
			default :
				ASSERT(false);
				break;
			}
			;
			break;
			case srch2::instantsearch::Acl_Record_Delete:
				switch (returnValue) {
				case srch2::instantsearch::OP_SUCCESS:
					statusValue =  true;
					break;
				case srch2::instantsearch::OP_FAIL:
					statusValue = false;
					messageCodes.push_back(HTTP_JSON_PK_Does_Not_Exist);
					break;
				default :
					ASSERT(false);
					break;
				}
				;
				break;
				default:
					ASSERT(false);
					break;
	};
}

void DPInternalRequestHandler::attributeAclModify(Indexer *indexer,
		const string &roleId, const vector<string> &attributes,
		AclActionType commandType,
		vector<JsonMessageCode> & messageCodes, bool & statusValue) {

	vector<string> roleIds;
	roleIds.push_back(roleId);
	vector<string> attributeList = attributes;
	indexer->getAttributeAcl().processAclRequest(attributeList, roleIds, commandType);
	statusValue = true;
}

void DPInternalRequestHandler::addFeedback(Indexer *indexer,
		const string &externalRecordId, const vector<string> &queries,
		vector<JsonMessageCode> & messageCodes, bool & statusValue) {

	INDEXLOOKUP_RETVAL retVal = indexer->lookupRecord(externalRecordId);
	if (retVal == LU_PRESENT_IN_READVIEW_AND_WRITEVIEW) {
		for (unsigned i = 0; i < queries.size(); ++i) {
			indexer->getFeedbackIndexer()->addFeedback(queries[i], externalRecordId, time(NULL));
		}
		statusValue = true;
	} else {
		messageCodes.push_back(HTTP_JSON_PK_Does_Not_Exist);
		statusValue = false;
	}
}

SP(AclAttributeReadNotification::ACK) DPInternalRequestHandler::
		resolveAclAttributeListRead(SP(AclAttributeReadNotification) notif){
	if(! notif){
		return SP(AclAttributeReadNotification::ACK)();
	}
	AclAttributeReadNotification * aclNotif = notif.get();

	AclAttributeReadNotification::ACK * response =  new AclAttributeReadNotification::ACK();

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	notif->getReadview()->getLocalShardContainer(notif->getTarget().getCoreId())->getShards(notif->getTarget(),shards);
	// prepare the result containers
	const string& roleId = notif->getRoleId();
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		const AttributeAccessControl & acl = shard->getSrch2Server()->getIndexer()->getAttributeAcl();
		boost::shared_ptr<vector<unsigned> > attrList;
		acl.fetchRefiningAttrsAcl(roleId, attrList);
		response->setListOfRefiningAttributes(*attrList.get());
		attrList.reset();
		acl.fetchSearchableAttrsAcl(roleId, attrList);
		response->setListOfSearchableAttributes(*attrList.get());

		break; // only one target is enough, it gives us the list
	}

	return SP(AclAttributeReadNotification::ACK)(response);
}

SP(AclAttributeReplaceNotification::ACK) DPInternalRequestHandler::
		resolveAclAttributeReplaceDeletePhase(SP(AclAttributeReplaceNotification) notif){
	const CoreInfo_t * coreInfo = notif->getCoreInfo();
	if(coreInfo == NULL){
		return SP(AclAttributeReplaceNotification::ACK)();
	}
	// prepare shards which are all shards in this node for this core.
	const LocalShardContainer * localContainer = notif->getReadview()->getLocalShardContainer(coreInfo->getCoreId());
	vector<const Shard *> allLocalShards;
	localContainer->getAllShards(allLocalShards);
	for(unsigned shardIdx = 0; shardIdx < allLocalShards.size(); ++shardIdx){
		const Shard * shard = allLocalShards.at(shardIdx);
		//TODO shard->getSrch2Server() gives the shared pointer to srch2Server object
	}


	//
	return SP(AclAttributeReplaceNotification::ACK)();
}

}
}
