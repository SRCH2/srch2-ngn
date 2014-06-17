#include "DistributedProcessorInternal.h"


#include "util/Logger.h"
#include "util/FileOps.h"
#include "server/Srch2Server.h"

#include "serializables/SerializableInsertUpdateCommandInput.h"
#include "serializables/SerializableSerializeCommandInput.h"
#include "serializables/SerializableDeleteCommandInput.h"
#include "serializables/SerializableSearchResults.h"
#include "serializables/SerializableSearchCommandInput.h"
#include "serializables/SerializableCommandStatus.h"
#include "serializables/SerializableGetInfoCommandInput.h"
#include "serializables/SerializableGetInfoResults.h"
#include "serializables/SerializableResetLogCommandInput.h"
#include "serializables/SerializableCommitCommandInput.h"

#include "QueryExecutor.h"
#include "instantsearch/LogicalPlan.h"
#include <instantsearch/QueryResults.h>
#include <core/query/QueryResultsInternal.h>
#include <core/util/Version.h>
#include "ServerHighLighter.h"
#include "wrapper/ParsedParameterContainer.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {


boost::shared_ptr<Srch2Server> Srch2ServerAccess::getSrch2Server(Srch2ServerAccessAvailabilty requestedAvailability, bool & available){
    boost::shared_lock< boost::shared_mutex > lock(availabilityLock);
    // if the requested access level is less than availale level, we can return the pointer
    // this mechanism helps us stop some requests when needed.
    if((unsigned) requestedAvailability <= (unsigned) this->availability){
    	available = true;
		return srch2Server;
    }else{
    	available = false;
    	return boost::shared_ptr<Srch2Server>();
    }
}

Srch2ServerAccess::Srch2ServerAccess(const ShardId correspondingShardId, const string & coreName ):correspondingShardId(correspondingShardId){
	srch2Server.reset(new Srch2Server());
	// set the corename in srch2Server so that it can access correct information in config manager
	srch2Server->setCoreName(coreName);
	setAvailability(DPInternal_NonAvailable);
}
void Srch2ServerAccess::setAvailability(Srch2ServerAccessAvailabilty availability){
    boost::unique_lock< boost::shared_mutex > lock(availabilityLock);
    this->availability = availability;
}


// DPInternalRequestHandler

DPInternalRequestHandler::DPInternalRequestHandler(ConfigManager * configurationManager){
    this->configurationManager = configurationManager;
}
/*
 * 1. Receives a search request from a shard
 * 2. Uses core to evaluate this search query
 * 3. Sends the results to the shard which initiated this search query
 */
SearchCommandResults * DPInternalRequestHandler::internalSearchCommand(Srch2Server * server, SearchCommand * searchData){

    if(searchData == NULL || server == NULL){
        SearchCommandResults * searchResults = new SearchCommandResults();
        return searchResults;
    }
    // first find the search URL
    LogicalPlan & logicalPlan = *(searchData->getLogicalPlan());

    // search results to be serialized and sent over the network
    SearchCommandResults * searchResults = new SearchCommandResults();

    struct timespec tstart;
    //    struct timespec tstart2;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);

    // search in core
    // TODO : is it possible to make executor and planGen singleton ?
    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;
    QueryExecutor qe(logicalPlan, searchResults->getQueryResultsFactory(), server , indexDataContainerConf);
    // in here just allocate an empty QueryResults object, it will be initialized in execute.
    qe.executeForDPInternal(searchResults->getQueryResults(), searchResults->getInMemoryRecordStringsWrite());
    // compute elapsed time in ms , end the timer
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

    searchResults->setSearcherTime(ts1);

    if (server->indexDataConfig->getHighlightAttributeIdsVector().size() > 0 ) {
    	    //TODO: V1 we need these two parameters in DP internal
    		//!paramContainer.onlyFacets &&
    		//paramContainer.isHighlightOn) {
    	ParsedParameterContainer paramContainer; // temp for V0

    	QueryResults *finalResults = searchResults->getQueryResults();
    	ServerHighLighter highlighter =  ServerHighLighter(finalResults, server, paramContainer,
    			logicalPlan.getOffset(), logicalPlan.getNumberOfResultsToRetrieve());
    	highlighter.generateSnippets(searchResults->getInMemoryRecordStringsWrite());
    }

    return searchResults;

}

/*
 * This call back is always called for insert and update, it will use
 * internalInsertCommand and internalUpdateCommand
 */
CommandStatus * DPInternalRequestHandler::internalInsertUpdateCommand(Srch2Server * server,
        InsertUpdateCommand * insertUpdateData){
    if(insertUpdateData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_INSERT_UPDATE, false, "");
        return status;
    }
    if(insertUpdateData->getInsertOrUpdate() == InsertUpdateCommand::DP_INSERT){ // insert case
        return internalInsertCommand(server, insertUpdateData);
    }else{ // update case
        return internalUpdateCommand(server, insertUpdateData);
    }
}

