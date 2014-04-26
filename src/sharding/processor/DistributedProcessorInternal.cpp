#include "DistributedProcessorInternal.h"


#include "util/Logger.h"



namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

/*
 * 1. Receives a search request from a shard
 * 2. Uses core to evaluate this search query
 * 3. Sends the results to the shard which initiated this search query
 */
SerializableSearchResults DPInternalRequestHandler::internalSearchCommand(Srch2Server * server, SerializableSearchCommandInput * searchData){

	if(searchData == NULL || server == NULL){
		SerializableSearchResults searchResults;
		return searchResults;
	}
	// first find the search URL
	LogicalPlan & logicalPlan = *(searchData->logicalPlan);

    struct timespec tstart;
//    struct timespec tstart2;
    struct timespec tend;
    clock_gettime(CLOCK_REALTIME, &tstart);

	// search in core
    srch2is::QueryResultFactory * resultsFactory =
            new srch2is::QueryResultFactory();
    // TODO : is it possible to make executor and planGen singleton ?
    const CoreInfo_t *indexDataContainerConf = server->indexDataConfig;
    QueryExecutor qe(logicalPlan, resultsFactory, server , indexDataContainerConf);
    // in here just allocate an empty QueryResults object, it will be initialized in execute.
    QueryResults * finalResults = new QueryResults();
    qe.execute(finalResults);
    // compute elapsed time in ms , end the timer
    clock_gettime(CLOCK_REALTIME, &tend);
    unsigned ts1 = (tend.tv_sec - tstart.tv_sec) * 1000
            + (tend.tv_nsec - tstart.tv_nsec) / 1000000;
    SerializableSearchResults searchResults;
    searchResults.queryResults = finalResults;
    searchResults.searcherTime = ts1;

    return searchResults;

}

/*
 * This call back is always called for insert and update, it will use
 * internalInsertCommand and internalUpdateCommand
 */
