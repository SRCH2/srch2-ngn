#include "DistributedProcessorInternal.h"


#include "util/Logger.h"
#include "util/FileOps.h"
#include "server/Srch2Server.h"

#include "serializables/SerializableInsertUpdateCommandInput.h"
#include "serializables/SerializableSerializeCommandInput.h"
#include "serializables/SerializableDeleteCommandInput.h"
#include "serializables/SerializableSearchCommandInput.h"
#include "serializables/SerializableGetInfoCommandInput.h"
#include "serializables/SerializableResetLogCommandInput.h"
#include "serializables/SerializableCommitCommandInput.h"
#include "serializables/SerializableMergeCommandInput.h"

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

	std::stringstream log_str;
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardInsertUpdateThreads[shardIdx], NULL);
		ShardInsertUpdateArgs * insertResults = allShardsInsertArguments.at(shardIdx);
		status->addShardResult(insertResults->shardResults);
		log_str << insertResults->shardResults->message ;
		delete insertResults;
	}
	delete shardInsertUpdateThreads;

    Logger::info("%s", log_str.str().c_str());
    return status;


}


void DPInternalRequestHandler::insertInShard(const Record * record,
		Srch2Server * server, string & msg, bool & statusValue){
    //add the record to the index
    std::stringstream log_str;
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
            log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"success\"}";
            Logger::info("%s", log_str.str().c_str());
            statusValue = true;
            msg = log_str.str();
            return;
        }
        case srch2::instantsearch::OP_FAIL:
        {
            log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"The record with same primary key already exists\"}";
            Logger::info("%s", log_str.str().c_str());
            statusValue = false;
            msg = log_str.str();
            return;
        }
        };
    }
    else
    {
        log_str << "{\"rid\":\"" << record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"document limit reached. Email support@srch2.com for account upgrade.\"}";
        Logger::info("%s", log_str.str().c_str());
        statusValue = false;
        msg = log_str.str();
        return;
    }
}

void * DPInternalRequestHandler::insertInShardThreadWork(void * args){
	ShardInsertUpdateArgs * shardArgs = (ShardInsertUpdateArgs *) args;
	insertInShard(shardArgs->record, shardArgs->server, shardArgs->shardResults->message, shardArgs->shardResults->statusValue);

	//
	return NULL;
}

void DPInternalRequestHandler::updateInShard(const Record * record,
		Srch2Server * server, string & msg, bool & statusValue){
    std::stringstream log_str;
	unsigned deletedInternalRecordId;
	std::string primaryKeyStringValue;
	primaryKeyStringValue = record->getPrimaryKey();
	log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"update\":\"";

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
			if (recordExisted)
				log_str << "Existing record updated successfully\"}";
			else
				log_str << "New record inserted successfully\"}";

			Logger::info("%s", log_str.str().c_str());
			statusValue = true;
			msg = log_str.str();
			return;
		}
		case srch2::instantsearch::OP_FAIL:
		{
			log_str << "failed\",\"reason\":\"insert: The record with same primary key already exists\",";
			break;
		}
		};
	}
	else
	{
		log_str << "failed\",\"reason\":\"insert: Document limit reached. Email support@srch2.com for account upgrade.\",";
	}

	/// reaching here means the insert failed, need to resume the deleted old record

	srch2::instantsearch::INDEXWRITE_RETVAL ret = server->getIndexer()->recoverRecord(primaryKeyStringValue, deletedInternalRecordId);

	switch ( ret )
	{
	case srch2::instantsearch::OP_FAIL:
	{
		log_str << "\"resume\":\"no record with given primary key\"}";
		Logger::info("%s", log_str.str().c_str());
		statusValue = false;
		msg = log_str.str();
		return;
	}
	default: // OP_SUCCESS.
	{
		log_str << "\"resume\":\"success\"}";
		Logger::info("%s", log_str.str().c_str());
		statusValue = true;
		msg = log_str.str();
		return;
	}
	};

}

