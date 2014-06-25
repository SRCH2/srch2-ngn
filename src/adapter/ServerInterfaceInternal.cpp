/*
 * ServerInterfaceInternal.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: liusrch2
 */

//#include "Srch2Server.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <map>
#include "ServerInterfaceInternal.h"
#include "DataConnectorThread.h"
#include "util/Logger.h"
#include "json/json.h"
#include "AnalyzerFactory.h"
#include "JSONRecordParser.h"
#include "boost/algorithm/string.hpp"
#include <instantsearch/Constants.h>
#include "IndexWriteUtil.h"
#include "util/RecordSerializerUtil.h"
#include <instantsearch/Indexer.h>
#include <instantsearch/Record.h>
#include <instantsearch/Schema.h>
using namespace std;


const std::string ServerInterfaceInternal::DB_CONNECTORS_PATH =
//		"/home/liusrch2/srch2-ngn/db_connectors/mysql/mysql-replication-listener/build/lib/";
		"/home/liusrch2/srch2-ngn/db_connectors/build/";
const std::string ServerInterfaceInternal::DYNAMIC_LIBRARY_SUFFIX = "Connector.so";
const std::string ServerInterfaceInternal::DYNAMIC_LIBRARY_PREFIX = "lib";
const std::string ServerInterfaceInternal::PRIMARY_KEY="uniqueKey";
const std::string ServerInterfaceInternal::DATABASE_NAME="db";
const std::string ServerInterfaceInternal::DATABASE_PORT="port";
const std::string ServerInterfaceInternal::DATABASE_HOST="host";
const std::string ServerInterfaceInternal::DATABASE_COLLECTION="collection";
const std::string ServerInterfaceInternal::DATABASE_TYPE_NAME="dbTypeName";
const std::string ServerInterfaceInternal::DATABASE_LISTENER_WATI_TIME="listenerWaitTime";
const std::string ServerInterfaceInternal::DATABASE_MAX_RETRY_ON_FALIFURE="maxRetryOnFailure";
const std::string ServerInterfaceInternal::DATABASE_MAX_RETRY_COUNT="maxRetryCount";
const std::string ServerInterfaceInternal::SRCH2HOME="srch2Home";
const std::string ServerInterfaceInternal::INDEXTYPE="indexType";

ServerInterfaceInternal::ServerInterfaceInternal(void *server) {
	this->server=(srch2::httpwrapper::Srch2Server*)server;
	this->connectorConfig = new std::map<std::string, std::string>();
	populateConnectorConfig();
}


int ServerInterfaceInternal::insertRecord(std::string jsonString) {
	stringstream errorMsg;
	errorMsg << "INSERT : ";

	// Parse example data
	Json::Value root;
	Json::Reader reader;
	bool parseSuccess = reader.parse(jsonString, root, false);

	if (parseSuccess == false) {
		Logger::error("JSON object parse error %s", jsonString.c_str());
	} else {
		srch2is::Record *record = new srch2is::Record(
				this->server->indexer->getSchema());
		srch2::httpwrapper::IndexWriteUtil::_insertCommand(this->server->indexer, this->server->indexDataConfig, root, record,
				errorMsg);
		record->clear();
		delete record;
	}

	Logger::debug(errorMsg.str().c_str());
	return 0;
}

int ServerInterfaceInternal::deleteRecord(std::string primaryKey) {
	stringstream errorMsg;
	errorMsg << "DELETE : ";

	if (primaryKey.size()) {
		errorMsg << "{\"rid\":\"" << primaryKey << "\",\"delete\":\"";
		//delete the record from the index
		switch (server->indexer->deleteRecord(primaryKey)) {
		case srch2is::OP_FAIL: {
			errorMsg
					<< "failed\",\"reason\":\"no record with given primary key\"}";
			break;
		}
		default: // OP_SUCCESS.
		{
			errorMsg << "success\"}";
		}
		};
	} else {
		errorMsg
				<< "{\"rid\":\"NULL\",\"delete\":\"failed\",\"reason\":\"no record with given primary key\"}";
	}

	Logger::debug(errorMsg.str().c_str());
	return 0;
}

