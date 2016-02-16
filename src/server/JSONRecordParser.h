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
//$Id: JSONRecordParser.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef _DAEMONDATASOURCE_H_
#define _DAEMONDATASOURCE_H_

#include "json/json.h"
#include <instantsearch/Schema.h>
#include <instantsearch/Analyzer.h>
#include <instantsearch/Record.h>
#include <instantsearch/Indexer.h>
#include "util/RecordSerializer.h"

#include "src/sharding/configuration/ConfigManager.h"

namespace srch2is = srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class JSONRecordParser {
public:
    static bool populateRecordFromJSON(const std::string &inputLine,
            const CoreInfo_t *indexDataContainerConf, srch2is::Record *record,
            std::stringstream &error, RecordSerializer& compactRecSerializer);
    static bool _JSONValueObjectToRecord(srch2is::Record *record,
            const Json::Value &root,const CoreInfo_t *indexDataContainerConf, 
            std::stringstream &error,
            RecordSerializer& compactRecSerializer);
    static bool _extractResourceAndRoleIds(vector<string> &roleIds, string &resourcePrimaryKeyID,
    		const Json::Value &root, const CoreInfo_t *indexDataContainerConf, std::stringstream &error);
    static bool _extractRoleAndResourceIds(vector<string> &resourceIds, string &rolePrimaryKeyID,
        		const Json::Value &root, const CoreInfo_t *indexDataContainerConf, std::stringstream &error);
    static bool _extractRoleIds(vector<string> &roleIds, const Json::Value &root,
    		const CoreInfo_t *indexDataContainerConf, std::stringstream &error);
    static bool getAclInfoFromJSON(vector<string> &roleIds, string &primaryKeyID,
    		const string& inputLine, const CoreInfo_t *indexDataContainerConf, std::stringstream &error);
    static srch2is::Schema* createAndPopulateSchema(
            const CoreInfo_t *indexDataContainerConf);
private:
    static bool getJsonValueString(const Json::Value &jsonValue,
            const std::string &key, std::vector<std::string> &stringValue,
            const string &configName);
    static bool getJsonValueDateAndTime(const Json::Value &jsonValue,
            const std::string &key, std::vector<std::string> &stringValue,
            const string &configName);
    static bool getJsonValueDouble(const Json::Value &jsonValue,
            const std::string &key, double &doubleValue,
            const string& configName);
    static bool setRecordPrimaryKey(srch2is::Record *record,
            const Json::Value &root, const CoreInfo_t *indexDataContainerConf,
            std::stringstream &error);
    static bool setRecordSearchableValue(srch2is::Record *record,
            const Json::Value &root, const CoreInfo_t *indexDataContainerConf,
            std::stringstream &error);
    static bool setRecordRefiningValue(srch2is::Record *record,
            const Json::Value &root, const CoreInfo_t *indexDataContainerConf,
            std::stringstream &error);
    static bool setCompactRecordSearchableValue(const srch2is::Record *record,
            RecordSerializer& compactRecSerializer,std::stringstream &error);
    static bool setCompactRecordRefiningValue(const srch2is::Record *record,
            RecordSerializer& compactRecSerializer,std::stringstream &error);
    static bool setRecordLocationValue(srch2is::Record *record,
            const Json::Value &root, const CoreInfo_t *indexDataContainerConf,
            std::stringstream &error);
    static bool setRecordBoostValue(srch2is::Record *record,
            const Json::Value &root, const CoreInfo_t *indexDataContainerConf);
};

class DaemonDataSource {
public:
public:
	static unsigned createNewIndexFromFile(srch2is::Indexer *indexer, Schema * storedAttrSchema,
			const CoreInfo_t *indexDataContainerConf, const string & dataFilePath);

    static void addRecordAclFile(srch2is::Indexer *indexer,
                const CoreInfo_t *indexDataContainerConf);
};

}
}

#endif // _DAEMONDATASOURCE_H_