/*
 * 1. Receives an insert request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this insert query
 * 3. Sends the results to the shard which initiated this insert query (Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::internalInsertCommand(Srch2Server * server,
        InsertUpdateCommand * insertUpdateData){

    if(insertUpdateData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_INSERT, false, "");
        return status;
    }
    //add the record to the index
    std::stringstream log_str;
    if ( server->indexer->getNumberOfDocumentsInIndex() < server->indexDataConfig->getDocumentLimit() )
    {
        // Do NOT delete analyzer because it is thread specific. It will be reused for
        // search/update/delete operations.
        srch2::instantsearch::Analyzer * analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->indexDataConfig);
        srch2::instantsearch::INDEXWRITE_RETVAL ret = server->indexer->addRecord(insertUpdateData->getRecord(), analyzer);

        switch( ret )
        {
        case srch2::instantsearch::OP_SUCCESS:
        {
            log_str << "{\"rid\":\"" << insertUpdateData->getRecord()->getPrimaryKey() << "\",\"insert\":\"success\"}";
            Logger::info("%s", log_str.str().c_str());
            CommandStatus * status=
                    new CommandStatus(CommandStatus::DP_INSERT, true, log_str.str());
            return status;
        }
        case srch2::instantsearch::OP_FAIL:
        {
            log_str << "{\"rid\":\"" << insertUpdateData->getRecord()->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"The record with same primary key already exists\"}";
            Logger::info("%s", log_str.str().c_str());
            CommandStatus * status =
                    new CommandStatus(CommandStatus::DP_INSERT, false, log_str.str());
            return status;
        }
        };
    }
    else
    {
        log_str << "{\"rid\":\"" << insertUpdateData->getRecord()->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"document limit reached. Email support@srch2.com for account upgrade.\"}";
        Logger::info("%s", log_str.str().c_str());
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_INSERT, false, log_str.str());
        return status;
    }

    ASSERT(false);
    CommandStatus * status =
            new CommandStatus(CommandStatus::DP_INSERT, false, log_str.str());
    Logger::info("%s", log_str.str().c_str());
    return status;


}



/*
 * 1. Receives a update request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this update query
 * 3. Sends the results to the shard which initiated this update request (Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::internalUpdateCommand(Srch2Server * server,
        InsertUpdateCommand * insertUpdateData){

    if(insertUpdateData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_UPDATE, false, "");
        return status;
    }
    std::stringstream log_str;

    unsigned deletedInternalRecordId;
    std::string primaryKeyStringValue;
    primaryKeyStringValue = insertUpdateData->getRecord()->getPrimaryKey();
    log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"update\":\"";

    //delete the record from the index
    bool recordExisted = false;
    switch(server->indexer->deleteRecordGetInternalId(primaryKeyStringValue, deletedInternalRecordId))
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

    if ( server->indexer->getNumberOfDocumentsInIndex() < server->indexDataConfig->getDocumentLimit() )
    {
        // Do NOT delete analyzer because it is thread specific. It will be reused for
        // search/update/delete operations.
        Analyzer* analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->indexDataConfig);
        srch2::instantsearch::INDEXWRITE_RETVAL ret = server->indexer->addRecord(insertUpdateData->getRecord(), analyzer);
        switch( ret )
        {
        case srch2::instantsearch::OP_SUCCESS:
        {
            if (recordExisted)
                log_str << "Existing record updated successfully\"}";
            else
                log_str << "New record inserted successfully\"}";

            Logger::info("%s", log_str.str().c_str());
            CommandStatus * status =
                    new CommandStatus(CommandStatus::DP_UPDATE, true, log_str.str());
            return status;
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

    srch2::instantsearch::INDEXWRITE_RETVAL ret = server->indexer->recoverRecord(primaryKeyStringValue, deletedInternalRecordId);

    switch ( ret )
    {
    case srch2::instantsearch::OP_FAIL:
    {
        log_str << "\"resume\":\"no record with given primary key\"}";
        Logger::info("%s", log_str.str().c_str());
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_UPDATE, false, log_str.str());
        return status;
    }
    default: // OP_SUCCESS.
    {
        log_str << "\"resume\":\"success\"}";
        Logger::info("%s", log_str.str().c_str());
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_UPDATE, true, log_str.str());
        return status;
    }
    };

    // we should not reach here
    ASSERT(false);
    Logger::info("%s", log_str.str().c_str());
    CommandStatus * status =
            new CommandStatus(CommandStatus::DP_UPDATE, false, log_str.str());
    return status;

}

/*
 * 1. Receives a delete request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this delete query
 * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::    internalDeleteCommand(Srch2Server * server, DeleteCommand * deleteData){

    if(deleteData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_DELETE, false, "");
        return status;
    }
    std::stringstream log_str;
    log_str << "{\"rid\":\"" << deleteData->getPrimaryKey() << "\",\"delete\":\"";

    //delete the record from the index
    switch(server->indexer->deleteRecord(deleteData->getPrimaryKey())){
    case OP_FAIL:
    {
        log_str << "failed\",\"reason\":\"no record with given primary key\"}";
        Logger::info("%s", log_str.str().c_str());
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_DELETE, false, log_str.str());
        return status;
    }
    default: // OP_SUCCESS.
    {
        log_str << "success\"}";
        Logger::info("%s", log_str.str().c_str());
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_DELETE, true, log_str.str());
        return status;
    }
    };
}


/*
 * 1. Receives a GetInfo request from a shard
 * 2. Uses core to get info
 * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
 */
