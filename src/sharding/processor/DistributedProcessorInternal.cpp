#include "DistributedProcessorInternal.h"


#include "util/Logger.h"
#include "util/FileOps.h"
#include "server/Srch2Server.h"

#include "serializables/SerializableInsertUpdateCommandInput.h"
#include "serializables/SerializableDeleteCommandInput.h"
#include "serializables/SerializableSearchCommandInput.h"
#include "serializables/SerializableGetInfoCommandInput.h"
#include "sharding/sharding/metadata_manager/ResourceMetadataManager.h"
#include "QueryExecutor.h"
#include "instantsearch/LogicalPlan.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>
#include <core/util/Version.h>
#include "ServerHighLighter.h"
#include "wrapper/ParsedParameterContainer.h"
#include "sharding/sharding/ShardManager.h"

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
SearchCommandResults * DPInternalRequestHandler::internalSearchCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, SearchCommand * searchData){

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);
	// 1. create multiple threads to take care of each shard in target.
	vector<ShardSearchArgs *> allShardsSearchArguments;
    pthread_t * shardSearchThreads = new pthread_t[shards.size()];
	struct timespec * tstarts = new timespec[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardSearchArgs * shardSearchArgs = new ShardSearchArgs(new LogicalPlan(*(searchData->getLogicalPlan())),
				shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsSearchArguments.push_back(shardSearchArgs);
		clock_gettime(CLOCK_REALTIME, &tstarts[shardIdx]);
		// 1. give these structures to server along with a copy of logical plan so that
		// it finds the results and gives us the query results in these structures
	    if (pthread_create(&shardSearchThreads[shardIdx], NULL, searchInShardThreadWork , shardSearchArgs) != 0){
	        perror("Cannot create thread for handling local message");
	        return NULL;
	    }
	}
	delete tstarts;

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
	delete shardSearchThreads;
	// at this point all shard searches are done.
	// aggregate query results and etc ...
    // search results to be serialized and sent over the network
    SearchCommandResults * searchResults = new SearchCommandResults();
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
    QueryExecutor qe(*(shardSearchArgs->logicalPlan), &(shardSearchArgs->shardResults->resultsFactory), shardSearchArgs->server , indexDataContainerConf);
    // in here just allocate an empty QueryResults object, it will be initialized in execute.
    qe.executeForDPInternal(&(shardSearchArgs->shardResults->queryResults), shardSearchArgs->shardResults->inMemoryRecordStrings);

    if (shardSearchArgs->server->getCoreInfo()->getHighlightAttributeIdsVector().size() > 0 ) {
    	    //TODO: V1 we need these two parameters in DP internal
    		//!paramContainer.onlyFacets &&
    		//paramContainer.isHighlightOn) {
    	ParsedParameterContainer paramContainer; // temp for V0

    	QueryResults *finalResults = &(shardSearchArgs->shardResults->queryResults);
    	ServerHighLighter highlighter =  ServerHighLighter(finalResults, shardSearchArgs->server, paramContainer,
    			shardSearchArgs->logicalPlan->getOffset(), shardSearchArgs->logicalPlan->getNumberOfResultsToRetrieve());
    	highlighter.generateSnippets(shardSearchArgs->shardResults->inMemoryRecordStrings);
    }
	//
	return NULL;
}

/*
 * This call back is always called for insert and update, it will use
 * internalInsertCommand and internalUpdateCommand
 */
CommandStatus * DPInternalRequestHandler::internalInsertUpdateCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, InsertUpdateCommand * insertUpdateData){

    if(insertUpdateData == NULL){
        CommandStatus * status = NULL;
    	if(insertUpdateData->getInsertOrUpdate() == InsertUpdateCommand::DP_INSERT){ // insert case
    		status = new CommandStatus(CommandStatus::DP_INSERT);
    	}else{
    		status = new CommandStatus(CommandStatus::DP_UPDATE);
    	}
        return status;
    }

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardInsertUpdateArgs *> allShardsInsertArguments;
    pthread_t * shardInsertUpdateThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardInsertUpdateArgs * shardInsertUpdateArgs = new ShardInsertUpdateArgs(new Record(*(insertUpdateData->getRecord())),
				shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsInsertArguments.push_back(shardInsertUpdateArgs);
		if(insertUpdateData->getInsertOrUpdate() == InsertUpdateCommand::DP_INSERT){ // insert case
			if (pthread_create(&shardInsertUpdateThreads[shardIdx], NULL, insertInShardThreadWork , shardInsertUpdateArgs) != 0){
				perror("Cannot create thread for handling local message");
				return NULL;
			}
		}else{
			if (pthread_create(&shardInsertUpdateThreads[shardIdx], NULL, updateInShardThreadWork , shardInsertUpdateArgs) != 0){
				perror("Cannot create thread for handling local message");
				return NULL;
			}
		}
	}

    CommandStatus * status = NULL;
	if(insertUpdateData->getInsertOrUpdate() == InsertUpdateCommand::DP_INSERT){ // insert case
		status = new CommandStatus(CommandStatus::DP_INSERT);
	}else{
		status = new CommandStatus(CommandStatus::DP_UPDATE);
	}

	Json::Value allOutputs(Json::arrayValue);
	allOutputs.resize(shards.size());
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardInsertUpdateThreads[shardIdx], NULL);
		ShardInsertUpdateArgs * insertResults = allShardsInsertArguments.at(shardIdx);
		status->addShardResult(insertResults->shardResults);
		allOutputs[shardIdx] = insertResults->shardResults->messages;
		delete insertResults;
	}
	delete shardInsertUpdateThreads;

    Logger::info("%s", global_customized_writer.write(allOutputs).c_str());
    return status;

}