SerializableCommandStatus DPInternalRequestHandler::internalInsertUpdateCommand(Srch2Server * server,
		SerializableInsertUpdateCommandInput * insertUpdateData){
	if(insertUpdateData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 0; // update
		status.message = "";
		status.status = false;
		return status;
	}
	if(insertUpdateData == true){ // insert case
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
SerializableCommandStatus DPInternalRequestHandler::internalInsertCommand(Srch2Server * server,
		SerializableInsertUpdateCommandInput * insertUpdateData){

	if(insertUpdateData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 0; // insert
		status.message = "";
		status.status = false;
		return status;
	}
	//add the record to the index
	std::stringstream log_str;
	if ( server->indexer->getNumberOfDocumentsInIndex() < server->indexDataConfig->getDocumentLimit() )
	{
		// Do NOT delete analyzer because it is thread specific. It will be reused for
		// search/update/delete operations.
        srch2::instantsearch::Analyzer * analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->indexDataConfig);
		srch2::instantsearch::INDEXWRITE_RETVAL ret = server->indexer->addRecord(insertUpdateData->record, analyzer);

		switch( ret )
		{
			case srch2::instantsearch::OP_SUCCESS:
			{
				log_str << "{\"rid\":\"" << insertUpdateData->record->getPrimaryKey() << "\",\"insert\":\"success\"}";
				SerializableCommandStatus status;
				status.commandNumber = 0;//insert
				status.message = log_str.str();
				status.status = true;
				return status;
			}
			case srch2::instantsearch::OP_FAIL:
			{
				log_str << "{\"rid\":\"" << insertUpdateData->record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"The record with same primary key already exists\"}";
				SerializableCommandStatus status;
				status.commandNumber = 0;//insert
				status.message = log_str.str();
				status.status = false;
				return status;
			}
		};
	}
	else
	{
		log_str << "{\"rid\":\"" << insertUpdateData->record->getPrimaryKey() << "\",\"insert\":\"failed\",\"reason\":\"document limit reached. Email support@srch2.com for account upgrade.\"}";
		SerializableCommandStatus status;
		status.commandNumber = 0;//insert
		status.message = log_str.str();
		status.status = false;
		return status;
	}

	ASSERT(false);
	SerializableCommandStatus status;
	status.commandNumber = 0;//insert
	status.message = log_str.str();
	status.status = false;
	return status;


}



/*
 * 1. Receives a update request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this update query
 * 3. Sends the results to the shard which initiated this update request (Failure or Success)
 */
SerializableCommandStatus DPInternalRequestHandler::internalUpdateCommand(Srch2Server * server,
		SerializableInsertUpdateCommandInput * insertUpdateData){

	if(insertUpdateData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 1; // update
		status.message = "";
		status.status = false;
		return status;
	}
	std::stringstream log_str;

    unsigned deletedInternalRecordId;
    std::string primaryKeyStringValue;
	primaryKeyStringValue = insertUpdateData->record->getPrimaryKey();
	log_str << "{\"rid\":\"" << primaryKeyStringValue << "\",\"update\":\"";

	//delete the record from the index
	bool recordExisted = false;
	switch(indexer->deleteRecordGetInternalId(primaryKeyStringValue, deletedInternalRecordId))
	{
		case srch2::instantsearch::OP_FAIL:
		{
			// record to update doesn't exit, will insert it
			break;
		}
		default: // OP_SUCCESS.
		{
		    recordExisted = true;
		}
	};


    /// step 2, insert new record

	//add the record to the index

	if ( indexer->getNumberOfDocumentsInIndex() < server->indexDataConfig->getDocumentLimit() )
	{
		// Do NOT delete analyzer because it is thread specific. It will be reused for
		// search/update/delete operations.
        Analyzer* analyzer = AnalyzerFactory::getCurrentThreadAnalyzer(server->indexDataConfig);
		srch2::instantsearch::INDEXWRITE_RETVAL ret = server->indexer->addRecord(insertUpdateData->record, analyzer);
		switch( ret )
		{
			case srch2::instantsearch::OP_SUCCESS:
			{
				if (recordExisted)
				  log_str << "Existing record updated successfully\"}";
				else
				  log_str << "New record inserted successfully\"}";

				SerializableCommandStatus status;
				status.commandNumber = 1; // update
				status.message = log_str.str();
				status.status = true;
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

    srch2::instantsearch::INDEXWRITE_RETVAL ret = indexer->recoverRecord(primaryKeyStringValue, deletedInternalRecordId);

    switch ( ret )
    {
        case srch2::instantsearch::OP_FAIL:
        {
            log_str << "\"resume\":\"no record with given primary key\"}";
			SerializableCommandStatus status;
			status.commandNumber = 1; // update
			status.message = log_str.str();
			status.status = false;
			return status;
        }
        default: // OP_SUCCESS.
        {
            log_str << "\"resume\":\"success\"}";
			SerializableCommandStatus status;
			status.commandNumber = 1; // update
			status.message = log_str.str();
			status.status = true;
			return status;
        }
    };

    // we should not reach here
    ASSERT(false);
	SerializableCommandStatus status;
	status.commandNumber = 1; // update
	status.message = log_str.str();
	status.status = false;
	return status;

}

/*
 * 1. Receives a delete request from a shard and makes sure this
 *    shard is the correct reponsible of this record using Partitioner
 * 2. Uses core execute this delete query
 * 3. Sends the results to the shard which initiated this delete request (Failure or Success)
 */
SerializableCommandStatus DPInternalRequestHandler::	internalDeleteCommand(Srch2Server * server, SerializableDeleteCommandInput * deleteData){

	if(deleteData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 2; // delete
		status.message = "";
		status.status = false;
		return status;
	}
	std::stringstream log_str;
	log_str << "{\"rid\":\"" << deleteData->primaryKey << "\",\"delete\":\"";

	SerializableCommandStatus status;
	status.commandNumber = 2; // update
	//delete the record from the index
	switch(server->indexer->deleteRecord(deleteData->primaryKey)){
		case OP_FAIL:
		{
			log_str << "failed\",\"reason\":\"no record with given primary key\"}";
			status.status = false;
			break;
		}
		default: // OP_SUCCESS.
		{
			log_str << "success\"}";
			status.status = true;
			break;
		}
	};

	status.message = log_str.str();
	return status;
}


/*
 * 1. Receives a GetInfo request from a shard
 * 2. Uses core to get info
 * 3. Sends the results to the shard which initiated this getInfo request (Failure or Success)
 */
SerializableGetInfoResults DPInternalRequestHandler::internalGetInfoCommand(Srch2Server * server, string versionInfo, SerializableGetInfoCommandInput * getInfoData){
	if(getInfoData == NULL || server == NULL){
		SerializableGetInfoResults getInfoResult;
		return getInfoResult;
	}
	SerializableGetInfoResults getInfoResult;
	server->indexer->getIndexHealth(getInfoResult.readCount, getInfoResult.writeCount, getInfoResult.numberOfDocumentsInIndex ,
			getInfoResult.lastMergeTimeString , getInfoResult.docCount);
	getInfoResult.versionInfo = versionInfo;
	return getInfoResult;
}


/*
 * This call back function is called for serialization. It uses internalSerializeIndexCommand
 * and internalSerializeRecordsCommand for our two types of serialization.
 */
SerializableCommandStatus DPInternalRequestHandler::	internalSerializeCommand(Srch2Server * server, SerializableSerializeCommandInput * serailizeData){
	if(serailizeData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 4; // seralize
		status.message = "";
		status.status = false;
		return status;
	}
	if(serailizeData->indexOrRecord){ // serialize index
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
SerializableCommandStatus DPInternalRequestHandler::internalSerializeIndexCommand(Srch2Server * server, SerializableSerializeCommandInput * serailizeData){
	if(serailizeData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 4; // seralize index
		status.message = "";
		status.status = false;
		return status;
	}
	server->indexer->save();
	SerializableCommandStatus status;
	status.commandNumber = 4; // seralize index
	status.message = "{\"save\":\"success\"}";
	status.status = true;
	return status;
}

/*
 * 1. Receives a SerializeRecords request from a shard
 * 2. Uses core to do the serialization
 * 3. Sends the results to the shard which initiated this serialization request(Failure or Success)
 */
SerializableCommandStatus DPInternalRequestHandler::internalSerializeRecordsCommand(Srch2Server * server, SerializableSerializeCommandInput * serailizeData){

	if(serailizeData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 5; // seralize records
		status.message = "";
		status.status = false;
		return status;
	}
	string exportedDataFileName = serailizeData->dataFileName;
    if(srch2::util::checkDirExistence(exportedDataFileName.c_str())){
        exportedDataFileName = "export_data.json";
    }
    server->indexer->exportData(exportedDataFileName);
	SerializableCommandStatus status;
	status.commandNumber = 5; // seralize records
	status.message = "{\"export\":\"success\"}";
	status.status = true;
	return status;

}

/*
 * 1. Receives a ResetLog request from a shard
 * 2. Uses core to reset log
 * 3. Sends the results to the shard which initiated this reset-log request(Failure or Success)
 */
SerializableCommandStatus DPInternalRequestHandler::internalResetLogCommand(Srch2Server * server, SerializableSerializeCommandInput * resetData){

	if(resetData == NULL || server == NULL){
		SerializableCommandStatus status;
		status.commandNumber = 6; // resetting log
		status.message = "";
		status.status = false;
		return status;
	}

    // create a FILE* pointer to point to the new logger file "logger.txt"
    FILE *logFile = fopen(server->indexDataConfig->getHTTPServerAccessLogFile().c_str(),
                "a");

    if (logFile == NULL) {
        srch2::util::Logger::error("Reopen Log file %s failed.",
                server->indexDataConfig->getHTTPServerAccessLogFile().c_str());
		SerializableCommandStatus status;
		status.commandNumber = 6; // resetting log
		status.message = "{\"message\":\"The logger file repointing failed. Could not create new logger file\", \"log\":\""
                + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}\n";
		status.status = false;
		return status;
    } else {
        FILE * oldLogger = srch2::util::Logger::swapLoggerFile(logFile);
        fclose(oldLogger);
		SerializableCommandStatus status;
		status.commandNumber = 6; // resetting log
		status.message = "{\"message\":\"The logger file repointing succeeded\", \"log\":\""
                + server->indexDataConfig->getHTTPServerAccessLogFile() + "\"}\n";
		status.status = true;
		return status;
    }
}


SerializableCommandStatus DPInternalRequestHandler::internalCommitCommand(Srch2Server * server, SerializableCommitCommandInput * resetData){
	SerializableCommandStatus status;
	status.commandNumber = 7; // commit
	if(resetData == NULL || server == NULL){
		status.message = "";
		status.status = false;
		return status;
	}

	//commit the index.
	if ( indexer->commit() == srch2::instantsearch::OP_SUCCESS)
	{
		status.message = "{\"commit\":\"success\"}";
		status.status = true;
	}
	else
	{
		status.message = "{\"commit\":\"failed\"}";
		status.status = true;
	}
	return status;
}

}
}
