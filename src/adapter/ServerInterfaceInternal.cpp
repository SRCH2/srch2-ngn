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
    stringstream debugMsg;
    debugMsg << "INSERT : ";

    // Parse example data
    Json::Value root;
    Json::Reader reader;
    Json::FastWriter writer;
    bool parseSuccess = reader.parse(jsonString, root, false);

    if (parseSuccess == false) {
        Logger::error("JSON object parse error %s", jsonString.c_str());
        return -1;
    } else {
        srch2is::Record *record = new srch2is::Record(
                this->server->indexer->getSchema());
        Json::Value response =
                srch2::httpwrapper::IndexWriteUtil::_insertCommand(
                        this->server->indexer, this->server->indexDataConfig,
                        root, record);
        debugMsg << writer.write(response);
        record->clear();
        delete record;

        Logger::debug(debugMsg.str().c_str());
        return response["insert"].asString().compare("success");
    }
}

//Called by the connector, accepts record pkey and delete from the index
int ServerInterfaceInternal::deleteRecord(const std::string& primaryKey) {
    stringstream debugMsg;
    debugMsg << "DELETE : ";
    bool op_success = false;

    Json::FastWriter writer;
    if (primaryKey.size()) {
        debugMsg << "{\"rid\":\"" << primaryKey << "\",\"delete\":\"";
        //delete the record from the index
        switch (server->indexer->deleteRecord(primaryKey)) {
        case srch2is::OP_FAIL: {
            debugMsg << "failed\",\"reason\":\"no record with given"
                    " primary key\"}";
            break;
        }
        default: // OP_SUCCESS.
        {
            debugMsg << "success\"}";
            op_success = true;
        }
        };
    } else {
        debugMsg << "{\"rid\":\"NULL\",\"delete\":\"failed\",\"reason\":\""
                "no record with given primary key\"}";
    }

    Logger::debug(debugMsg.str().c_str());
    return op_success;
}

/*
 * Called by the connector, accepts record pkey and json format
 * record and delete the old one create a new one.
 * Primary key is a must because in mongodb, the pk in jsonString is an object.
 */
int ServerInterfaceInternal::updateRecord(const std::string& pk,
        const std::string& jsonString) {
    stringstream debugMsg;
    debugMsg << "UPDATE : ";
    bool op_success = false;

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
                server->indexer->deleteRecordGetInternalId(pk,
                        deletedInternalRecordId);
        if (ret == srch2is::OP_FAIL) {
            debugMsg << "failed\",\"reason\":\"no record "
                    "with given primary key\"}";
        } else {
            Logger::debug("DATABASE_LISTENER:UPDATE: deleted record ");
            op_success = true;
        }

        if (!op_success) {
            Logger::debug(debugMsg.str().c_str());
            return false;
        }
        op_success = false;

        if (server->indexer->getNumberOfDocumentsInIndex()
                < this->server->indexDataConfig->getDocumentLimit()) {

            srch2is::Record *record = new srch2is::Record(
                    server->indexer->getSchema());
            Json::Value response =
                    srch2::httpwrapper::IndexWriteUtil::_insertCommand(
                            server->indexer, this->server->indexDataConfig,
                            root, record);
            record->clear();
            delete record;

            if (response["insert"].asString().compare("success") == 0) {
                op_success = true;
                Logger::debug("DATABASE_LISTENER:UPDATE: inserted record ");
            } else {
                srch2::instantsearch::INDEXWRITE_RETVAL ret =
                        server->indexer->recoverRecord(pk,
                                deletedInternalRecordId);
                if (ret == srch2is::OP_FAIL) {
                    Logger::error("DATABASE_LISTENER:UPDATE: insert record "
                            "failed and resume the deleted old "
                            "record failed too.");
                }
            }

        } else {
            debugMsg << "failed\",\"reason\":\"insert: Document limit reached."
                    << endl;
            // reaching here means the insert failed,
            //  need to resume the deleted old record
            srch2::instantsearch::INDEXWRITE_RETVAL ret =
                    server->indexer->recoverRecord(pk, deletedInternalRecordId);
            if (ret == srch2is::OP_FAIL) {
                Logger::error("DATABASE_LISTENER:UPDATE: insert record "
                        "failed and resume the deleted old "
                        "record failed too.");
            }
        }
    }
    Logger::debug(debugMsg.str().c_str());
    return op_success;
}

//Call save index to the disk manually.
int ServerInterfaceInternal::saveChanges() {
    server->indexer->save();
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
            *this->server->indexDataConfig->getDbParameters();
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
    return server->indexDataConfig->getDataSourceType()
            == srch2::httpwrapper::DATA_SOURCE_DATABASE;
}

ServerInterfaceInternal::~ServerInterfaceInternal() {
}
