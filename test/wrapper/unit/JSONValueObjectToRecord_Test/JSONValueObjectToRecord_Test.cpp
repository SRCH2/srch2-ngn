/*
 * JSONValueObjectToRecord_Test.cpp
 *
 *  Created on: Aug 1, 2014
 *      Author: Chen Liu liu@srch2.com
 */

/*
 * This test case tests the function "JSONRecordParser::_JSONValueObjectToRecord"
 * with different kinds of JSON string inputs.
 *
 * The input string may be incomplete, missing primary key or in wrong type.
 * And the function should not crash.
 */

#include "JSONRecordParser.h"
#include <iostream>
#include <string>
#include "ConfigManager.h"
#include <assert.h>
#include <stdlib.h>
#include "util/RecordSerializerUtil.h"

using namespace std;
using namespace srch2::httpwrapper;
namespace srch2is = srch2::instantsearch;

void afterTest(ConfigManager *conf, Record *record, Schema *schema,
        Schema * storedSchema) {
    delete conf;
    delete record;
    delete schema;
    delete storedSchema;
}

bool testRecord(const string& jsonRecord, const bool expectResult,
        const CoreInfo_t *indexDataConfig, RecordSerializer & recSerializer,
        Record *record) {
    //Parse JSON string to JSON object
    Json::Value root;
    Json::Reader reader;
    Json::FastWriter writer;
    stringstream log_str;

    bool parseSuccess = reader.parse(jsonRecord, root, false);

    /*
     * When parsing str to a JSON object, if the value field doesn't have
     * quote and is not correct integer, JSON  reader.parse() will be failed.
     * However, the root can still obtain the part that before the failed position.
     * e.g.
     *
     * {"id":2abc,"title":"test","genre":"male","year":"1000"}
     * ===>
     * {
     *    "id" : 2
     * }
     *
     * {"id":1,"title":"test","genre":"male","year":1000dafs}
     * {
     * "genre" : "male",
     * "id" : 1,
     * "title" : "test",
     * "year" : 1000
     * }
     *
     * {"id":1,"title":"test","genre":"male","year":}
     * {
     * "genre" : "male",
     * "id" : 1,
     * "title" : "test",
     * "year" : null
     * }
     */
    if (parseSuccess == false) {
        return false == expectResult;
    }
    return JSONRecordParser::_JSONValueObjectToRecord(record,
            writer.write(root), root, indexDataConfig, log_str, recSerializer)
            == expectResult;
}

void testCorrectRecord(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =

    "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testOnlyId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testOnlyIdTitle(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testIntYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":1000}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testIdText(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =

    "{\"id\":\"1\",\"title\":\"test\",\"genre\":\"male\",\"year\":1000}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testIntBadYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":1000dafs}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testEmptyYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testTextBadYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000dafs\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testTextBadYear2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"adsf\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testTextEmptyYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testNoYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\",\"genre\":\"male\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testIntBadId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":2abc,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testIntBadId2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":dafd,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testIntEmptyId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testTextEmptyId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":\"\",\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testTextBadId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":\"1123dsaf\",\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testTextBadId2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":\"dasf\",\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = true;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testNoId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

void testNoTitle(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectResult = false;
    ASSERT(
            testRecord(jsonRecord, expectResult, indexDataConfig, recSerializer, record));
}

int main(int argc, char **argv) {
    //Doing preparation for the tests.
    string configFile(string(getenv("srch2_config_file")) + "/conf.xml");

    CoreInfo_t* indexDataConfig;
    stringstream log_str;

    ConfigManager * conf = new ConfigManager(configFile);

    if (!conf->loadConfigFile()) {
        Logger::error("Error in loading the config file, therefore exiting.");
        exit(-1);
    }

    //Get indexDataConfig from the core
    for (ConfigManager::CoreInfoMap_t::const_iterator iterator =
            conf->coreInfoIterateBegin();
            iterator != conf->coreInfoIterateEnd(); iterator++) {

        indexDataConfig = const_cast<CoreInfo_t *>(conf->getCoreInfo(
                iterator->second->getName()));
    }

    //Create schema based on the config file.
    Schema * schema = JSONRecordParser::createAndPopulateSchema(
            indexDataConfig);
    Record * record = new srch2is::Record(schema);

    //Create stored schema for the test function
    Schema * storedSchema = Schema::create();
    RecordSerializerUtil::populateStoredSchema(storedSchema, schema);
    srch2::util::RecordSerializer recSerializer = RecordSerializer(
            *storedSchema);

    string jsonRecord;
    //Test the correct JSON record, the result should be true.
    testCorrectRecord(indexDataConfig, recSerializer, record);

    //Test the JSON record only contains ID, the result should be false since title is required.
    testOnlyId(indexDataConfig, recSerializer, record);

    //Test the JSON record only contains required fields, the result should be true.
    testOnlyIdTitle(indexDataConfig, recSerializer, record);

    //Test the JSON record when the year field is integer, the result should be true.
    testIntYear(indexDataConfig, recSerializer, record);

    //Test the JSON record when the id field is text, the result should be true.
    testIdText(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad year field (1000dafs), the result should be false.
    //JSON parser error.
    testIntBadYear(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad year field (empty), the result should be false.
    //JSON parser error.
    testEmptyYear(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad year field ("1000dafs"), the result should be true.
    //strtol will convert 1000dafs to 1000
    testTextBadYear(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad year field ("adsf"), the result should be true.
    //strtol will convert "adsf" to 0
    testTextBadYear2(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad year field (""), the result should be true.
    //strtol will convert "" to 0
    testTextEmptyYear(indexDataConfig, recSerializer, record);

    //Test the JSON record with no year field, the result should be true.
    testNoYear(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad id field (1dafd), the result should be false.
    //JSON parser error
    testIntBadId(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad id field (dafd), the result should be false.
    testIntBadId2(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad id field (empty), the result should be false.
    testIntEmptyId(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad id field (""), the result should be false.
    testTextEmptyId(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad id field ("1123dsaf"), the result should be true.
    //strtol will parse "1123dsaf" to 1123
    testTextBadId(indexDataConfig, recSerializer, record);

    //Test the JSON record with bad id field ("dasf"), the result should be true.
    //strtol will parse "dasf" to 0
    testTextBadId2(indexDataConfig, recSerializer, record);

    //Test the JSON record with no id field, the result should be false.
    testNoId(indexDataConfig, recSerializer, record);

    //Test the JSON record with no title(required=true) filed, the result should be false.
    testNoTitle(indexDataConfig, recSerializer, record);

    afterTest(conf, record, schema, storedSchema);
    return 0;
}