GetInfoCommandResults * DPInternalRequestHandler::internalGetInfoCommand(Srch2Server * server, GetInfoCommand * getInfoData){
    if(getInfoData == NULL || server == NULL){
        GetInfoCommandResults * getInfoResult =
                new GetInfoCommandResults(0,0,0,"",0,"");
        return getInfoResult;
    }
    unsigned readCount;
    unsigned writeCount;
    unsigned numberOfDocumentsInIndex;
    string lastMergeTimeString;
    unsigned docCount;
    server->indexer->getIndexHealthThoughArguments(readCount, writeCount, numberOfDocumentsInIndex, lastMergeTimeString ,docCount);

    GetInfoCommandResults * getInfoResult =
            new GetInfoCommandResults(readCount, writeCount, numberOfDocumentsInIndex, lastMergeTimeString, docCount, Version::getCurrentVersion());
    return getInfoResult;
}


/*
 * This call back function is called for serialization. It uses internalSerializeIndexCommand
 * and internalSerializeRecordsCommand for our two types of serialization.
 */
CommandStatus * DPInternalRequestHandler::internalSerializeCommand(Srch2Server * server, SerializeCommand * serailizeData){
    if(serailizeData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_SERIALIZE, false, "");
        return status;
    }
    if(serailizeData->getIndexOrRecord() == SerializeCommand::SERIALIZE_INDEX){ // serialize index
        return this->internalSerializeIndexCommand(server, serailizeData);
    }else{ // serialize records
        return this->internalSerializeRecordsCommand(server, serailizeData);
    }
}


/*
 * 1. Receives a SerializeIndex request from a shard
 * 2. Uses core to do the serialization
 * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::internalSerializeIndexCommand(Srch2Server * server, SerializeCommand * serailizeData){
    if(serailizeData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_SERIALIZE_INDEX, false, "");
        return status;
    }
    server->indexer->save();
    Logger::info("%s", "{\"save\":\"success\"}");
    CommandStatus * status =
            new CommandStatus(CommandStatus::DP_SERIALIZE_INDEX, true, "{\"save\":\"success\"}");
    return status;
}

/*
 * 1. Receives a SerializeRecords request from a shard
 * 2. Uses core to do the serialization
 * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::internalSerializeRecordsCommand(Srch2Server * server, SerializeCommand * serailizeData){

    if(serailizeData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_SERIALIZE_RECORDS, false, "");
        return status;
    }
    string exportedDataFileName = serailizeData->getDataFileName();
    if(srch2::util::checkDirExistence(exportedDataFileName.c_str())){
        exportedDataFileName = "export_data.json";
    }
    server->indexer->exportData(exportedDataFileName);
    Logger::info("%s", "{\"export\":\"success\"}");
    CommandStatus * status =
            new CommandStatus(CommandStatus::DP_SERIALIZE_RECORDS, true, "{\"export\":\"success\"}");
    return status;

}

/*
 * 1. Receives a ResetLog request from a shard
 * 2. Uses core to reset log
 * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
 */