void * DPInternalRequestHandler::updateInShardThreadWork(void * args){
	ShardInsertUpdateArgs * updateArgs = (ShardInsertUpdateArgs  *) args;

	updateInShard(updateArgs->record, updateArgs->server, updateArgs->shardResults->message, updateArgs->shardResults->statusValue);

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

	std::stringstream log_str;
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardDeleteThreads[shardIdx], NULL);
		ShardDeleteArgs * deleteResults = allShardsDeleteArguments.at(shardIdx);
		status->addShardResult(deleteResults->shardResults);
		log_str << deleteResults->shardResults->message;
		delete deleteResults;
	}
	delete shardDeleteThreads;

    Logger::info("%s", log_str.str().c_str());
    return status;


}


void DPInternalRequestHandler::deleteInShard(const string primaryKey, unsigned shardingKey,
		Srch2Server * server, string & msg, bool & statusValue){
    std::stringstream log_str;
    log_str << "{\"rid\":\"" << primaryKey << "\",\"delete\":\"";

    //delete the record from the index
    switch(server->getIndexer()->deleteRecord(primaryKey)){
    case OP_FAIL:
    {
        log_str << "failed\",\"reason\":\"no record with given primary key\"}";
        Logger::info("%s", log_str.str().c_str());
        msg = log_str.str();
        statusValue = false;
        return;
    }
    default: // OP_SUCCESS.
    {
        log_str << "success\"}";
        Logger::info("%s", log_str.str().c_str());
        msg = log_str.str();
        statusValue = true;
        return;
    }
    };
}

