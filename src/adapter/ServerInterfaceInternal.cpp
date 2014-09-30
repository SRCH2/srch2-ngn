/*
 * ServerInterfaceInternal.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: Chen Liu at SRCH2
 */

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

ServerInterfaceInternal::ServerInterfaceInternal(void *server) {
    this->server = (srch2::httpwrapper::Srch2Server*) server;
}

//Called by the connector, accepts json format record and insert into the index
int ServerInterfaceInternal::insertRecord(const std::string& jsonString) {
    stringstream errorMsg;
    errorMsg << "INSERT : ";

    // Parse example data
    Json::Value root;
    Json::Reader reader;
    bool parseSuccess = reader.parse(jsonString, root, false);

    if (parseSuccess == false) {
        Logger::error("JSON object parse error %s", jsonString.c_str());
        return -1;
    } else {
        srch2is::Record *record = new srch2is::Record(
                this->server->getIndexer()->getSchema());
        srch2::httpwrapper::IndexWriteUtil::_insertCommand(
                this->server->getIndexer(), this->server->getCoreInfo(), root,
                record, errorMsg);
        record->clear();
        delete record;
    }

    Logger::debug(errorMsg.str().c_str());
    return 0;
}

//Called by the connector, accepts record pkey and delete from the index
int ServerInterfaceInternal::deleteRecord(const std::string& primaryKey) {
    stringstream errorMsg;
    errorMsg << "DELETE : ";

    if (primaryKey.size()) {
        errorMsg << "{\"rid\":\"" << primaryKey << "\",\"delete\":\"";
        //delete the record from the index
        switch (server->getIndexer()->deleteRecord(primaryKey)) {
        case srch2is::OP_FAIL: {
            errorMsg << "failed\",\"reason\":\"no record with given"
                    " primary key\"}";
            break;
        }
        default: // OP_SUCCESS.
        {
            errorMsg << "success\"}";
        }
        };
    } else {
        errorMsg << "{\"rid\":\"NULL\",\"delete\":\"failed\",\"reason\":\""
                "no record with given primary key\"}";
    }

    Logger::debug(errorMsg.str().c_str());
    return 0;
}

/*
 * Called by the connector, accepts record pkey and json format
 * record and delete the old one create a new one.
 * Primary key is a must because in mongodb, the pk in jsonString is an object.
 */
int ServerInterfaceInternal::updateRecord(const std::string& pk,
        const std::string& jsonString) {
    stringstream errorMsg;
    errorMsg << "UPDATE : ";

    //Parse JSON Object;
    Json::Value root;
    Json::Reader reader;
    bool parseSuccess = reader.parse(jsonString, root, false);
    if (parseSuccess == false) {
        Logger::error("UPDATE : JSON object parse error %s",
                jsonString.c_str());
        return -1;
    }

    unsigned deletedInternalRecordId;
    if (pk.size()) {
        srch2is::INDEXWRITE_RETVAL ret =
                server->getIndexer()->deleteRecordGetInternalId(pk,
                        deletedInternalRecordId);
        if (ret == srch2is::OP_FAIL) {
            errorMsg << "failed\",\"reason\":\"no record "
                    "with given primary key\"}";
        } else
            Logger::debug("DATABASE_LISTENER:UPDATE: deleted record ");

        if (server->getIndexer()->getNumberOfDocumentsInIndex()
                < this->server->getCoreInfo()->getDocumentLimit()) {

            srch2is::Record *record = new srch2is::Record(
                    server->getIndexer()->getSchema());
            srch2::httpwrapper::IndexWriteUtil::_insertCommand(server->getIndexer(),
                    this->server->getCoreInfo(), root, record, errorMsg);
            record->clear();
            delete record;
            Logger::debug("DATABASE_LISTENER:UPDATE: inserted record ");
        } else {
            errorMsg << "failed\",\"reason\":\"insert: Document limit reached."
                    << endl;
            /// reaching here means the insert failed,
            //  need to resume the deleted old record
            srch2::instantsearch::INDEXWRITE_RETVAL ret =
                    server->getIndexer()->recoverRecord(pk, deletedInternalRecordId);
        }
    }
    Logger::debug(errorMsg.str().c_str());
    return 0;
}

//Call save index to the disk manually.
int ServerInterfaceInternal::saveChanges() {
    server->getIndexer()->save();
    Logger::debug("ServerInterface calls saveChanges");
    return 0;
}

/*
 * Find the config file value. the key is the same name in the config file.
 * Also, change the input string to lower case to match the key.
 */
int ServerInterfaceInternal::configLookUp(const std::string& key,
        std::string & value) {
    std::string newKey = key;
    std::transform(newKey.begin(), newKey.end(), newKey.begin(), ::tolower);

    std::map<std::string, std::string>::const_iterator it;
    const std::map<std::string, std::string> & dbParameters =
            *this->server->getCoreInfo()->getDbParameters();
    it = dbParameters.find(newKey);
    if (it != dbParameters.end()) {
        value = it->second;
        return 0;
    } else {
        value = "";
        return -1;
    }
}

//return false if the source is not database
bool ServerInterfaceInternal::isDatabase() {
    return server->getCoreInfo()->getDataSourceType()
            == srch2::httpwrapper::DATA_SOURCE_DATABASE;
}

ServerInterfaceInternal::~ServerInterfaceInternal() {
}
