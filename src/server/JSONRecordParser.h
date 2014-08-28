//$Id: JSONRecordParser.h 3456 2013-06-14 02:11:13Z jiaying $

#ifndef _DAEMONDATASOURCE_H_
#define _DAEMONDATASOURCE_H_

#include "json/json.h"
#include <instantsearch/Schema.h>
#include <instantsearch/Analyzer.h>
#include <instantsearch/Record.h>
#include <instantsearch/Indexer.h>
#include "util/RecordSerializer.h"

#include "ConfigManager.h"

namespace srch2is = srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

class JSONRecordParser {
public:
    static bool populateRecordFromJSON(const std::string &inputLine,
            const CoreInfo_t *indexDataContainerConf, srch2is::Record *record,
            std::stringstream &error, RecordSerializer& compactRecSerializer);
    static bool _JSONValueObjectToRecord(srch2is::Record *record,
            const std::string &inputLine, const Json::Value &root,
            const CoreInfo_t *indexDataContainerConf, std::stringstream &error,
            RecordSerializer& compactRecSerializer);
    static bool _extractRoleIds(vector<string> &roleIds, string &primaryKeyID,
    		const Json::Value &root, const CoreInfo_t *indexDataContainerConf, std::stringstream &error);
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
    static unsigned createNewIndexFromFile(srch2is::Indexer *indexer,
            Schema * storedAttrSchema,
            const CoreInfo_t *indexDataContainerConf);

    static void addAccessControlsFromFile(srch2is::Indexer *indexer,
                const CoreInfo_t *indexDataContainerConf, srch2is::Indexer *roleCoreIndexer);
};

}
}

#endif // _DAEMONDATASOURCE_H_
