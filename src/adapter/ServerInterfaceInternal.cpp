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


ServerInterfaceInternal::ServerInterfaceInternal(void *server, std::map<std::string, std::string> *connectorConfig) {
	this->server=(srch2::httpwrapper::Srch2Server*)server;
	this->connectorConfig = connectorConfig;
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

ServerInterfaceInternal::~ServerInterfaceInternal(){

}
