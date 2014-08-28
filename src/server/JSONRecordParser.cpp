
// $Id: JSONRecordParser.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include "JSONRecordParser.h"
#include <instantsearch/GlobalCache.h>

#include "thirdparty/utf8/utf8.h"
#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/Logger.h"
#include "ParserUtility.h"
#include <instantsearch/Analyzer.h>
#include "AnalyzerFactory.h"
#include "util/DateAndTimeHandler.h"

#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "util/RecordSerializerUtil.h"

using namespace snappy;

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace srch2::util;

namespace srch2
{
namespace httpwrapper
{

// string,wstring for conversion functions
std::wstring StringToWString(const std::string& s)
{
    std::wstring temp(s.length(),L' ');
    std::copy(s.begin(), s.end(), temp.begin());
    return temp;
}

std::string WStringToString(const std::wstring& s)
{
    std::string temp(s.length(), ' ');
    std::copy(s.begin(), s.end(), temp.begin());
    return temp;
}

bool JSONRecordParser::setRecordPrimaryKey(srch2is::Record *record,
        const Json::Value &root, const CoreInfo_t *indexDataContainerConf,
        std::stringstream &error) {

    string primaryKeyName = indexDataContainerConf->getPrimaryKey();

    std::vector<string> stringValues;

    getJsonValueString(root, primaryKeyName, stringValues, "primary-key");

    if (!stringValues.empty() && stringValues.at(0).compare("") != 0) {
        string primaryKeyStringValue = stringValues.at(0);
        // trim to avoid any mismatch due to leading and trailing white space
        boost::algorithm::trim(primaryKeyStringValue);
        const std::string primaryKey = primaryKeyStringValue.c_str();
        record->setPrimaryKey(primaryKey);
        if (indexDataContainerConf->getIsPrimSearchable()) {
            record->setSearchableAttributeValue(
                    indexDataContainerConf->getPrimaryKey(),
                    primaryKeyStringValue);
        }
    } else {
        error << "Failed to parse JSON - No primary key found.";
        return false; // Raise Error
    }

    return true;
}

bool JSONRecordParser::setRecordSearchableValue(srch2is::Record *record,
        const Json::Value &root, const CoreInfo_t *indexDataContainerConf,
        std::stringstream &error) {
    for (map<string, SearchableAttributeInfoContainer>::const_iterator attributeIter =
            indexDataContainerConf->getSearchableAttributes()->begin();
            attributeIter
                    != indexDataContainerConf->getSearchableAttributes()->end();
            ++attributeIter) {
        string attributeKeyName = attributeIter->first;

        vector<string> attributeStringValues;

        getJsonValueString(root, attributeKeyName, attributeStringValues,
                "attributes-search");

        if (!attributeStringValues.empty()
                && std::find(attributeStringValues.begin(),
                        attributeStringValues.end(), "")
                        == attributeStringValues.end()) {
            if (attributeIter->second.isMultiValued) {
                record->setSearchableAttributeValues(attributeKeyName,
                        attributeStringValues);
            } else {
                record->setSearchableAttributeValue(attributeKeyName,
                        attributeStringValues[0]);
            }
        } else { // error if required or set to default
            if (attributeIter->second.required) { // true means required
                // ERROR
                error << "\nRequired field has a null value.";
                return false;                    // Raise Error
            } else {
                // passing the default value from config file
                if (attributeStringValues.empty()) {
                    attributeStringValues.push_back(
                            attributeIter->second.defaultValue);
                } else {
                    std::replace(attributeStringValues.begin(),
                            attributeStringValues.end(), (string) "",
                            attributeIter->second.defaultValue);
                }
                if (attributeIter->second.isMultiValued) {
                    record->setSearchableAttributeValues(attributeKeyName,
                            attributeStringValues);
                } else {
                    record->setSearchableAttributeValue(attributeKeyName,
                            attributeStringValues[0]);
                }
            }
        }
    }
    return true;
}

bool JSONRecordParser::setRecordRefiningValue(srch2is::Record *record,
        const Json::Value &root, const CoreInfo_t *indexDataContainerConf,
        std::stringstream &error) {
    for (map<string, RefiningAttributeInfoContainer>::const_iterator attributeIter =
            indexDataContainerConf->getRefiningAttributes()->begin();
            attributeIter
                    != indexDataContainerConf->getRefiningAttributes()->end();
            ++attributeIter) {

        string attributeKeyName = attributeIter->first;

        // if type is date/time, check the syntax
        if (attributeIter->second.attributeType
                == srch2is::ATTRIBUTE_TYPE_TIME) {
            vector<string> attributeStringValues;

            getJsonValueDateAndTime(root, attributeKeyName,
                    attributeStringValues, "refining-attributes");

            if (attributeStringValues.empty()) {
                // ERROR
                error << "\nDATE/TIME field has non recognizable format.";
                return false;                    // Raise Error
            } else {
                if (std::find(attributeStringValues.begin(),
                        attributeStringValues.end(), "")
                        != attributeStringValues.end()) {
                    if (attributeIter->second.required) {
                        // ERROR
                        error << "\nDATE/TIME field " << attributeKeyName
                                << " is marked as required field but does not have any value in input JSON record";
                        return false;                    // Raise Error
                    }

                    //first verify whether default value itself is valid or not
                    const string& defaultValue =
                            attributeIter->second.defaultValue;
                    if (srch2is::DateAndTimeHandler::verifyDateTimeString(
                            defaultValue, srch2is::DateTimeTypePointOfTime)
                            || srch2is::DateAndTimeHandler::verifyDateTimeString(
                                    defaultValue,
                                    srch2is::DateTimeTypeDurationOfTime)) {
                        std::replace(attributeStringValues.begin(),
                                attributeStringValues.end(), (string) "",
                                defaultValue);
                    } else {
                        // ERROR
                        error << "\nDATE/TIME field " << attributeKeyName
                                << " has empty value and the default specified in the config file is not a valid value.";
                        return false;                    // Raise Error
                    }
                }
                string attributeStringValue = "";
                for (vector<string>::iterator stringValueIter =
                        attributeStringValues.begin();
                        stringValueIter != attributeStringValues.end();
                        ++stringValueIter) {
                    if (stringValueIter != attributeStringValues.begin()) {
                        attributeStringValue += MULTI_VAL_ATTR_DELIMITER;
                    }
                    attributeStringValue += *stringValueIter;
                }
                // set the default value
                record->setRefiningAttributeValue(attributeKeyName,
                        attributeStringValue);
            }
        } else {

            vector<string> attributeStringValues;

            getJsonValueString(root, attributeKeyName, attributeStringValues,
                    "refining-attributes");

            if (!attributeStringValues.empty()
                    && std::find(attributeStringValues.begin(),
                            attributeStringValues.end(), "")
                            == attributeStringValues.end()) {
                string attributeStringValue = "";
                for (vector<string>::iterator stringValueIter =
                        attributeStringValues.begin();
                        stringValueIter != attributeStringValues.end();
                        ++stringValueIter) {
                    if (stringValueIter != attributeStringValues.begin()) {
                        attributeStringValue += MULTI_VAL_ATTR_DELIMITER;
                    }
                    attributeStringValue += *stringValueIter;
                }
                std::string attributeStringValueLowercase = attributeStringValue;
                std::transform(attributeStringValueLowercase.begin(),
                        attributeStringValueLowercase.end(),
                        attributeStringValueLowercase.begin(), ::tolower);
                record->setRefiningAttributeValue(attributeKeyName,
                        attributeStringValueLowercase);
            } else {
                if (attributeIter->second.required) {
                    // ERROR
                    error << "\nRequired refining attribute is null.";
                    return false;                    // Raise Error
                } else {
                    if (attributeStringValues.empty()) {
                        attributeStringValues.push_back("");
                    }
                    std::replace(attributeStringValues.begin(),
                            attributeStringValues.end(), (string) "",
                            attributeIter->second.defaultValue);
                    // set the default value
                    string attributeStringValue = "";
                    for (vector<string>::iterator stringValueIter =
                            attributeStringValues.begin();
                            stringValueIter != attributeStringValues.end();
                            ++stringValueIter) {
                        if (stringValueIter != attributeStringValues.begin()) {
                            attributeStringValue += MULTI_VAL_ATTR_DELIMITER;
                        }
                        attributeStringValue += *stringValueIter;
                    }
                    std::string attributeStringValueLowercase =
                            attributeStringValue;
                    std::transform(attributeStringValueLowercase.begin(),
                            attributeStringValueLowercase.end(),
                            attributeStringValueLowercase.begin(), ::tolower);
                    record->setRefiningAttributeValue(attributeKeyName,
                            attributeStringValueLowercase);
                }
            }
        }
    }
    return true;
}

bool JSONRecordParser::setCompactRecordSearchableValue(
        const srch2is::Record *record, RecordSerializer& compactRecSerializer,
        std::stringstream &error) {
    string compressedInputLine;
    typedef map<string, unsigned>::const_iterator SearchableAttrIter;
    // Note: storageSchema is a schema for in-memory data and it differs from actual schema populated
    // from config file and kept in the index.
    // In storage schema: Var Length Attributes (including MultiValAtr) => Searchable Attributes
    //                    fixed Length Attrbutes (only float and int) => refining Attributes.
    const Schema& storageSchema = compactRecSerializer.getStorageSchema();
    for (SearchableAttrIter iter =
            storageSchema.getSearchableAttribute().begin();
            iter != storageSchema.getSearchableAttribute().end(); ++iter) {
        vector<string> attributeStringValues;
        record->getSearchableAttributeValues(iter->first,
                attributeStringValues);
        string singleString;
        if (attributeStringValues.size() > 0) {
            singleString = boost::algorithm::join(attributeStringValues,
                    " $$ ");
        } else {
            record->getRefiningAttributeValue(iter->first, singleString);
        }
        snappy::Compress(singleString.c_str(), singleString.length(),
                &compressedInputLine);
        compactRecSerializer.addSearchableAttribute(iter->first,
                compressedInputLine);
    }
    return true;
}

bool JSONRecordParser::setCompactRecordRefiningValue(
        const srch2is::Record *record, RecordSerializer& compactRecSerializer,
        std::stringstream &error) {
    typedef map<string, unsigned>::const_iterator RefineAttrIter;
    // Note: storageSchema is a schema for in-memory data and it differs from actual schema populated
    // from config file and kept in the index.
    // In storage schema: Var Length Attributes (including MultiValAtr) => Searchable Attributes
    //                    fixed Length Attrbutes (only float and int) => refining Attributes.
    const Schema& storageSchema = compactRecSerializer.getStorageSchema();
    for (RefineAttrIter iter = storageSchema.getRefiningAttributes()->begin();
            iter != storageSchema.getRefiningAttributes()->end(); ++iter) {
        string attributeStringValue;
        record->getRefiningAttributeValue(iter->first, attributeStringValue);
        srch2is::FilterType type = storageSchema.getTypeOfRefiningAttribute(
                iter->second);

        char * pEnd = NULL;
        switch (type) {
        case srch2is::ATTRIBUTE_TYPE_INT: {
            int val = static_cast<int>(strtol(attributeStringValue.c_str(),
                    &pEnd, 10));
            if (*pEnd != '\0') {
                error << ("\nInvalid value %s of type integer.",
                        attributeStringValue.c_str());
                return false;
            }
            compactRecSerializer.addRefiningAttribute(iter->first, val);
            break;
        }
        case srch2is::ATTRIBUTE_TYPE_LONG: {
            long val = strtol(attributeStringValue.c_str(),
                    &pEnd, 10);
            if (*pEnd != '\0') {
                error << ("\nInvalid value %s of type long.",
                        attributeStringValue.c_str());
                return false;
            }
            compactRecSerializer.addRefiningAttribute(iter->first, val);
            break;
        }
        case srch2is::ATTRIBUTE_TYPE_FLOAT: {
            float val = static_cast<float>(strtod(attributeStringValue.c_str(),
                    &pEnd));
            if (*pEnd != '\0') {
                error << ("\nInvalid value %s of type float.",
                        attributeStringValue.c_str());
                return false;
            }
            compactRecSerializer.addRefiningAttribute(iter->first, val);
            break;
        }
        case srch2is::ATTRIBUTE_TYPE_DOUBLE: {
            double val = strtod(attributeStringValue.c_str(),
                    &pEnd);
            if (*pEnd != '\0') {
                error << ("\nInvalid value %s of type double.",
                        attributeStringValue.c_str());
                return false;
            }
            compactRecSerializer.addRefiningAttribute(iter->first, val);
            break;
        }
        default: {
            error << ("\nRefining attribute that need to be compacted "
                    "in memory should be INT | LONG | FLOAT | DOUBLE,"
                    " no others are accepted.");
            return false;
            break;
        }
        }
    }
    return true;
}

bool JSONRecordParser::setRecordLocationValue(srch2is::Record *record,
        const Json::Value &root, const CoreInfo_t *indexDataContainerConf,std::stringstream &error) {
    if (indexDataContainerConf->getIndexType() == 1) {
        string latitudeAttributeKeyName =
                indexDataContainerConf->getAttributeLatitude();
        string longitudeAttributeKeyName =
                indexDataContainerConf->getAttributeLongitude();
        double recordLatitude;

        if(!getJsonValueDouble(root, latitudeAttributeKeyName, recordLatitude,
                "attribute-latitude"))
        	return false;

        if (recordLatitude > 200.0 || recordLatitude < -200.0) {
            Logger::warn("bad x: %f, set to 40.0 for testing purposes.\n",
                    recordLatitude);
        }
        double recordLongitude;

        if(!getJsonValueDouble(root, longitudeAttributeKeyName, recordLongitude,
                "attribute-longitude"))
        	return false;

        if (recordLongitude > 200.0 || recordLongitude < -200.0) {
            Logger::warn("bad y: %f, set to -120.0 for testing purposes.\n",
                    recordLongitude);
        }
        record->setLocationAttributeValue(recordLatitude, recordLongitude);
    }
    return true;
}

bool JSONRecordParser::setRecordBoostValue(srch2is::Record *record,
        const Json::Value &root, const CoreInfo_t *indexDataContainerConf) {
    record->setRecordBoost(1); // default record boost: 1
    if (indexDataContainerConf->isRecordBoostAttributeSet() == true) {
        // use "score" as boost
        string attributeKeyName =
                indexDataContainerConf->getAttributeRecordBoostName();
        double recordBoost;
        getJsonValueDouble(root, attributeKeyName, recordBoost,
                "attribute-record-boost");

        //Set the valid boost value. If the boost value is invalid (<=0), use
        //the default boost "1" set above.
        if (recordBoost > 0.0)
            record->setRecordBoost(recordBoost);
    }
    return true;
}

bool JSONRecordParser::_JSONValueObjectToRecord(srch2is::Record *record, const std::string &inputLine,
                        const Json::Value &root,
                        const CoreInfo_t *indexDataContainerConf,
                        std::stringstream &error,
                        RecordSerializer& compactRecSerializer)
{
    if (root.type() != Json::objectValue)
    {
        error << "\nFailed to parse JSON - No primary key found.";
        return false;// Raise Error
    }

    //Get primary key's value from the JSON object
    if (!setRecordPrimaryKey(record, root, indexDataContainerConf, error)) {
        return false;
    }

    // Get the searchable value from the JSON object and store them into the Record
    if (!setRecordSearchableValue(record, root, indexDataContainerConf,
            error)) {
        return false;
    }

    // Get the refining value from the JSON object and store them into the Record
    if (!setRecordRefiningValue(record, root, indexDataContainerConf, error)) {
        return false;
    }

    // Creating in-memory compact representation below by using the Record object. Sanity check of input
    // data is done before creating the record object.
    // 1. storing variable length attributes
    if(!setCompactRecordSearchableValue(record,compactRecSerializer,error)){
        return false;
    }
    // 2. Now we need to store the Fixed attributes (int and float)
    if(!setCompactRecordRefiningValue(record,compactRecSerializer,error)){
        return false;
    }

    RecordSerializerBuffer compactBuffer = compactRecSerializer.serialize();
    record->setInMemoryData(compactBuffer.start, compactBuffer.length);
    compactRecSerializer.nextRecord();

    // Add recordBoost, setSortableAttribute and setScoreAttribute
    if(!setRecordBoostValue(record, root, indexDataContainerConf)){
        return false;
    }
    // Set the location value if the index type is "1"
    if (!setRecordLocationValue(record, root, indexDataContainerConf,error)) {
        return false;
    }
    return true;
}

bool JSONRecordParser::populateRecordFromJSON(const string &inputLine,
        const CoreInfo_t *indexDataContainerConf, srch2is::Record *record, std::stringstream &error,
        RecordSerializer& compactRecSerializer)
{
    string::const_iterator end_it = utf8::find_invalid(inputLine.begin(), inputLine.end());
    if (end_it != inputLine.end()) {
        error << "Invalid UTF-8 encoding detected.";
        return false;
    }

    // Parse example data
    Json::Value root;
    Json::Reader reader;

    bool parseSuccess = reader.parse(inputLine, root, false);

    if (!parseSuccess)
    {
        error << "\nFailed to parse JSON - " << reader.getFormatedErrorMessages();
        return false;
    }
    else
    {
        parseSuccess = JSONRecordParser::_JSONValueObjectToRecord(record, inputLine, root,
                indexDataContainerConf, error, compactRecSerializer);
    }
    return parseSuccess;
}


srch2is::Schema* JSONRecordParser::createAndPopulateSchema(const CoreInfo_t *indexDataContainerConf)
{
    srch2::instantsearch::IndexType indexType;
    srch2::instantsearch::PositionIndexType positionIndexType = srch2::instantsearch::POSITION_INDEX_NONE;

    if (indexDataContainerConf->getIndexType() == 0)
    {
        indexType = srch2is::DefaultIndex;
    }
    else
    {
        indexType = srch2is::LocationIndex;
    }

    // if position word/offset index is enabled then attribute based search is also enabled
    // so check whether position index is enabled first
    if (indexDataContainerConf->isPositionIndexWordEnabled()){
        positionIndexType = srch2::instantsearch::POSITION_INDEX_WORD ;
    }
    if (indexDataContainerConf->isPositionIndexCharEnabled()) {
        if (positionIndexType == srch2::instantsearch::POSITION_INDEX_WORD)
            positionIndexType = srch2::instantsearch::POSITION_INDEX_FULL ;
        else
            positionIndexType = srch2::instantsearch::POSITION_INDEX_CHAR ;
    }
    if (positionIndexType == srch2::instantsearch::POSITION_INDEX_NONE &&
            indexDataContainerConf->getSupportAttributeBasedSearch())
    {
        positionIndexType = srch2::instantsearch::POSITION_INDEX_FIELDBIT;
    }

    srch2is::Schema* schema = srch2is::Schema::create(indexType, positionIndexType);

    // Set PrimaryKey
    string primaryKey = indexDataContainerConf->getPrimaryKey();

    schema->setPrimaryKey(primaryKey);

    if (indexDataContainerConf->getIsPrimSearchable())
    {
        schema->setSearchableAttribute(primaryKey); // searchable primaryKey
    }

    vector<unsigned> aclSearchableAttrIds;
    vector<unsigned> nonAclSearchableAttrIds;
    // Set SearchableAttributes
    // map<string, pair<bool, pair<string, pair<unsigned,pair<unsigned , bool> > > > >
    map<string, SearchableAttributeInfoContainer>::const_iterator searchableAttributeIter = indexDataContainerConf->getSearchableAttributes()->begin();
    for ( ; searchableAttributeIter != indexDataContainerConf->getSearchableAttributes()->end();
                    searchableAttributeIter++)
    {
        unsigned id = schema->setSearchableAttribute(searchableAttributeIter->first,
                searchableAttributeIter->second.boost ,
                searchableAttributeIter->second.isMultiValued,
                searchableAttributeIter->second.highlight); // searchable text
        if (searchableAttributeIter->second.isAclEnabled) {
        	aclSearchableAttrIds.push_back(id);
        } else {
        	nonAclSearchableAttrIds.push_back(id);
        }

    }
    schema->setAclSearchableAttrIdsList(aclSearchableAttrIds);
    schema->setNonAclSearchableAttrIdsList(nonAclSearchableAttrIds);

    vector<unsigned> aclRefiningAttrIds;
    vector<unsigned> nonAclRefiningAttrIds;
    // Set NonSearchableAttributes
    map<string, RefiningAttributeInfoContainer >::const_iterator
        nonSearchableAttributeIter = indexDataContainerConf->getRefiningAttributes()->begin();

    for ( ; nonSearchableAttributeIter != indexDataContainerConf->getRefiningAttributes()->end(); ++nonSearchableAttributeIter)
    {

    	unsigned id = schema->setRefiningAttribute(nonSearchableAttributeIter->first,
                nonSearchableAttributeIter->second.attributeType,
                nonSearchableAttributeIter->second.defaultValue,
                nonSearchableAttributeIter->second.isMultiValued);

        if (nonSearchableAttributeIter->second.isAclEnabled) {
        	aclRefiningAttrIds.push_back(id);
        } else {
        	nonAclRefiningAttrIds.push_back(id);
        }
    }
    schema->setAclRefiningAttrIdsList(aclRefiningAttrIds);
    schema->setNonAclRefiningAttrIdsList(nonAclRefiningAttrIds);

    if (aclSearchableAttrIds.size() > 0 && !isEnabledAttributeBasedSearch(positionIndexType)) {
    	//make sure to enable attributes position index if ACL on attributes is set.
    	positionIndexType = srch2::instantsearch::POSITION_INDEX_FIELDBIT;
    	schema->setPositionIndexType(positionIndexType);
    	const_cast<CoreInfo_t *>(indexDataContainerConf)->setSupportAttributeBasedSearch(true);
    }
    std::string scoringExpressionString = indexDataContainerConf->getScoringExpressionString();
    schema->setScoringExpression(scoringExpressionString);
    schema->setSupportSwapInEditDistance(indexDataContainerConf->getSupportSwapInEditDistance());

    schema->setNameOfLatitudeAttribute(indexDataContainerConf->getAttributeLatitude());
    schema->setNameOfLongitudeAttribute(indexDataContainerConf->getAttributeLongitude());

    return schema;
}

/*
 *  Create indexes using records from json file and return the total indexed records.
 */
unsigned DaemonDataSource::createNewIndexFromFile(srch2is::Indexer* indexer, Schema * storedAttrSchema,
        const CoreInfo_t *indexDataContainerConf)
{
    string filePath = indexDataContainerConf->getDataFilePath();
    ifstream in(filePath.c_str());
    if (in.fail())
    {
        Logger::error("DataSource file not found at: %s", filePath.c_str());
        return 0;
    }

    string line;
    srch2is::Record *record = new srch2is::Record(indexer->getSchema());

    unsigned lineCounter = 0;
    unsigned indexedRecordsCount = 0;
    // use same analyzer object for all the records
    srch2is::Analyzer *analyzer = AnalyzerFactory::createAnalyzer(indexDataContainerConf); 

    RecordSerializer compactRecSerializer = RecordSerializer(*storedAttrSchema);

    if(in.good()){
        bool isArrayOfJsonRecords = false;
        while(getline(in, line))
        {
            bool parseSuccess = false;

        // remove the trailing space or "," characters
        while (!line.empty() && (
                    line.at(line.length() - 1) == ' ' ||
                                    line.at(line.length() - 1) == ','
                                    )
                  ) {
                line.erase(line.length() - 1);
        }

            boost::trim(line);
            if (indexedRecordsCount == 0 &&  line == "[") {
                // Solr style data source - array of JSON records
                isArrayOfJsonRecords = true;
                continue;
            }
            if (isArrayOfJsonRecords == true && line == "]") {
                // end of JSON array in Solr style data source
                break; // assume nothing follows array (will ignore more records or another array)
            }

            std::stringstream error;
            parseSuccess = JSONRecordParser::populateRecordFromJSON(line, indexDataContainerConf, record, error, compactRecSerializer);

            if(parseSuccess)
            {
                // Add the record to the index
                //indexer->addRecordBeforeCommit(record, 0);
                indexer->addRecord(record, analyzer);
                indexedRecordsCount++;
            }
            else
            {
                Logger::error("at line: %d" , lineCounter);
                Logger::error("%s", error.str().c_str());
            }
            record->clear();
            int reportFreq = 10000;
            ++lineCounter;
            if (indexedRecordsCount % reportFreq == 0) {
                Logger::console("Indexing first %d records.\r", indexedRecordsCount);
            }
        }
    }
    Logger::console("                                                     \r");
    Logger::console("Indexed %d / %d records.", indexedRecordsCount, lineCounter);
    Logger::console("Finalizing ...");
    in.close();

    delete analyzer;
    delete record;
    return indexedRecordsCount;
}

// convert other types to string
template<class T>
string convertToStr(T value) {
    std::ostringstream o;
    if (!(o << value))
        return "";
    return o.str();
}

// convert a Json value to string
//If the value is null or empty, the vector<string> will only contain "".
void convertValueToString(Json::Value value, vector<string> &stringValues) {
    std::string lowercaseString, originalString;

    if (value.isString()) {
        originalString = value.asString();
        lowercaseString = originalString;
        std::transform(lowercaseString.begin(), lowercaseString.end(),
                lowercaseString.begin(), ::tolower);
        if (lowercaseString.compare("null") == 0 ) {
            stringValues.push_back("");
        } else {
            stringValues.push_back(originalString);
        }
    } else if (value.isDouble()) {
        originalString = convertToStr<double>(value.asDouble());
        lowercaseString = originalString;
        std::transform(lowercaseString.begin(), lowercaseString.end(),
                lowercaseString.begin(), ::tolower);
        if (lowercaseString.compare("null") == 0) {
            stringValues.push_back("");
        } else {
            stringValues.push_back(originalString);
        }
        /*
         * In JSONCPP 0.6.0, long value (>0) will be treated as UInt (unsigned int)
         * long value (<0) will be treated as Int.
         * int value will be treated as Int.
         *
         * All the cases above can be parsed from JSON object to long long int
         * by asInt64().
         */
    } else if (value.isInt()||value.isUInt()) {
        originalString = convertToStr<long>(value.asInt64());
        lowercaseString = originalString;
        std::transform(lowercaseString.begin(), lowercaseString.end(),
                lowercaseString.begin(), ::tolower);
        if (lowercaseString.compare("null") == 0) {
            stringValues.push_back("");
        } else {
            stringValues.push_back(originalString);
        }
    } else if (value.isArray()) {
        for (Json::Value::iterator iter = value.begin(); iter != value.end();
                iter++) {
            convertValueToString(*iter, stringValues);
        }
    } else if (value.isObject()) {
        // for certain data sources such as mongo db, the field value may be
        // JSON object ( e.g mongo db primary key "_id")
        // For JSON object, recursively concatenate all keys' value
        vector<string> keys = value.getMemberNames();
        for (int i = 0; i < keys.size(); ++i) {
            convertValueToString(value.get(keys[i], ""), stringValues);
        }
    } else { // if the type is not string, set it to the empty string
        stringValues.clear();
        stringValues.push_back("");
    }
}

// get the string from a json value based on a key value.  Check the type first before
// calling "asString()" to deal with the case where the input data was not formatted
// properly.
// if the type is int or double we convert it to string
// parameter configName is used to be included in error/warning messages to make them meaningful ...
bool JSONRecordParser::getJsonValueString(const Json::Value &jsonValue,
        const std::string &key, std::vector<std::string> &stringValues,
        const string &configName) {
    if (!jsonValue.isMember(key)) {
        //If the key does not exist in the JSON object, parse should be failed.
        //And the Record will not be inserted into the engine.
        stringValues.clear();
        Logger::warn(
                "[Warning] Wrong value setting for %s. There is no such attribute %s.\n",
                configName.c_str(), key.c_str());
        Logger::warn(
                "Please set it to IGNORE in the configure file if you don't need it.");
        return false;
    }
    Json::Value value = jsonValue.get(key, "");
    convertValueToString(value, stringValues);
    return true;
}


// get the string from a json value based on a key value.
// check to see if it is proper date/time format.
bool JSONRecordParser::getJsonValueDateAndTime(const Json::Value &jsonValue,
        const std::string &key, vector<std::string> &stringValues,
        const string &configName) {
    if (!jsonValue.isMember(key)) {
        //If the key does not exist in the JSON object, parse should be failed.
        //And the Record will not be inserted into the engine.
        stringValues.clear();
        Logger::warn(
                "[Warning] Wrong value setting for %s. There is no such attribute %s.\n",
                configName.c_str(), key.c_str());
        Logger::warn(
                "Please set it to IGNORE in the configure file if you don't need it.");
        return false;
    }
    vector<string> temp;
    Json::Value value = jsonValue.get(key, "");
    convertValueToString(value, temp);

    // now check to see if it has proper date/time format

    string stringValue = "";
    for (vector<string>::iterator valueToken = temp.begin();
            valueToken != temp.end(); ++valueToken) {
        boost::algorithm::trim(*valueToken);
        if (srch2is::DateAndTimeHandler::verifyDateTimeString(*valueToken,
                srch2is::DateTimeTypePointOfTime)
                || srch2is::DateAndTimeHandler::verifyDateTimeString(
                        *valueToken, srch2is::DateTimeTypeDurationOfTime)) {
            stringValues.push_back(*valueToken);
        } else {
            if (*valueToken == "") {
                stringValues.push_back("");
            } else {
                stringValues.clear();
                return false;
            }
        }
    }
    return true;

}

// get the double from a json value based on a key value.  Check the type first before
// calling "asDouble()" to deal with the case where the input data was not formatted
// properly.
// Written by CHENLI
bool JSONRecordParser::getJsonValueDouble(const Json::Value &jsonValue,
        const std::string &key, double &doubleValue, const string& configName) {
    if (!jsonValue.isMember(key)) {
        //If the key does not exist in the JSON object, parse should be failed.
        //And the Record will not be inserted into the engine.
        doubleValue = 0;
        Logger::warn("value setting for %s. There is no such attribute %s.",
                configName.c_str(), key.c_str());
        Logger::warn(
                "Please set it to IGNORE in the configure file if you don't need it.\n");
        return false;
    }
    Json::Value value = jsonValue.get(key, "");
    if (value.isDouble())
        doubleValue = value.asDouble();
    else if (value.isInt())
        doubleValue = value.asInt();
    else if (value.isUInt())
        doubleValue = value.asUInt();
    else // if the type is not double not int, set it to 0
    {
        doubleValue = 0;
        Logger::warn("The attribute %s was set to be %s.", key.c_str(),
                value.asString().c_str());
        Logger::warn("It should be a float or integer.\n");
    }
    return true;
}

}
}
