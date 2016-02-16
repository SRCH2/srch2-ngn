/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
#include "analyzer/AnalyzerFactory.h"
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
                this->server->getIndexer()->getSchema());
        Json::Value response = srch2::httpwrapper::IndexWriteUtil::_insertCommand(
                this->server->getIndexer(), this->server->getCoreInfo(), root,
                record);
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
        switch (server->getIndexer()->deleteRecord(primaryKey)) {
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
    if (op_success) {
        return 0;
    } else {
        return -1;
    }
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

    //Parse JSON Object;
    Json::Value root;
    Json::Reader reader;
    bool parseSuccess = reader.parse(jsonString, root, false);
    if (parseSuccess == false) {
        Logger::error("DATABASE_LISTENER: JSON object parse error %s "
                "while updating the record.", jsonString.c_str());
        return -1;
    }

    unsigned deletedInternalRecordId;
    if (pk.size()) {
        srch2is::INDEXWRITE_RETVAL ret =
                server->getIndexer()->deleteRecordGetInternalId(pk,
                        deletedInternalRecordId);
        if (ret == srch2is::OP_FAIL) {
            debugMsg << "failed\",\"reason\":\"no record "
                    "with given primary key\"}";

            Logger::debug(debugMsg.str().c_str());
            return -1;
        }

        if (server->getIndexer()->getNumberOfDocumentsInIndex()
                < this->server->getCoreInfo()->getDocumentLimit()) {

            srch2is::Record *record = new srch2is::Record(

                    server->getIndexer()->getSchema());
            Json::Value response = srch2::httpwrapper::IndexWriteUtil::_insertCommand(server->getIndexer(),
                    this->server->getCoreInfo(), root, record);

            record->clear();
            delete record;
        } else {
            debugMsg << "failed\",\"reason\":\"insert: Document limit reached."
                    << endl;
            // reaching here means the insert failed,
            //  need to resume the deleted old record
            srch2::instantsearch::INDEXWRITE_RETVAL ret =
                    server->getIndexer()->recoverRecord(pk, deletedInternalRecordId);

            if (ret == srch2is::OP_FAIL) {
                Logger::error("DATABASE_LISTENER: Update error while trying to "
                        "resume the deleted old record.");
            }
        }
    }
    Logger::debug(debugMsg.str().c_str());
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
