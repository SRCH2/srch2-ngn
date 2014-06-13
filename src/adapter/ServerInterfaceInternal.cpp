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

class Srch2Server;

ServerInterfaceInternal::ServerInterfaceInternal(void *server, std::map<std::string, std::string> *connectorConfig) {
	std::cout << "ServerInterfaceInternal::ServerInterfaceInternal()" << std::endl;
	this->server=server;
	this->connectorConfig = connectorConfig;
}

int ServerInterfaceInternal::insertRecord(std::string jsonString) {
	std::cout << "1.ServerInterfaceInternal::insertRecord()" << std::endl;

	Srch2Server * srch2server = (Srch2Server*)this->server;

	stringstream errorMsg;
//	const CoreInfo_t *config = srch2server->indexDataConfig;

	errorMsg << "INSERT : ";
	// Parse example data
	Json::Value root;
	Json::Reader reader;
	bool parseSuccess = reader.parse(jsonString, root, false);

	if (parseSuccess == false) {
//		Logger::error("JSON object parse error %s", jsonString.c_str());
	} else {
		srch2is::Record *record = new srch2is::Record(
				srch2server->indexer->getSchema());
//		IndexWriteUtil::_insertCommand(srch2server->indexer, config, root, record,
//				errorMsg);
		record->clear();
		delete record;
	}
}

int ServerInterfaceInternal::deleteRecord(std::string primaryKey) {
	std::cout << "2.ServerInterfaceInternal::deleteRecord()" << std::endl;
}

int ServerInterfaceInternal::updateRecord(std::string jsonString) {
	std::cout << "3.ServerInterfaceInternal::updateRecord()" << std::endl;
}

std::string ServerInterfaceInternal::configLookUp(std::string key){

}

ServerInterfaceInternal::~ServerInterfaceInternal(){
	delete this;
}