CommandStatus * DPInternalRequestHandler::internalResetLogCommand(Srch2Server * server, ResetLogCommand * resetData){

    if(resetData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_RESET_LOG, false, "");
        return status;
    }

    // create a FILE* pointer to point to the new logger file "logger.txt"
    FILE *logFile = fopen(server->indexDataConfig->getHTTPServerAccessLogFile().c_str(),
            "a");

    if (logFile == NULL) {
        srch2::util::Logger::error("Reopen Log file %s failed.",
                server->indexDataConfig->getHTTPServerAccessLogFile().c_str());
        Logger::info("%s", string("{\"message\":\"The logger file repointing failed. Could not create new logger file\", \"log\":\""
                + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}").c_str());
        CommandStatus * status=
                new CommandStatus(CommandStatus::DP_RESET_LOG, false,
                        "{\"message\":\"The logger file repointing failed. Could not create new logger file\", \"log\":\""
                        + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}");
        return status;
    } else {
        FILE * oldLogger = srch2::util::Logger::swapLoggerFile(logFile);
        fclose(oldLogger);
        Logger::info("%s", string("{\"message\":\"The logger file repointing succeeded\", \"log\":\""
                + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}").c_str());
        CommandStatus * status=
                new CommandStatus(CommandStatus::DP_RESET_LOG, true,
                        "{\"message\":\"The logger file repointing succeeded\", \"log\":\""
                        + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}");
        return status;
    }
}


CommandStatus * DPInternalRequestHandler::internalCommitCommand(Srch2Server * server, CommitCommand * resetData){
    if(resetData == NULL || server == NULL){
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_COMMIT, false, "");
        return status;
    }

    //commit the index.
    if ( server->indexer->commit() == srch2::instantsearch::OP_SUCCESS)
    {
        Logger::info("%s", "{\"commit\":\"success\"}");
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_COMMIT, true, "{\"commit\":\"success\"}");
        return status;
    }
    else
    {
        Logger::info("%s", "{\"commit\":\"failed\"}");
        CommandStatus * status =
                new CommandStatus(CommandStatus::DP_COMMIT, false, "{\"commit\":\"failed\"}");
        return status;
    }
}


/*
 * The following methods provide an API to register/allocate/delete/load/create and other operations on
 * indices.
 * NOTE: As of June 16th, since our core codebase is wrapped and accessed from sharding layers through Srch2Server
 * objects, indices and processing are combined in the Srch2Server objects, so DP Internal shouldn't be viewed as
 * an Index Manager. Index Managers tend to be a container for indices while this module is more of a wrapper on the API
 * provided by the core codebase.
 */
Srch2ServerHandle DPInternalRequestHandler::registerSrch2Server(const ShardId correspondingShardId, const string & coreName){
	// add a Srch2ServerAccess to the map and return the handle.
    boost::unique_lock< boost::shared_mutex > lock(globalIndexLock);
    // number of already registered srch2Server access objects
    unsigned numberOfCurrentSrch2Servers = srch2Servers.size();
    srch2Servers.insert(std::make_pair(numberOfCurrentSrch2Servers + 1 , new Srch2ServerAccess(correspondingShardId, coreName)));
    return numberOfCurrentSrch2Servers + 1;
}

DPInternalAPIStatus DPInternalRequestHandler::bootstrapSrch2Server(Srch2ServerHandle handle){
	ASSERT(handle > 0);
	// first find the handle in the map and get the Srch2ServerAccess object
	Srch2ServerAccess * srch2ServerAccess;
	{
        boost::shared_lock< boost::shared_mutex > lock(globalIndexLock);
        map< Srch2ServerHandle , Srch2ServerAccess * >::iterator srch2ServerItr =
        		srch2Servers.find(handle);
        if(srch2ServerItr == srch2Servers.end()){
        	return DPInternal_Srch2ServerNotFound;
        }
        srch2ServerAccess = srch2ServerItr->second;
	}
	// When bootstrap is called, srch2Server must be non-available
	ASSERT(srch2ServerAccess->availability == DPInternal_NonAvailable);

	// bootstrap the server
	bool availabe;
	boost::shared_ptr<Srch2Server> srch2Server = srch2ServerAccess->getSrch2Server(DPInternal_NonAvailable, availabe);
	ASSERT(availabe == true);
	srch2Server->init(configurationManager);

	// set the availability of the access object to available for read and write
	// first look in the map again to find the Srch2ServerAccess
	// if it's gone in the time of load/create, ignore
	// otherwise, green the flag
	{
        boost::shared_lock< boost::shared_mutex > lock(globalIndexLock);
        map< Srch2ServerHandle , Srch2ServerAccess * >::iterator srch2ServerItr =
        		srch2Servers.find(handle);
        if(srch2ServerItr == srch2Servers.end()){
        	return DPInternal_Srch2ServerNotFound;
        }
        srch2ServerItr->second->setAvailability(DPInternal_ReadWriteAvailable);
	}

	return DPInternal_Success;
}

DPInternalAPIStatus DPInternalRequestHandler::deleteSrch2Server(Srch2ServerHandle handle){
	ASSERT(handle > 0);
	// first find the handle in the map and get the Srch2ServerAccess object
	boost::shared_lock< boost::shared_mutex > lock(globalIndexLock);
	map< Srch2ServerHandle , Srch2ServerAccess * >::iterator srch2ServerItr =
			srch2Servers.find(handle);
	if(srch2ServerItr == srch2Servers.end()){
		return DPInternal_Srch2ServerNotFound;
	}
	// erase it from map, Srch2ServerAccess will be destroyed and
	// Srch2Server shared pointer inside that will be deleted when all
	// dp internal readers are gone.
	delete srch2ServerItr->second;
	srch2Servers.erase(srch2ServerItr);
	return DPInternal_Success;
}

}
}