int ServerInterfaceInternal::updateRecord(std::string pk,std::string jsonString) {
	stringstream errorMsg;
	errorMsg << "UPDATE : ";

	//Parse JSON Object;
	Json::Value root;
	Json::Reader reader;
	bool parseSuccess = reader.parse(jsonString, root, false);
	if (parseSuccess == false) {
		Logger::error("UPDATE : JSON object parse error %s",
				jsonString.c_str());
	}

	string primaryKeyStringValue=pk;
	unsigned deletedInternalRecordId;
	if (primaryKeyStringValue.size()) {
		srch2is::INDEXWRITE_RETVAL ret =
				server->indexer->deleteRecordGetInternalId(
						primaryKeyStringValue, deletedInternalRecordId);
		if (ret == srch2is::OP_FAIL) {
			errorMsg
					<< "failed\",\"reason\":\"no record with given primary key\"}";
		} else
			Logger::debug("DATABASE_LISTENER:UPDATE: deleted record ");

		if (server->indexer->getNumberOfDocumentsInIndex()
				< this->server->indexDataConfig->getDocumentLimit()) {

			srch2is::Record *record = new srch2is::Record(
					server->indexer->getSchema());
			srch2::httpwrapper::IndexWriteUtil::_insertCommand(server->indexer,
					this->server->indexDataConfig, root, record, errorMsg);
			record->clear();
			delete record;
			Logger::debug("DATABASE_LISTENER:UPDATE: inserted record ");
		} else {
			errorMsg << "failed\",\"reason\":\"insert: Document limit reached."
					<< endl;
			/// reaching here means the insert failed, need to resume the deleted old record
			srch2::instantsearch::INDEXWRITE_RETVAL ret =
					server->indexer->recoverRecord(primaryKeyStringValue,
							deletedInternalRecordId);
		}
	}
	 Logger::debug(errorMsg.str().c_str());
	 return 0;
}

std::string ServerInterfaceInternal::configLookUp(std::string key){
	return (*this->connectorConfig)[key];
}


void ServerInterfaceInternal::populateConnectorConfig() {

	srch2::httpwrapper::Srch2Server *srch2Server =
			(srch2::httpwrapper::Srch2Server *) server;

	srch2::httpwrapper::DataSourceType dbType=srch2Server->indexDataConfig->getDataSourceType();

	(*connectorConfig)[PRIMARY_KEY]=srch2Server->indexDataConfig->getPrimaryKey();

	switch (dbType) {
	case srch2::httpwrapper::DATA_SOURCE_NOT_SPECIFIED:
		break;
	case srch2::httpwrapper::DATA_SOURCE_JSON_FILE:
		break;
	case srch2::httpwrapper::DATA_SOURCE_MONGO_DB:{
		(*connectorConfig)[DATABASE_TYPE_NAME]="mongodb";
		(*connectorConfig)[DATABASE_NAME]=srch2Server->indexDataConfig->getMongoDbName();
		(*connectorConfig)[DATABASE_HOST]=srch2Server->indexDataConfig->getMongoServerHost();
		(*connectorConfig)[DATABASE_PORT]=srch2Server->indexDataConfig->getMongoServerPort();
		(*connectorConfig)[DATABASE_COLLECTION]=srch2Server->indexDataConfig->getMongoCollection();
		(*connectorConfig)[DATABASE_LISTENER_WATI_TIME]=srch2Server->indexDataConfig->getMongoListenerWaitTime();
		(*connectorConfig)[DATABASE_MAX_RETRY_ON_FALIFURE]=srch2Server->indexDataConfig->getMongoListenerMaxRetryOnFailure();
		(*connectorConfig)[DATABASE_MAX_RETRY_COUNT]=srch2Server->indexDataConfig->getMongoListenerMaxRetryCount();
		(*connectorConfig)[SRCH2HOME]=srch2Server->indexDataConfig->getIndexPath();

		srch2::instantsearch::IndexType it  = (srch2::instantsearch::IndexType)srch2Server->indexDataConfig->getIndexType();
		std::stringstream ss;ss << it;std::string itStr;   ss >> itStr;
		(*connectorConfig)[INDEXTYPE]=itStr;
	}
		break;
	default:
		break;
	}
}

ServerInterfaceInternal::~ServerInterfaceInternal(){
	delete connectorConfig;
}