void DPInternalRequestHandler::insertInShard(const Record * record,
		Srch2Server * server, Json::Value & messages , bool & statusValue){
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
            messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_PK_Exists_Error));
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
        messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Doc_Limit_Reached_Error));
        statusValue = false;
        return;
    }
}

void * DPInternalRequestHandler::insertInShardThreadWork(void * args){
	ShardInsertUpdateArgs * shardArgs = (ShardInsertUpdateArgs *) args;
	insertInShard(shardArgs->record, shardArgs->server, shardArgs->shardResults->messages, shardArgs->shardResults->statusValue);

	//
	return NULL;
}

void DPInternalRequestHandler::updateInShard(const Record * record,
		Srch2Server * server, Json::Value & messages , bool & statusValue){
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
		        messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Existing_Record_Update_Info));
			}

			statusValue = true;
			return;
		}
		case srch2::instantsearch::OP_FAIL:
		{
	        messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Update_Failed_Error));
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
        messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Update_Failed_Error));
		statusValue = false;
	}

	/// reaching here means the insert failed, need to resume the deleted old record

	srch2::instantsearch::INDEXWRITE_RETVAL ret = server->getIndexer()->recoverRecord(primaryKeyStringValue, deletedInternalRecordId);

	switch ( ret )
	{
	case srch2::instantsearch::OP_FAIL:
	{
        messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Update_Failed_Error));
		statusValue = false;
		Logger::error("%s" , global_customized_writer.write(messages).c_str());
		return;
	}
	default: // OP_SUCCESS.
	{
        messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Update_Failed_Error));
		statusValue = false;
		return;
	}
	};

}

void * DPInternalRequestHandler::updateInShardThreadWork(void * args){
	ShardInsertUpdateArgs * updateArgs = (ShardInsertUpdateArgs  *) args;

	updateInShard(updateArgs->record, updateArgs->server, updateArgs->shardResults->messages, updateArgs->shardResults->statusValue);

	//
	return NULL;
}


/*
 * 1. Receives a delete request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this delete query
 * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::internalDeleteCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, DeleteCommand * deleteData){

    if(deleteData == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_DELETE);
        return status;
    }

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardDeleteArgs *> allShardsDeleteArguments;
    pthread_t * shardDeleteThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardDeleteArgs * shardDeleteArgs = new ShardDeleteArgs(deleteData->getPrimaryKey(),
				deleteData->getShardingKey(),
				shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsDeleteArguments.push_back(shardDeleteArgs);
		if (pthread_create(&shardDeleteThreads[shardIdx], NULL, deleteInShardThreadWork , shardDeleteArgs) != 0){
			perror("Cannot create thread for handling local message");
			return NULL;
		}
	}

    CommandStatus * status = new CommandStatus(CommandStatus::DP_DELETE);

	Json::Value allOutputs(Json::arrayValue);
	allOutputs.resize(shards.size());
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardDeleteThreads[shardIdx], NULL);
		ShardDeleteArgs * deleteResults = allShardsDeleteArguments.at(shardIdx);
		status->addShardResult(deleteResults->shardResults);
		allOutputs[shardIdx] = deleteResults->shardResults->messages;
		delete deleteResults;
	}
	delete shardDeleteThreads;

    Logger::info("%s", global_customized_writer.write(allOutputs).c_str());
    return status;

}


void DPInternalRequestHandler::deleteInShard(const string primaryKey, unsigned shardingKey,
		Srch2Server * server, Json::Value & messages, bool & statusValue){
    //delete the record from the index
    switch(server->getIndexer()->deleteRecord(primaryKey)){
    case OP_FAIL:
    {
        messages.append(JsonResponseHandler::getJsonSingleMessage(HTTP_JSON_Delete_Record_Not_Found_Error));
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

void * DPInternalRequestHandler::deleteInShardThreadWork(void * args){
	ShardDeleteArgs * deleteArgs = (ShardDeleteArgs  *) args;

	deleteInShard(deleteArgs->primaryKey,deleteArgs->shardingKey, deleteArgs->server, deleteArgs->shardResults->messages, deleteArgs->shardResults->statusValue);

	//
	return NULL;
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
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardStatusOnlyArgs *> allShardsResetLogsArguments;
    pthread_t * shardResetLogsThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardStatusOnlyArgs * shardResetLogsArgs =
				new ShardStatusOnlyArgs(shard->getSrch2Server().get(), shard->cloneShardId());
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

    // create a FILE* pointer to point to the new logger file "logger.txt"
    FILE *logFile = fopen(shardArgs->server->getCoreInfo()->getHTTPServerAccessLogFile().c_str(),"a");

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
			return internalResetLogCommand(notif->getTarget(), clusterReadview);
	}
	ASSERT(false);
	return SP(CommandStatusNotification)(); // NULL

}

//void * DPInternalRequestHandler::aclAttributeModifyShardThreadWork(void * args){
//
//}
//
//void * DPInternalRequestHandler::aclRecordModifyShardThreadWork(void * args){
//
//}

}
}
