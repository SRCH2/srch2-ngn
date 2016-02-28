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
 * JSONValueObjectToRecord_Test.cpp
 *
 *  Created on: Aug 1, 2014
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

bool testRecord(const string& jsonRecord, const bool expectedResult,
        const CoreInfo_t *indexDataConfig, RecordSerializer & recSerializer,
        Record *record) {
    //Parse JSON string to JSON object
    Json::Value root;
    Json::Reader reader;
    stringstream log_str;

    bool parseSuccess = reader.parse(jsonRecord, root, false);

    /*
     * When parsing a string to a JSON object, if the value field doesn't have
     * quote and is not correct integer, the JSON reader.parse() will fail.
     * However, the root can still generate part of the JSON object based on
     * the JSON string that is before the failed position.
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
        return false == expectedResult;
    }
    return JSONRecordParser::_JSONValueObjectToRecord(record,
            root, indexDataConfig, log_str, recSerializer)
            == expectedResult;
}

//Test the correct JSON record, the result should be true.
void testCorrectRecord(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
    "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record only contains ID, the result should be false since title is required.
void testOnlyId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record only contains required fields, the result should be true.
//All other fields are missing (incomplete JSON record)
void testOnlyIdTitle(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\"}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record when the year field is integer, the result should be true.
void testIntYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":1000}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record when the id field is text, the result should be true.
void testIdText(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =

    "{\"id\":\"1\",\"title\":\"test\",\"genre\":\"male\",\"year\":1000}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad year field (1000dafs), the result should be false.
//JSON parser error.
void testIntBadYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":1000dafs}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad year field (empty), the result should be false.
//JSON parser error.
void testEmptyYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad year field ("1000dafs"), the result should be false.
void testTextBadYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000dafs\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad year field ("adsf"), the result should be false.
void testTextBadYear2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"adsf\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad year field (""), the result should be true.
//strtol will convert "" to 0, and will use the default value.
void testTextEmptyYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":1,\"title\":\"test\",\"genre\":\"male\",\"year\":\"\"}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with no year field(not required), the result should be true.
void testNoYear(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\",\"genre\":\"male\"}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad id field (1dafd), the result should be false.
//JSON parser error
void testIntBadId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":2abc,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad id field (dafd), the result should be false.
void testIntBadId2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":dafd,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad id field (empty), the result should be false.
void testIntEmptyId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":,\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad id field (""), the result should be false.
//Because id  is required
void testTextEmptyId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":\"\",\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad id field ("1123dsaf"), the result should be true.
//no matter user defined it as integer or text or float
void testTextBadId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":\"1123dsaf\",\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with bad id field ("dasf"), the result should be true.
//no matter user defined it as integer or text or float
void testTextBadId2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"id\":\"dasf\",\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = true;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with no id field, the result should be false.
void testNoId(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord =
            "{\"title\":\"test\",\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with no title(required=true) field, the result should be false.
void testNoTitle(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"genre\":\"male\",\"year\":\"1000\"}";
    bool expectedResult = false;
    ASSERT(
            testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with empty title(required) field, the result should be false.
void testEmptyTitle1(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":,\"genre\":\"male\",\"year\":\"1000\"}";
        bool expectedResult = false;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with "" title(required) field, the result should be false.
void testEmptyTitle2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"\",\"genre\":\"male\",\"year\":\"1000\"}";
        bool expectedResult = false;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with "NULL" title(required) field, the result should be false.
void testNULLTitle(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"NULL\",\"genre\":\"male\",\"year\":\"1000\"}";
        bool expectedResult = false;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with "null" title(required) field, the result should be false.
void testnullTitle(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"null\",\"genre\":\"male\",\"year\":\"1000\"}";
        bool expectedResult = false;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with empty genre(not required) field, the result should be false.
void testEmptyGenre1(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\",\"genre\":,\"year\":\"1000\"}";
        bool expectedResult = false;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with "" genre(not required) field, the result should be true.
void testEmptyGenre2(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\",\"genre\":\"\",\"year\":\"1000\"}";
        bool expectedResult = true;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with no genre(not required) field, the result should be true.
void testNoGenre(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\",\"year\":\"1000\"}";
        bool expectedResult = true;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with NULL genre(not required) field, the result should be true.
void testNULLGenre(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\",\"genre\":\"NULL\",\"year\":\"1000\"}";
        bool expectedResult = true;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}

//Test the JSON record with null genre(not required) field, the result should be true.
void testnullGenre(const CoreInfo_t *indexDataConfig,
        RecordSerializer & recSerializer, Record *record) {
    string jsonRecord = "{\"id\":1,\"title\":\"test\",\"genre\":\"null\",\"year\":\"1000\"}";
        bool expectedResult = true;
        ASSERT(
                testRecord(jsonRecord, expectedResult, indexDataConfig, recSerializer, record));
}


int main(int argc, char **argv) {
    //Doing preparation for the tests.
    string configFile(string(getenv("srch2_config_file")) + "/conf.xml");

    CoreInfo_t* indexDataConfig = NULL;
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
    testCorrectRecord(indexDataConfig, recSerializer, record);

    testOnlyId(indexDataConfig, recSerializer, record);

    testOnlyIdTitle(indexDataConfig, recSerializer, record);

    testIntYear(indexDataConfig, recSerializer, record);

    testIdText(indexDataConfig, recSerializer, record);

    testIntBadYear(indexDataConfig, recSerializer, record);

    testEmptyYear(indexDataConfig, recSerializer, record);

    testTextBadYear(indexDataConfig, recSerializer, record);

    testTextBadYear2(indexDataConfig, recSerializer, record);

    testTextEmptyYear(indexDataConfig, recSerializer, record);

    testNoYear(indexDataConfig, recSerializer, record);

    testIntBadId(indexDataConfig, recSerializer, record);

    testIntBadId2(indexDataConfig, recSerializer, record);

    testIntEmptyId(indexDataConfig, recSerializer, record);

    testTextEmptyId(indexDataConfig, recSerializer, record);

    testTextBadId(indexDataConfig, recSerializer, record);

    testTextBadId2(indexDataConfig, recSerializer, record);

    testNoId(indexDataConfig, recSerializer, record);

    testNoTitle(indexDataConfig, recSerializer, record);

    testEmptyTitle1(indexDataConfig, recSerializer, record);

    testEmptyTitle2(indexDataConfig, recSerializer, record);

    testNULLTitle(indexDataConfig, recSerializer, record);

    testnullTitle(indexDataConfig, recSerializer, record);

    testEmptyGenre1(indexDataConfig, recSerializer, record);

    testEmptyGenre2(indexDataConfig, recSerializer, record);

    testNoGenre(indexDataConfig, recSerializer, record);

    testNULLGenre(indexDataConfig, recSerializer, record);

    testnullGenre(indexDataConfig, recSerializer, record);

    afterTest(conf, record, schema, storedSchema);

    return 0;
}