void * DPInternalRequestHandler::deleteInShardThreadWork(void * args){
	ShardDeleteArgs * deleteArgs = (ShardDeleteArgs  *) args;

	deleteInShard(deleteArgs->primaryKey,deleteArgs->shardingKey, deleteArgs->server, deleteArgs->shardResults->message, deleteArgs->shardResults->statusValue);

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
		ShardGetInfoArgs * shardGetInfoArgs = new ShardGetInfoArgs(shard->getSrch2Server().get(), shard->getShardIdentifier());
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


void DPInternalRequestHandler::getInfoInShard(Srch2Server * server,unsigned & readCount,
		unsigned & writeCount, unsigned & numberOfIndexedDocuments,
		std::string & lastMergeTimeString, unsigned & docCount){
    server->getIndexer()->getIndexHealthThoughArguments(readCount, writeCount,
    		numberOfIndexedDocuments, lastMergeTimeString ,docCount);
}
void * DPInternalRequestHandler::getInfoInShardThreadWork(void * args){
	ShardGetInfoArgs * infoArgs = (ShardGetInfoArgs *)args;
	getInfoInShard(infoArgs->server, infoArgs->shardResult->readCount, infoArgs->shardResult->writeCount,
			infoArgs->shardResult->numberOfDocumentsInIndex,
			infoArgs->shardResult->lastMergeTimeString, infoArgs->shardResult->docCount);

	//
	return NULL;
}

/*
 * This call back function is called for serialization. It uses internalSerializeIndexCommand
 * and internalSerializeRecordsCommand for our two types of serialization.
 */
CommandStatus * DPInternalRequestHandler::internalSerializeCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, SerializeCommand * serailizeData){
	if(serailizeData == NULL){
        CommandStatus * status = NULL;
        if(serailizeData->getIndexOrRecord() == SerializeCommand::SERIALIZE_INDEX){ // serialize index
        	status = new CommandStatus(CommandStatus::DP_SERIALIZE_INDEX);
        }else{ // serialize records
        	status = new CommandStatus(CommandStatus::DP_SERIALIZE_RECORDS);
        }
        return status;
    }

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardSerializeArgs *> allShardsSerializeArguments;
    pthread_t * shardSerializeThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardSerializeArgs * shardSerializeArgs =
				new ShardSerializeArgs(serailizeData->getDataFileName(),
						shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsSerializeArguments.push_back(shardSerializeArgs);

		if(serailizeData->getIndexOrRecord() == SerializeCommand::SERIALIZE_INDEX){ // insert case
			if (pthread_create(&shardSerializeThreads[shardIdx], NULL, serializeIndexInShardThreadWork , shardSerializeArgs) != 0){
				perror("Cannot create thread for handling local message");
				return NULL;
			}
		}else{
			if (pthread_create(&shardSerializeThreads[shardIdx], NULL, serializeRecordsInShardThreadWork , shardSerializeArgs) != 0){
				perror("Cannot create thread for handling local message");
				return NULL;
			}
		}
	}
    CommandStatus * status = NULL;
	if(serailizeData->getIndexOrRecord() == SerializeCommand::SERIALIZE_INDEX){ // insert case
		status = new CommandStatus(CommandStatus::DP_SERIALIZE_INDEX);
		Logger::info("%s", "{\"save\":\"success\"}");
	}else{
		status = new CommandStatus(CommandStatus::DP_SERIALIZE_RECORDS);
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
	ShardSerializeArgs * serializeRecordsArgs = (ShardSerializeArgs *) args;
	serializeRecordsArgs->server->getIndexer()->save();
    //
    return NULL;
}

void * DPInternalRequestHandler::serializeRecordsInShardThreadWork(void * args){
	ShardSerializeArgs * serializeRecordsArgs = (ShardSerializeArgs *) args;
    string exportedDataFileName = serializeRecordsArgs->dataFileName;
    if(srch2::util::checkDirExistence(exportedDataFileName.c_str())){
        exportedDataFileName = "export_data.json";
    }
    serializeRecordsArgs->server->getIndexer()->exportData(exportedDataFileName);
    //
    return NULL;
}

/*
 * 1. Receives a ResetLog request from a shard
 * 2. Uses core to reset log
 * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::internalResetLogCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, ResetLogCommand * resetData){

    if(resetData == NULL ){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_RESET_LOG);
        return status;
    }

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardStatusOnlyArgs *> allShardsResetLogsArguments;
    pthread_t * shardResetLogsThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardStatusOnlyArgs * shardResetLogsArgs =
				new ShardStatusOnlyArgs(shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsResetLogsArguments.push_back(shardResetLogsArgs);

		if (pthread_create(&shardResetLogsThreads[shardIdx], NULL, resetLogInShardThreadWork, shardResetLogsArgs) != 0){
			perror("Cannot create thread for handling local message");
			return NULL;
		}
	}

    CommandStatus * status = new CommandStatus(CommandStatus::DP_RESET_LOG);

    for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardResetLogsThreads[shardIdx], NULL);
		ShardStatusOnlyArgs * resetResults = allShardsResetLogsArguments.at(shardIdx);
		status->addShardResult(resetResults->shardResults);
		delete resetResults;
	}
	delete shardResetLogsThreads;

    return status;


}


CommandStatus * DPInternalRequestHandler::internalCommitCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, CommitCommand * commitData){

	if(commitData == NULL ){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_COMMIT);
        return status;
    }

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardStatusOnlyArgs *> allShardsCommitArguments;
    pthread_t * shardCommitThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardStatusOnlyArgs * shardCommitArgs =
				new ShardStatusOnlyArgs(shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsCommitArguments.push_back(shardCommitArgs);

		if (pthread_create(&shardCommitThreads[shardIdx], NULL, commitInShardThreadWork, shardCommitArgs) != 0){
			perror("Cannot create thread for handling local message");
			return NULL;
		}
	}

    CommandStatus * status = new CommandStatus(CommandStatus::DP_COMMIT);

    for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		pthread_join(shardCommitThreads[shardIdx], NULL);
		ShardStatusOnlyArgs * commitResults = allShardsCommitArguments.at(shardIdx);
		status->addShardResult(commitResults->shardResults);
		delete commitResults;
	}
	delete shardCommitThreads;

    return status;


}


CommandStatus * DPInternalRequestHandler::internalMergeCommand(const NodeTargetShardInfo & target,
		boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview, MergeCommand * mergeData){

	if(mergeData == NULL ){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_MERGE);
        return status;
    }

	// 0. use target and readview to get Srch2Server pointers
	vector<const Shard *> shards;
	clusterReadview->getLocalShardContainer(target.getCoreId())->getShards(target,shards);

	vector<ShardStatusOnlyArgs *> allShardsMergeArguments;
    pthread_t * shardMergeThreads = new pthread_t[shards.size()];
	for(unsigned shardIdx = 0; shardIdx < shards.size(); ++shardIdx){
		const Shard * shard = shards.at(shardIdx);
		ShardStatusOnlyArgs * shardMergeArgs =
				new ShardStatusOnlyArgs(shard->getSrch2Server().get(), shard->getShardIdentifier());
		allShardsMergeArguments.push_back(shardMergeArgs);

		if (pthread_create(&shardMergeThreads[shardIdx], NULL, mergeInShardThreadWork, shardMergeArgs) != 0){
			perror("Cannot create thread for handling local message");
			return NULL;
		}
	}

    CommandStatus * status = new CommandStatus(CommandStatus::DP_MERGE);

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
        Logger::info("%s", string("{\"message\":\"The logger file repointing failed. Could not create new logger file\", \"log\":\""
                + shardArgs->server->getCoreInfo()->getHTTPServerAccessLogFile() + "\"}").c_str());
        shardArgs->shardResults->message = "{\"message\":\"The logger file repointing failed. Could not create new logger file\", \"log\":\""
                + shardArgs->server->getCoreInfo()->getHTTPServerAccessLogFile() + "\"}";
        shardArgs->shardResults->statusValue = false;
    } else {
        FILE * oldLogger = srch2::util::Logger::swapLoggerFile(logFile);
        fclose(oldLogger);
        Logger::info("%s", string("{\"message\":\"The logger file repointing succeeded\", \"log\":\""
                + shardArgs->server->getCoreInfo()->getHTTPServerAccessLogFile() + "\"}").c_str());
        shardArgs->shardResults->message = "{\"message\":\"The logger file repointing succeeded\", \"log\":\""
                + shardArgs->server->getCoreInfo()->getHTTPServerAccessLogFile() + "\"}";
        shardArgs->shardResults->statusValue = true;
    }

	//
	return NULL;

}
void * DPInternalRequestHandler::commitInShardThreadWork(void * args){
	ShardStatusOnlyArgs * shardArgs = (ShardStatusOnlyArgs * )args;

    //commit the index.
    if ( shardArgs->server->getIndexer()->commit() == srch2::instantsearch::OP_SUCCESS)
    {
        Logger::info("%s", "{\"commit\":\"success\"}");
        shardArgs->shardResults->message = "{\"commit\":\"success\"}";
        shardArgs->shardResults->statusValue = true;
    }
    else
    {
        Logger::info("%s", "{\"commit\":\"failed\"}");
        shardArgs->shardResults->message = "{\"commit\":\"failed\"}";
         shardArgs->shardResults->statusValue = false;
    }

	//
	return NULL;
}


void * DPInternalRequestHandler::mergeInShardThreadWork(void * args){
	ShardStatusOnlyArgs * shardArgs = (ShardStatusOnlyArgs * )args;

    //merge the index.
    if ( shardArgs->server->getIndexer()->merge() == srch2::instantsearch::OP_SUCCESS)
    {
        Logger::info("%s", "{\"merge\":\"success\"}");
        shardArgs->shardResults->message = "{\"merge\":\"success\"}";
        shardArgs->shardResults->statusValue = true;
    }
    else
    {
        Logger::info("%s", "{\"merge\":\"failed\"}");
        shardArgs->shardResults->message = "{\"merge\":\"failed\"}";
         shardArgs->shardResults->statusValue = false;
    }

	//
	return NULL;
}

}
}
