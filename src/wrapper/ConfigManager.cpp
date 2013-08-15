//$Id: ConfigManager.h 2013-07-5 02:11:13Z iman $

#include "ConfigManager.h"

#include "util/xmlParser/pugixml.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <assert.h>
#include "util/Assert.h"
#include "util/Logger.h"
#include <boost/algorithm/string.hpp>
#include <sys/stat.h>


using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace pugi; // it is related to the pgixml.hpp which is a xml parser.

namespace srch2 {
namespace httpwrapper {

ConfigManager::ConfigManager(string& configFile) {
    this->configFile = configFile;
}

void ConfigManager::loadConfigFile() {
    cout << "Reading config file: " << this->configFile << endl;
    xml_document configDoc;
    // Checks if the xml file is parsed correctly or not.
    if (!configDoc.load_file(this->configFile.c_str())) {
        //TODO: TO_ASK: do we need cout or not?
        cout << "file " << this->configFile << " parsed with errors." << endl;
        Logger::debug("%s parsed with errors.\n", this->configFile.c_str());
        return;
    }

    bool configSuccess = true;
    std::stringstream parseError;
    // parse the config file and set the variables.
    this->parse(configDoc, configSuccess, parseError);

    if (!configSuccess) {
        cout << "Error while reading the config file" << endl;
        cout << parseError.str() << endl; // assumption: parseError is set properly
        exit(-1);
    }
}

void ConfigManager::parse(pugi::xml_document& configDoc, bool &configSuccess, std::stringstream &parseError) {
    string tempUse = ""; // This is just for temporary use.


    // srch2Home is a required field
    xml_node configAttribute = configDoc.child("config").child("srch2Home");
    if (configAttribute && configAttribute.text()) { // checks if the config/srch2Home has any text in it or not
        this->srch2Home = string(configAttribute.text().get());
    } else {
        parseError << "srch2Home is not set.\n";
        configSuccess = false;
        return;
    }


    /*
     * schema: beginning
     */
    /*
     * field: beginning
     */
    bool hasLatitude = false;
    bool hasLongitude = false;
    vector<string> searchableFieldsVector;
    vector<string> searchableFieldTypesVector;
    vector<bool> searchableFieldIndexsVector;
    configAttribute = configDoc.child("schema").child("fields");
    if (configAttribute) {
        for (xml_node field = configAttribute.first_child(); field; field =
                field.next_sibling()) {
            if (string(field.name()).compare("field") == 0) {
                // Checking if the values are empty or not
                if (string(field.attribute("name").value()).compare("") != 0
                        && string(field.attribute("type").value()).compare("")
                                != 0
                        && string(field.attribute("indexed").value()).compare("") != 0) {

                    searchableFieldsVector.push_back(
                            string(field.attribute("name").value()));

                    // Checking the validity of field type
                    tempUse = string(field.attribute("type").value());
                    if (this->isValidFieldType(tempUse)) {
                        searchableFieldTypesVector.push_back(tempUse);
                    } else {
                        parseError << "Config File Error: " << tempUse
                                << " is an unknown field type.\n";
                        configSuccess = false;
                        return;
                    }

                    tempUse = field.attribute("indexed").value();
                    if (isValidBool(tempUse)) {
                        searchableFieldIndexsVector.push_back(
                                field.attribute("indexed").as_bool());
                    } else {
                        parseError << "Config File Error: can not convert "
                                << tempUse << " to boolean type.\n";
                        configSuccess = false;
                        return;
                    }

                    // Checks for geo types. location_latitude and location_longitude are geo types
                    if (string(field.attribute("type").value()).compare("location_latitude") == 0) {
                        hasLatitude = true;
                        this->fieldLatitude = string(
                                field.attribute("name").value());
                    }
                    if (string(field.attribute("type").value()).compare("location_longitude") == 0) {
                        hasLongitude = true;
                        this->fieldLongitude = string(
                                field.attribute("name").value());
                    }
                } else { // if one of the values of name, type or indexed is empty
                    parseError << "For the searchable fields, "
                            << "providing values for'name', 'type' and 'indexed' is required\n ";
                    configSuccess = false;
                    return;
                }
            }

        }
    } else { // No searchable fields provided.
        parseError << "No searchable fields are provided.\n";
        configSuccess = false;
        return;
    }
    // Checking if there is any field or not.
    if (searchableFieldsVector.size() == 0) {
        parseError << "No searchable fields are provided.\n";
        configSuccess = false;
        return;
    }
    // XXX searchableAttributesVector -- I should take care of it.
    // XOR, if one of them is set and one isn't, that doesn't work. We should return false.
    if (hasLatitude ^ hasLongitude) {
        parseError << "One of the latitude or longitude is not set.\n";
        configSuccess = false;
        return;
    }
    // if hasLongitude and hasLatitude are set, the index type is going to be 1
    if (hasLatitude && hasLongitude) {
        this->indexType = 1;
        this->searchType = 2; // GEO search
    } else {
        this->indexType = 0;
        this->searchType = 0; // by default it is top-k
        // TODO: searchType should be set/
    }
    //  xxx: It should be removed: defaultSpatialQueryBoundingBox =  vm["default-spatial-query-bounding-square-side-length"].as<float>();

    // uniqueKey is required
    configAttribute = configDoc.child("schema").child("uniqueKey");
    if (configAttribute && configAttribute.text()) {
        this->primaryKey = string(configAttribute.text().get());
    } else {
        parseError << "uniqueKey (primary key) is not set.\n";
        configSuccess = false;
        return;
    }

    // we can remove it. Because we have all the attributes as searchable or not.
    this->isPrimSearchable = 0;
    for (int ind = 0; ind < searchableFieldsVector.size(); ind++) {
        if (searchableFieldsVector[ind].compare(this->primaryKey) == 0) {
            this->isPrimSearchable = searchableFieldIndexsVector[ind] ? 1 : 0; // isPrimeSearchable is integer.
        }
    }

    // Analyzer flags : default values.(everything is disable)
    this->stemmerFlag = false;
    this->stemmerFile = "";
    this->stopFilterFilePath = "";
    this->synonymFilterFilePath = "";
    this->synonymKeepOrigFlag = false;

    configAttribute = configDoc.child("schema").child("types");
    if (configAttribute) {        // Checks if <schema><types> exists or not
        for (xml_node fieldType = configAttribute.first_child(); fieldType;
                fieldType = fieldType.next_sibling()) { // Going on the children
            if ((string(fieldType.name()).compare("fieldType") == 0)) { // Finds the fieldTypes
                if (string(fieldType.attribute("name").value()).compare(
                        "text_en") == 0) {
                    // Checking if the values are empty or not
                    xml_node configAttributeTemp = fieldType.child("analyzer"); // looks for analyzer
                    for (xml_node field = configAttributeTemp.first_child();
                            field; field = field.next_sibling()) {
                        if (string(field.name()).compare("filter") == 0) {
                            if (string(field.attribute("name").value()).compare(
                                    "PorterStemFilter") == 0) { // STEMMER FILTER
                                if (string(field.attribute("dictionary").value()).compare("") != 0) { // the dictionary for porter stemmer is set.
                                    this->stemmerFlag = true;
                                    this->stemmerFile = string(field.attribute("dictionary").value());
                                    if (!(this->isPathFileValid(this->stemmerFile))) {
                                        this->stemmerFile = this->srch2Home + this->stemmerFile;
                                    }
                                }
                            } else if (string(field.attribute("name").value()).compare("StopFilter") == 0) { // STOP FILTER
                                if (string(field.attribute("words").value()).compare("") != 0) { // the words file for stop filter is set.
                                    this->stopFilterFilePath = string(field.attribute("words").value());
                                    if (!(this->isPathFileValid(this->stopFilterFilePath))) {
                                        this->stopFilterFilePath = this->srch2Home + this->stopFilterFilePath;
                                    }
                                }
                            } else if (string(field.attribute("name").value()).compare("SynonymFilter") == 0) {
                                if (string(field.attribute("synonyms").value()).compare("") != 0) { // the dictionary file for synonyms is set
                                    this->synonymFilterFilePath = string(field.attribute("synonyms").value());
                                    if (!(this->isPathFileValid(this->synonymFilterFilePath))) {
                                        this->synonymFilterFilePath = this->srch2Home + this->synonymFilterFilePath;
                                    }
                                    // checks the validity of boolean provided for 'expand'
                                    tempUse = string(field.attribute("expand").value());
                                    if (this->isValidBool(tempUse)) {
                                        this->synonymKeepOrigFlag = field.attribute("expand").as_bool();
                                    } else {
                                        parseError << "Config File Error: can not convert from '" << tempUse << "' to boolean\n";
                                        configSuccess = false;
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        parseError << "Analyzer Filters will be disabled.\n";
    }
    /*
     * Schema: End
     */

    /*
     * Config: Beginning
     */
    // licenseFile is a required field
    configAttribute = configDoc.child("config").child("licenseFile");
    if (configAttribute && configAttribute.text()) { // checks if config/licenseFile exists and have any text value or not
        this->licenseKeyFile = string(configAttribute.text().get());
        if(!(this->isPathFileValid(this->licenseKeyFile))) {
            this->licenseKeyFile = this->srch2Home + this->licenseKeyFile;
        }
    } else {
        parseError << "License key is not set.\n";
        configSuccess = false;
        return;
    }

    // listeningHostname is a required field
    configAttribute = configDoc.child("config").child("listeningHostname");
    if (configAttribute && configAttribute.text()) { // checks if config/listeningHostname exists and have any text value or not
        this->httpServerListeningHostname = string(configAttribute.text().get());
    } else {
        parseError << "listeningHostname is not set.\n";
        configSuccess = false;
        return;
    }

    // listeningPort is a required field
    configAttribute = configDoc.child("config").child("listeningPort");
    if (configAttribute && configAttribute.text()) { // checks if the config/listeningPort has any text in it or not
        this->httpServerListeningPort = string(configAttribute.text().get());
    } else {
        parseError << "listeningPort is not set.\n";
        configSuccess = false;
        return;
    }

    // dataDir is a required field
    configAttribute = configDoc.child("config").child("dataDir");
    if (configAttribute && configAttribute.text()) { // checks if the config/dataDir has any text in it or not
        this->indexPath = string(configAttribute.text().get());
        if(!(this->isPathFileValid(this->indexPath))) {
           this->indexPath = this->srch2Home + this->indexPath;
        }
    } else {
        parseError << "Path of index file is not set. You should set it as <dataDir>path/to/index/file</dataDir> in the config file.\n";
        configSuccess = false;
        return;
    }

    // dataFile is a required field
    configAttribute = configDoc.child("config").child("dataFile");
    if (configAttribute && configAttribute.text()) { // checks if the config/dataFile has any text in it or not
        this->filePath = string(configAttribute.text().get());
        if(!(this->isPathFileValid(this->filePath))) {
           this->filePath = this->srch2Home + this->filePath;
        }
    } else {
        parseError << "Path to the data file is not set. You should set it as <dataFile>path/to/data/file</dataFile> in the config file.\n";
        configSuccess = false;
        //TODO:  dataSourceType = FILEBOOTSTRAP_FALSE;
        return;
    }

    //fieldBoost
    configAttribute = configDoc.child("config").child("indexConfig").child("fieldBoost");
    map<string, unsigned> boostsMap;
    // spliting the field boost input and put them in boostsMap
    if (configAttribute && configAttribute.text()) {
        string boostString = string(configAttribute.text().get());
        boost::algorithm::trim(boostString);
        this->splitBoostFieldValues(boostString, boostsMap);
    }
    // filling the searchableAttributesTriple map
    for (int i = 0; i < searchableFieldsVector.size(); i++) {
        if (boostsMap.find(searchableFieldsVector[i]) == boostsMap.end()) {
            this->searchableAttributesTriple.insert(
                    make_pair(searchableFieldsVector[i], make_pair(0, 1)));
        } else {
            this->searchableAttributesTriple.insert(
                    make_pair(searchableFieldsVector[i], make_pair(0, boostsMap[searchableFieldsVector[i]])));
        }
    }
    // checks the validity of the values in boostsMap
    if (!this->isValidBoostFields(boostsMap)) {
        configSuccess = false;
        parseError << "Fields that are provided in the boostField do not match with the defined fields.";
        return;
    }
    // give each searchable attribute an id based on the order in the triple
    // should be consistent with the id in the schema
    unsigned idIter = 0;
    map<string, pair<unsigned, unsigned> >::iterator searchableAttributeIter = searchableAttributesTriple.begin();
    for ( ; searchableAttributeIter != searchableAttributesTriple.end(); searchableAttributeIter++) {
        searchableAttributeIter->second.first = idIter;
        idIter++;
    }




    // recordBoostField is an optional field
    this->recordBoostFieldFlag = false;
    configAttribute = configDoc.child("config").child("indexConfig").child("recordBoostField");
    if (configAttribute && configAttribute.text()) {
        this->recordBoostFieldFlag = true;
        this->recordBoostField = string(configAttribute.text().get());
    }

    // queryTermBoost is an optional field
    this->queryTermBoost = 1; // By default it is 1
    configAttribute = configDoc.child("config").child("indexConfig").child("defaultQueryTermBoost");
    if (configAttribute && configAttribute.text()) {
        string qtb = configAttribute.text().get();
        if (this->isValidQueryTermBoost(qtb)) {
            this->queryTermBoost = configAttribute.text().as_uint();
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermBoost is not a (non-negative)number.";
            return;
        }
    }

    // indexCreateOrLoad is an optional field
    this->indexCreateOrLoad = INDEXCREATE; // By default it is INDEXCREATE
    configAttribute = configDoc.child("config").child("indexConfig").child("indexLoadCreate");
    if (configAttribute && configAttribute.text()) {
        string ioc = configAttribute.text().get();
        if (this->isValidIndexCreateOrLoad(ioc)) {
            this->indexCreateOrLoad =
                    configAttribute.text().as_bool() == 0 ?
                            INDEXCREATE : INDEXLOAD;
        } else {
            configSuccess = false;
            parseError << "The value provided for indexCreateOrLoad is not a valid. It should be 0 or 1.";
            return;
        }
    }

    // scoringExpressionString is an optional field
    this->scoringExpressionString = "1"; // By default it is 1
    configAttribute = configDoc.child("config").child("query").child("rankingAlgorithm").child("recordScoreExpression");
    if (configAttribute && configAttribute.text()) {
        string exp = configAttribute.text().get();
        boost::algorithm::trim(exp);
        if (this->isValidRecordScoreExpession(exp)) {
            this->scoringExpressionString = exp;
        } else {
            configSuccess = false;
            parseError << "The expression provided for recordScoreExpression is not a valid.";
            return;
        }
    }

    // indexCreateOrLoad is an optional field
    this->queryTermSimilarityBoost = 0.5; // By default it is 0.5
    configAttribute = configDoc.child("config").child("query").child("queryTermSimilarityBoost");
    if (configAttribute && configAttribute.text()) {
        string qtsb = configAttribute.text().get();
        if (this->isValidQueryTermSimilarityBoost(qtsb)) {
            this->queryTermSimilarityBoost = configAttribute.text().as_float();
        } else {
            configSuccess = false;
            parseError
                    << "The expression provided for queryTermSimilarityBoost is not a valid.";
            return;
        }
    }

    // queryTermLengthBoost is an optional field
    this->queryTermLengthBoost = 0.5; // By default it is 0.5
    configAttribute = configDoc.child("config").child("query").child("queryTermLengthBoost");
    if (configAttribute && configAttribute.text()) {
        string qtlb = configAttribute.text().get();
        if (this->isValidQueryTermLengthBoost(qtlb)) {
            this->queryTermLengthBoost = configAttribute.text().as_float();
        } else {
            configSuccess = false;
            parseError
                    << "The expression provided for queryTermLengthBoost is not a valid.";
            return;
        }
    }

    // prefixMatchPenalty is an optional field.
    this->prefixMatchPenalty = 0.95; // By default it is 0.5
    configAttribute = configDoc.child("config").child("query").child("prefixMatchPenalty");
    if (configAttribute && configAttribute.text()) {
        string pm = configAttribute.text().get();

        if (this->isValidPrefixMatch(pm)) {
            this->prefixMatchPenalty = configAttribute.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The value provided for prefixMatch is not a valid.";
            return;
        }
    }

    // sortField is an optional field
    //TODO: the types should be removed.
    configAttribute = configDoc.child("config").child("query").child(
            "sortField");
    if (configAttribute && configAttribute.text()) {
        xml_node configAttributesortType = configDoc.child("config").child("query").child("sortFieldType");
        xml_node configAttributesortDefaultValue = configDoc.child("config").child("query").child(
                        "sortFieldDefaultValue");
        // If sort field is provided, sort type and sort default values should be provided too
        if (configAttributesortType && configAttributesortType.text()
                && configAttributesortDefaultValue
                && configAttributesortDefaultValue.text()) {

            vector<string> fieldNames;
            vector<srch2is::FilterType> fieldTypes;
            vector<string> fieldDefaultValues;

            this->splitString(string(configAttribute.text().get()), ",", fieldNames);
            this->splitString(string(configAttributesortDefaultValue.text().get()), ",", fieldDefaultValues);
            {
                vector<string> fieldTypesTemp;
                this->splitString(string(configAttributesortType.text().get()),
                        ",", fieldTypesTemp);
                if (this->isValidSortFieldType(fieldTypesTemp)) {
                    for (unsigned iter = 0; iter < fieldTypesTemp.size();
                            iter++) {
                        if (fieldTypesTemp[iter].compare("0") == 0) {
                            fieldTypes.push_back(srch2is::UNSIGNED);
                        } else if (fieldTypesTemp[iter].compare("1") == 0) {
                            fieldTypes.push_back(srch2is::FLOAT);
                        }
                    }
                } else {
                    parseError << "provided field-sort-types are not set correctly.\n";
                    configSuccess = false;
                    return;
                }
            }
            if (fieldNames.size() == fieldTypes.size()
                    && fieldNames.size() == fieldDefaultValues.size()) {
                for (unsigned iter = 0; iter < fieldNames.size(); iter++) {
                    this->sortableAttributes.push_back(fieldNames[iter]);
                    this->sortableAttributesType.push_back(fieldTypes[iter]);
                    this->sortableAttributesDefaultValue.push_back(
                            fieldDefaultValues[iter]);
                }
            } else {
                parseError << "field-sort related options were not set correctly.\n"
                        << " the number of field names and field types and field default values don't match\n";
                configSuccess = false;
                return;
            }
        } else {
            parseError << "Attributes-sort related options were not set correctly.\n";
            configSuccess = false;
            return;
        }

    }

    // validating the values
    if (!(this->isValidSortField(this->sortableAttributes)
            && this->isValidSortFieldDefaultValue(this->sortableAttributesDefaultValue))) {
        parseError << "Attributes-sort related options were not set correctly.\n";
        configSuccess = false;
        return;
    }

    // cacheSize is an optional field
    unsigned defaultCacheSize = 50 * 1048576; // 50MB
    this->cacheSizeInBytes = defaultCacheSize; // By default it is 0.5
    configAttribute = configDoc.child("config").child("query").child("cacheSize");
    if (configAttribute && configAttribute.text()) {
        string cs = configAttribute.text().get();
        if (this->isValidCacheSize(cs)) {
            this->cacheSizeInBytes = configAttribute.text().as_uint(defaultCacheSize);
        } else {
            parseError << "cache size provided is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // rows is an optional field
    this->resultsToRetrieve = 10; // by default it is 10
    configAttribute = configDoc.child("config").child("query").child("rows");
    if (configAttribute && configAttribute.text()) {
        string row = configAttribute.text().get();
        if (isValidRows(row)) {
            this->resultsToRetrieve = configAttribute.text().as_int();
        } else {
            parseError << "rows is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // maxSearchThreads is an optional field
    this->numberOfThreads = 1; // by default it is 10
    configAttribute = configDoc.child("config").child("query").child("maxSearchThreads");
    if (configAttribute && configAttribute.text()) {
        string mst = configAttribute.text().get();
        if (isValidMaxSearchThreads(mst)) {
            this->numberOfThreads = configAttribute.text().as_int();
        } else {
            parseError << "maxSearchThreads is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // fieldBasedSearch is an optional field
    this->supportAttributeBasedSearch = false; // by default it is 10
    configAttribute = configDoc.child("config").child("query").child("fieldBasedSearch");
    if (configAttribute && configAttribute.text()) {
        string fbs = configAttribute.text().get();
        if (this->isValidFieldBasedSearch(fbs)) {
            this->supportAttributeBasedSearch = configAttribute.text().as_bool();
        } else {
            parseError << "supportAttributeBasedSearch is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // queryTermMatchType is an optional field
    this->exactFuzzy = false; // by default it is 10
    configAttribute = configDoc.child("config").child("query").child("queryTermMatchType");
    if (configAttribute && configAttribute.text()) {
        string qtmt = configAttribute.text().get();
        if (this->isValidQueryTermMatchType(qtmt)) {
            this->exactFuzzy = configAttribute.text().as_bool();
        } else {
            parseError << "The queryTermMatchType that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    // queryTermType is an optional field
    this->queryTermType = false;
    configAttribute = configDoc.child("config").child("query").child("queryTermType");
    if (configAttribute && configAttribute.text()) {
        string qt = configAttribute.text().get();
        if (this->isValidQueryTermType(qt)) {
            this->queryTermType = configAttribute.text().as_bool();
        } else {
            parseError << "The queryTerm that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    // responseFormat is an optional field
    this->searchResponseJsonFormat = 0; // by default it is 10
    configAttribute = configDoc.child("config").child("query").child("queryResponseWriter").child("responseFormat");
    if (configAttribute && configAttribute.text()) {
        string rf = configAttribute.text().get();
        if (this->isValidResponseFormat(rf)) {
            this->searchResponseJsonFormat = configAttribute.text().as_int();
        } else {
            parseError << "The responseFormat provided is not valid";
            configSuccess = false;
            return;
        }
    }

    // responseContent is an optional field
    this->searchResponseFormat = 0; // by default it is 0
    configAttribute = configDoc.child("config").child("query").child("queryResponseWriter").child("responseContent");
    if (configAttribute) {
        string type = configAttribute.attribute("type").value();
        if (this->isValidResponseContentType(type)) {
            this->searchResponseJsonFormat = configAttribute.attribute("type").as_int();
        } else {
            parseError << "The type provided for responseContent is not valid";
            configSuccess = false;
            return;
        }

        if (this->searchResponseJsonFormat == 2) {
            if (configAttribute.text()) {
                this->splitString(string(configAttribute.text().get()), ",", this->attributesToReturn);
            } else {
                parseError << "For specified response content type, return fields should be provided.";
                configSuccess = false;
                return;
            }
        }

    }

//    //TODO: it should be removed.
//    // dataSourceType is an optional field
//    this->dataSourceType = FILEBOOTSTRAP_TRUE; // by default it is FILEBOOTSTRAP_TRUE
//    configAttribute = configDoc.child("config").child("query").child("dataSourceType");
//    if (configAttribute && configAttribute.text()) {
//        this->dataSourceType = configAttribute.text().as_int(FILEBOOTSTRAP_TRUE) ? FILEBOOTSTRAP_TRUE : FILEBOOTSTRAP_FALSE;
//        // if the value of dataSourceType is TRUE, the JSON data file should be provided.
//        // TODO: should we check it here again? because we want to remove this.
//    }
    this->dataSourceType = FILEBOOTSTRAP_TRUE; // Should it be as default

//    // TODO: it should be removed.
//    configAttribute = configDoc.child("config").child("query").child("writeApiType");
//    if (configAttribute && configAttribute.text()) {
//        this->writeApiType = configAttribute.text().as_bool(HTTPWRITEAPI) == 0 ? KAFKAWRITEAPI : HTTPWRITEAPI;
//        // if the value of dataSourceType is TRUE, the JSON data file should be provided.
//        // TODO: should we check it here again? because we want to remove this.
//        switch (writeApiType) {
//        case KAFKAWRITEAPI:
//            // TODO: kafka options are done?! W
//           //this->kafkaOptionsParse(vm, configSuccess, parseError);
//           break;
//        case HTTPWRITEAPI:
//           break;
//        }
//    } else {
//        parseError << "WriteAPI type Set. Default to HTTPWRITEAPI.\n";
//        writeApiType = HTTPWRITEAPI;
//    }
    this->writeApiType = HTTPWRITEAPI;

    configAttribute = configDoc.child("config").child("updatehandler").child("maxDocs");
    bool mdflag = false;
    if (configAttribute && configAttribute.text()) {
        string md = configAttribute.text().get();
        if (this->isValidMaxDoc(md)) {
            this->documentLimit = configAttribute.text().as_uint();
            mdflag = true;
        }
    }
    if (!mdflag) {
        parseError << "MaxDoc is not set correctly\n";
        configSuccess = false;
        return;
    }

    this->memoryLimit = 100000;
    bool mmflag = false;
    configAttribute = configDoc.child("config").child("updatehandler").child("maxMemory");
    if (configAttribute && configAttribute.text()) {
        string mm = configAttribute.text().get();
        if (this->isValidMaxMemory(mm)) {
            this->memoryLimit = configAttribute.text().as_uint();
            mmflag = true;
        }
    }
    if (!mmflag) {
        parseError << "MaxDoc is not set correctly\n";
        configSuccess = false;
        return;
    }

    // mergeEveryNSeconds
    configAttribute = configDoc.child("config").child("updatehandler").child("mergePolicy").child("mergeEveryNSeconds");
    bool mensflag = false;
    if (configAttribute && configAttribute.text()) {
        string mens = configAttribute.text().get();
        if (this->isValidMergeEveryNSeconds(mens)) {
            this->mergeEveryNSeconds = configAttribute.text().as_uint();
            mensflag = true;
        }
    }
    if (!mensflag) {
        parseError << "mergeEveryNSeconds is not set.\n";
        configSuccess = false;
        return;
    }

    // mergeEveryMWrites
    configAttribute = configDoc.child("config").child("updatehandler").child("mergePolicy").child("mergeEveryMWrites");
    bool memwflag = false;
    if (configAttribute && configAttribute.text()) {
        string memw = configAttribute.text().get();

        if (this->isValidMergeEveryMWrites(memw)) {
            this->mergeEveryMWrites = configAttribute.text().as_uint();
            memwflag = true;
        }
    }
    if (!memwflag) {
        parseError << "mergeEveryMWrites is not set.\n";
        configSuccess = false;
        return;
    }

    // logLevel is optional or required? XXX
    configAttribute = configDoc.child("config").child("updatehandler").child("updateLog").child("logLevel");
    bool llflag = false;
    if (configAttribute && configAttribute.text()) {
        string ll = configAttribute.text().get();
        if (this->isValidLogLevel(ll)) {
            this->loglevel =
                    static_cast<Logger::LogLevel>(configAttribute.text().as_int());
            llflag = true;
        }
    }
    if (!llflag) {
        parseError << "Log Level is not set correctly\n";
        configSuccess = false;
        return;
    }

    // accessLogFile is optional or required? XXX
    // TODO: do we need it? or we should remove it
    configAttribute = configDoc.child("config").child("updatehandler").child("updateLog").child("accessLogFile");
    if (configAttribute && configAttribute.text()) {
        // TODO: What is the defualt value for it?!
        this->httpServerAccessLogFile = string(configAttribute.text().get());
        if(!(this->isPathFileValid(this->httpServerAccessLogFile))) {
            this->httpServerAccessLogFile = this->srch2Home + this->httpServerAccessLogFile;
        }
    } else {
        parseError << "httpServerAccessLogFile is not set.\n";
        configSuccess = false;
        return;
    }

    // errorLogFile is optional or required? XXX
    configAttribute = configDoc.child("config").child("updatehandler").child("updateLog").child("errorLogFile");
    if (configAttribute && configAttribute.text()) {
        this->httpServerErrorLogFile = string(configAttribute.text().get());
        struct stat stResult;
        if(!(this->isPathFileValid(this->httpServerErrorLogFile))) {
            this->httpServerErrorLogFile = this->srch2Home + this->httpServerErrorLogFile;
        }
    } else {
        parseError << "httpServerErrorLogFile is not set.\n";
        configSuccess = false;
        return;
    }

    /*
     * query: END
     */


    if (this->supportAttributeBasedSearch && this->searchableAttributesTriple.size() > 31) {
        parseError << "To support attribute-based search, the number of searchable attributes cannot be bigger than 31.\n";
        configSuccess = false;
        return;
    }


    this->allowedRecordTokenizerCharacters = "";
    this->ordering = 0;
    this->trieBootstrapDictFile = "";
    this->attributeToSort = 0;

//
//    if (vm.count("trie-bootstrap-dict-file")) {
//        trieBootstrapDictFile = vm["trie-bootstrap-dict-file"].as<string>();
//    } else {
//        trieBootstrapDictFile = std::string("");
//    }
//
//    if (vm.count("allowed-record-special-characters")) {
//        allowedRecordTokenizerCharacters =
//                vm["allowed-record-special-characters"].as<string>();
//        if (allowedRecordTokenizerCharacters.empty())
//            std::cerr
//                    << "[Warning] There are no characters in the value allowedRecordTokenizerCharacters. To set it properly, those characters should be included in double quotes such as \"@'\""
//                    << std::endl;
//    } else {
//        allowedRecordTokenizerCharacters = string("");
//        //parseError << "allowed-record-special-characters is not set.\n";
//    }
//
//    if (vm.count("default-attribute-to-sort")) {
//        attributeToSort = vm["default-attribute-to-sort"].as<int>();
//    } else {
//        attributeToSort = 0;
//        //parseError << "attributeToSort is not set.\n";
//    }
//
//    if (vm.count("sortOrder")) {
//        ordering = vm["sortOrder"].as<int>();
//    } else {
//        ordering = 0;
//        //parseError << "ordering is not set.\n";
//    }
//
//    if (not (this->writeReadBufferInBytes > 4194304
//            && this->writeReadBufferInBytes < 65536000)) {
//        this->writeReadBufferInBytes = 4194304;
//    }

}

void ConfigManager::_setDefaultSearchableAttributeBoosts(
        const vector<string> &searchableAttributesVector) {
    for (unsigned iter = 0; iter < searchableAttributesVector.size(); iter++) {
        searchableAttributesTriple[searchableAttributesVector[iter]] = pair<
                unsigned, unsigned>(iter, 1);
    }
}

ConfigManager::~ConfigManager() {

}

const std::string& ConfigManager::getCustomerName() const {
    return kafkaConsumerTopicName;
}

uint32_t ConfigManager::getDocumentLimit() const {
    return documentLimit;
}

uint64_t ConfigManager::getMemoryLimit() const {
    return memoryLimit;
}

uint32_t ConfigManager::getMergeEveryNSeconds() const {
    return mergeEveryNSeconds;
}

uint32_t ConfigManager::getMergeEveryMWrites() const {
    return mergeEveryMWrites;
}

int ConfigManager::getIndexType() const {
    return indexType;
}

const string& ConfigManager::getAttributeLatitude() const {
    return fieldLatitude;
}

const string& ConfigManager::getAttributeLongitude() const {
    return fieldLongitude;
}

float ConfigManager::getDefaultSpatialQueryBoundingBox() const {
    return defaultSpatialQueryBoundingBox;
}

int ConfigManager::getNumberOfThreads() const {
    return numberOfThreads;
}

DataSourceType ConfigManager::getDataSourceType() const {
    return dataSourceType;
}

IndexCreateOrLoad ConfigManager::getIndexCreateOrLoad() const {
    return indexCreateOrLoad;
}

WriteApiType ConfigManager::getWriteApiType() const {
    return writeApiType;
}

const string& ConfigManager::getIndexPath() const {
    return indexPath;
}

const string& ConfigManager::getFilePath() const {
    return this->filePath;
}

const string& ConfigManager::getPrimaryKey() const {
    return primaryKey;
}

const map<string, pair<unsigned, unsigned> > * ConfigManager::getSearchableAttributes() const {
    return &searchableAttributesTriple;
}

const vector<string> * ConfigManager::getAttributesToReturnName() const {
    return &attributesToReturn;
}

const vector<string> * ConfigManager::getSortableAttributesName() const {
    return &sortableAttributes;
}

const vector<srch2::instantsearch::FilterType> * ConfigManager::getSortableAttributesType() const {
    return &sortableAttributesType;
}

const vector<string> * ConfigManager::getSortableAttributesDefaultValue() const {
    return &sortableAttributesDefaultValue;
}

/*const vector<unsigned> * Srch2ServerConf::getAttributesBoosts() const
 {
 return &attributesBoosts;
 }*/

string ConfigManager::getSrch2Home() const {
    return this->srch2Home;
}

bool ConfigManager::getStemmerFlag() const {
    return stemmerFlag;
}

string ConfigManager::getStemmerFile() const {
    return stemmerFile;
}

string ConfigManager::getSynonymFilePath() const {
    return synonymFilterFilePath;
}

bool ConfigManager::getSynonymKeepOrigFlag() const {
    return synonymKeepOrigFlag;
}

string ConfigManager::getStopFilePath() const {
    return stopFilterFilePath;
}

const string& ConfigManager::getAttributeRecordBoostName() const {
    return recordBoostField;
}

/*string getDefaultAttributeRecordBoost() const
 {
 return defaultAttributeRecordBoost;
 }*/

const std::string& ConfigManager::getScoringExpressionString() const {
    return scoringExpressionString;
}

int ConfigManager::getSearchResponseJSONFormat() const {
    return searchResponseJsonFormat;
}

const string& ConfigManager::getRecordAllowedSpecialCharacters() const {
    return allowedRecordTokenizerCharacters;
}

int ConfigManager::getSearchType() const {
    return searchType;
}

int ConfigManager::getIsPrimSearchable() const {
    return isPrimSearchable;
}

bool ConfigManager::getIsFuzzyTermsQuery() const {
    return exactFuzzy;
}

bool ConfigManager::getQueryTermType() const {
    return queryTermType;
}

unsigned ConfigManager::getQueryTermBoost() const {
    return queryTermBoost;
}

float ConfigManager::getQueryTermSimilarityBoost() const {
    return queryTermSimilarityBoost;
}

float ConfigManager::getQueryTermLengthBoost() const {
    return queryTermLengthBoost;
}

float ConfigManager::getPrefixMatchPenalty() const {
    return prefixMatchPenalty;
}

bool ConfigManager::getSupportAttributeBasedSearch() const {
    return supportAttributeBasedSearch;
}

int ConfigManager::getSearchResponseFormat() const {
    return searchResponseFormat;
}

const string& ConfigManager::getAttributeStringForMySQLQuery() const {
    return attributeStringForMySQLQuery;
}

const string& ConfigManager::getLicenseKeyFileName() const {
    return licenseKeyFile;
}

const std::string& ConfigManager::getTrieBootstrapDictFileName() const {
    return this->trieBootstrapDictFile;
}

const string& ConfigManager::getHTTPServerListeningHostname() const {
    return httpServerListeningHostname;
}

const string& ConfigManager::getHTTPServerListeningPort() const {
    return httpServerListeningPort;
}

const string& ConfigManager::getKafkaBrokerHostName() const {
    return kafkaBrokerHostName;
}

uint16_t ConfigManager::getKafkaBrokerPort() const {
    return kafkaBrokerPort;
}

const string& ConfigManager::getKafkaConsumerTopicName() const {
    return kafkaConsumerTopicName;
}

uint32_t ConfigManager::getKafkaConsumerPartitionId() const {
    return kafkaConsumerPartitionId;
}

uint32_t ConfigManager::getWriteReadBufferInBytes() const {
    return writeReadBufferInBytes;
}

uint32_t ConfigManager::getPingKafkaBrokerEveryNSeconds() const {
    return pingKafkaBrokerEveryNSeconds;
}

int ConfigManager::getDefaultResultsToRetrieve() const {
    return resultsToRetrieve;
}

int ConfigManager::getAttributeToSort() const {
    return attributeToSort;
}

int ConfigManager::getOrdering() const {
    return ordering;
}

bool ConfigManager::isRecordBoostAttributeSet() const {
    return recordBoostFieldFlag;
}

const string& ConfigManager::getHTTPServerAccessLogFile() const {
    return httpServerAccessLogFile;
}

const Logger::LogLevel& ConfigManager::getHTTPServerLogLevel() const {
    return loglevel;
}

const string& ConfigManager::getHTTPServerErrorLogFile() const {
    return httpServerErrorLogFile;
}

unsigned ConfigManager::getCacheSizeInBytes() const {
    return cacheSizeInBytes;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////// Validate & Helper functions
///////////////////////////////////////////////////////////////////////////////////////////////

// splitString gets a string as its input and a dlimiter. It slplits the string based on the delimiter and pushes back the values to the result
void ConfigManager::splitString(string str, const string& delimiter,
        vector<string>& result) {
    size_t pos = 0;
    string token;
    while ((pos = str.find(delimiter)) != string::npos) {
        result.push_back(str.substr(0, pos));
        str.erase(0, pos + delimiter.length());
    }
    result.push_back(str);
}

void ConfigManager::splitBoostFieldValues(string boostString, map<string, unsigned>& boosts) {
    vector<string> boostTokens;
    this->splitString(boostString, " ", boostTokens);
    for (int i = 0; i < boostTokens.size(); i++) {
        size_t pos = boostTokens[i].find("^");
        if (pos != string::npos) {
            string field = boostTokens[i].substr(0, pos);
            string boost = boostTokens[i].substr(pos + 1,
                    boostTokens[i].length());
            boosts[field] = (unsigned) atoi(boost.c_str());
        } else {
            boosts[boostTokens[i]] = 1;
        }
    }
}

bool ConfigManager::isOnlyDigits(string& str) {
    string::const_iterator it = str.begin();
    while (it != str.end() && std::isdigit(*it)) {
        ++it;
    }
    return !str.empty() && it == str.end();
}

bool ConfigManager::isFloat(string str){
    std::size_t found = str.find(".");
    if (found != std::string::npos) {
        str.erase(found, 1);
        if (str.find(".") != string::npos){
            return false;
        }
    }
    return this->isOnlyDigits(str);
}

bool ConfigManager::isPathFileValid(string& filePath){
    struct stat stResult;
    return (stat(filePath.c_str(), &stResult) == 0);
}

bool ConfigManager::isValidFieldType(string& fieldType) {
    // supported types are: text, location_latitude, location_longitude
    if ((fieldType.compare("text") == 0)
            || (fieldType.compare("location_latitude") == 0)
            || (fieldType.compare("location_longitude") == 0)) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidBool(string& fieldType) {
    // supported types are: text, location_latitude, location_longitude
    if ((fieldType.compare("true") == 0) || (fieldType.compare("True") == 0)
            || (fieldType.compare("TRUE") == 0)
            || (fieldType.compare("false") == 0)
            || (fieldType.compare("False") == 0)
            || (fieldType.compare("FALSE") == 0)) {
        return true;
    }
    return false;
}

// validates if all the fields from boosts Fields are in the Triple or not.
bool ConfigManager::isValidBoostFields(map<string, unsigned>& boostFields) {
    map<string, unsigned>::iterator iter;
    for (iter = boostFields.begin(); iter != boostFields.end(); ++iter) {
        if (this->searchableAttributesTriple.count(iter->first) > 0) {
            continue;
        }
        return false;
    }
    return true;
}

bool ConfigManager::isValidQueryTermBoost(string& queryTermBoost) {
    return this->isFloat(queryTermBoost);
}

bool ConfigManager::isValidIndexCreateOrLoad(string& indexCreateLoad) {
    if (indexCreateLoad.compare("0") == 0
            || indexCreateLoad.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidRecordScoreExpession(string& recordScoreExpression) {
    // We should validate this too.
    return true;
}

bool ConfigManager::isValidQueryTermSimilarityBoost(string& queryTermSimilarityBoost) {
    return this->isFloat(queryTermSimilarityBoost);
}

bool ConfigManager::isValidQueryTermLengthBoost(string& queryTermLengthBoost) {
    if (this->isFloat(queryTermLengthBoost)) {
        float val = ::atof(queryTermLengthBoost.c_str());
        if (val >= 0 && val <= 1) {
            return true;
        }
    } else {
        return false;
    }
    return true;
}

bool ConfigManager::isValidPrefixMatch(string& prefixmatch) {
    if (this->isFloat(prefixmatch)) {
        float val = ::atof(prefixmatch.c_str());
        if (val >= 0 && val <= 1) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidSortField(vector<string>& sortField) {
    for (unsigned iter = 0; iter != sortField.size(); ++iter) {
        if (this->searchableAttributesTriple.count(sortField[iter]) > 0) {
            continue;
        }
        return false;
    }
    return true;
}

bool ConfigManager::isValidSortFieldType(vector<string>& sortFieldType) {
    for (unsigned iter = 0; iter != sortFieldType.size(); ++iter) {
        if (sortFieldType[iter].compare("0") == 0
                || sortFieldType[iter].compare("1") == 0) {
            continue;
        }
        return false;
    }
    return true;
}

bool ConfigManager::isValidSortFieldDefaultValue(
        vector<string>& sortFieldDefaultValue) {
    for (unsigned iter = 0; iter != sortFieldDefaultValue.size(); ++iter) {
        if (!this->isOnlyDigits(sortFieldDefaultValue[iter])) {
            return false;
        }
    }
    return true;
}

bool ConfigManager::isValidCacheSize(string& cacheSize) {
    unsigned minCacheSize = 6553600;     // 50MB  6,553,600< x < 65,536,000
    unsigned maxCacheSize = 65536000;    // 500MB
    if (this->isOnlyDigits(cacheSize)) {
        int cs = atoi(cacheSize.c_str());
        if (cs >= minCacheSize && cs <= maxCacheSize) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidRows(string& rows) {
    return (this->isOnlyDigits(rows) && (atoi(rows.c_str()) > 0)); // should be number and greater that 1
}

bool ConfigManager::isValidMaxSearchThreads(string& maxSearchThreads) {
    return (this->isOnlyDigits(maxSearchThreads)
            && (atoi(maxSearchThreads.c_str()) > 0)); // should be number and greater that 1
}

bool ConfigManager::isValidFieldBasedSearch(string& fieldBasedSearch) {
    if (fieldBasedSearch.compare("0") == 0
            || fieldBasedSearch.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidQueryTermMatchType(string& queryTermMatchType) {
    if (queryTermMatchType.compare("0") == 0
            || queryTermMatchType.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidQueryTermType(string& queryTermType) {
    if (queryTermType.compare("0") == 0 || queryTermType.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidResponseFormat(string& responseFormat) {
    if (responseFormat.compare("0") == 0 || responseFormat.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidResponseContentType(string responseContentType) {
    if (responseContentType.compare("0") == 0
            || responseContentType.compare("1") == 0
            || responseContentType.compare("2") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidMaxDoc(string& maxDoc) {
    return this->isOnlyDigits(maxDoc);
}

bool ConfigManager::isValidMaxMemory(string& maxMemory) {
    return this->isOnlyDigits(maxMemory);
}

bool ConfigManager::isValidMergeEveryNSeconds(string& mergeEveryNSeconds) {
    if (this->isOnlyDigits(mergeEveryNSeconds)) {
        if (atoi(mergeEveryNSeconds.c_str()) >= 10) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidMergeEveryMWrites(string& mergeEveryMWrites) {
    if (this->isOnlyDigits(mergeEveryMWrites)) {
        if (atoi(mergeEveryMWrites.c_str()) >= 10) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidLogLevel(string& logLevel) {
    if (logLevel.compare("0") == 0 || logLevel.compare("1") == 0
            || logLevel.compare("2") == 0 || logLevel.compare("3") == 0) {
        return true;
    }
    return false;
}

// JUST FOR Wrapper TEST
void ConfigManager::setFilePath(const string& dataFile){
    this->filePath = dataFile;
}

}
}
