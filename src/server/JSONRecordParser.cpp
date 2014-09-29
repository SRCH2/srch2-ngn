
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
#include "util/ParserUtility.h"
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

bool JSONRecordParser::_JSONValueObjectToRecord(srch2is::Record *record, const std::string &inputLine,
                        const Json::Value &root, 
                        const CoreInfo_t *indexDataContainerConf,
                        std::stringstream &error,
                        RecordSerializer& compactRecSerializer)
{
    if (not (root.type() == Json::objectValue))
    {
        error << "\nFailed to parse JSON - No primary key found.";
        return false;// Raise Error
    }

    //string primaryKeyName = StringToWString(indexDataContainerConf->getPrimaryKey());

    string primaryKeyName = indexDataContainerConf->getPrimaryKey();

    //string primaryKeyStringValue = root.get(primaryKeyName, "NULL").asString(); // CHENLI
    std::vector<string> stringValues;
    getJsonValueString(root, primaryKeyName, stringValues, "primary-key");

    if (stringValues.empty() ||
        (stringValues.at(0).compare("NULL") != 0 && stringValues.at(0).compare("") != 0 ))
    {
        string primaryKeyStringValue = stringValues.at(0);
    	// trim to avoid any mismatch due to leading and trailing white space
    	boost::algorithm::trim(primaryKeyStringValue);
        const std::string primaryKey = primaryKeyStringValue.c_str();
        record->setPrimaryKey(primaryKey);
        if (indexDataContainerConf->getIsPrimSearchable())
        {
            record->setSearchableAttributeValue(indexDataContainerConf->getPrimaryKey(),primaryKeyStringValue);
        }
    }
    else
    {
        error << "\nFailed to parse JSON - No primary key found.";
        return false;// Raise Error
    }

    for (map<string , SearchableAttributeInfoContainer>::const_iterator attributeIter
    		= indexDataContainerConf->getSearchableAttributes()->begin();
    		attributeIter != indexDataContainerConf->getSearchableAttributes()->end();++attributeIter)
    {
    	string attributeKeyName = attributeIter->first;

        vector<string> attributeStringValues;
        getJsonValueString(root, attributeKeyName, attributeStringValues, "attributes-search");

        if (!attributeStringValues.empty() &&
            std::find(attributeStringValues.begin() , attributeStringValues.end() , "NULL") == attributeStringValues.end())
        {
            if (attributeIter->second.isMultiValued) {
                record->setSearchableAttributeValues(attributeKeyName,attributeStringValues);
            } else {
                record->setSearchableAttributeValue(attributeKeyName,attributeStringValues[0]);
            }
        }else{ // error if required or set to default
            if(attributeIter->second.required){ // true means required
                // ERROR
                error << "\nRequired field has a null value.";
                return false;// Raise Error
            }else{
                // passing the default value from config file
                if(attributeStringValues.empty()){
                    attributeStringValues.push_back(attributeIter->second.defaultValue);
                }else{
                    std::replace(attributeStringValues.begin() , attributeStringValues.end() , (string)"NULL" , attributeIter->second.defaultValue);
                }
                if (attributeIter->second.isMultiValued) {
                    record->setSearchableAttributeValues(attributeKeyName,attributeStringValues);
                } else {
                    record->setSearchableAttributeValue(attributeKeyName,attributeStringValues[0]);
                }
            }
        }
    }


    for (map<string, RefiningAttributeInfoContainer >::const_iterator attributeIter =
    		indexDataContainerConf->getRefiningAttributes()->begin();
            attributeIter != indexDataContainerConf->getRefiningAttributes()->end();
            ++attributeIter)
    {

        string attributeKeyName = attributeIter->first;

        // if type is date/time, check the syntax
        if( attributeIter->second.attributeType == srch2is::ATTRIBUTE_TYPE_TIME){
            vector<string> attributeStringValues;
            getJsonValueDateAndTime(root, attributeKeyName, attributeStringValues,"refining-attributes");
            if(attributeStringValues.empty()){
                // ERROR
                error << "\nDATE/TIME field has non recognizable format.";
                return false;// Raise Error
            }else{
                if (std::find(attributeStringValues.begin() , attributeStringValues.end() , "NULL") != attributeStringValues.end() &&
                    attributeIter->second.required ){
                    // ERROR
                    error << "\nDATE/TIME field " << attributeKeyName << " is marked as required field but does not have any value in input JSON record";
                    return false;// Raise Error
                }
                if (std::find(attributeStringValues.begin() , attributeStringValues.end() , "NULL") != attributeStringValues.end()){
                	//first verify whether default value itself is valid or not
                	const string& defaultValue = attributeIter->second.defaultValue;
                	if(srch2is::DateAndTimeHandler::verifyDateTimeString(defaultValue , srch2is::DateTimeTypePointOfTime)
                	  || srch2is::DateAndTimeHandler::verifyDateTimeString(defaultValue , srch2is::DateTimeTypeDurationOfTime)) {
                		std::replace(attributeStringValues.begin() , attributeStringValues.end() , (string)"NULL" , defaultValue);
                	} else {
                		// ERROR
                		error << "\nDATE/TIME field " << attributeKeyName << " has empty value and the default specified in the config file is not a valid value.";
                		return false;// Raise Error
                	}
                }
                string attributeStringValue = "";
                for(vector<string>::iterator stringValueIter = attributeStringValues.begin() ; stringValueIter != attributeStringValues.end() ; ++stringValueIter){
                    if(stringValueIter != attributeStringValues.begin()){
                        attributeStringValue += MULTI_VAL_ATTR_DELIMITER;
                    }
                    attributeStringValue += *stringValueIter;
                }
                // set the default value
                record->setRefiningAttributeValue(attributeKeyName, attributeStringValue);
            }
        }else{

            vector<string> attributeStringValues;
            getJsonValueString(root, attributeKeyName, attributeStringValues, "refining-attributes");

            if (!attributeStringValues.empty() &&
                std::find(attributeStringValues.begin() , attributeStringValues.end() , "NULL") == attributeStringValues.end()
                && std::find(attributeStringValues.begin() , attributeStringValues.end() , "") == attributeStringValues.end())
            {
                string attributeStringValue = "";
                for(vector<string>::iterator stringValueIter = attributeStringValues.begin() ; stringValueIter != attributeStringValues.end() ; ++stringValueIter){
                    if(stringValueIter != attributeStringValues.begin()){
                        attributeStringValue += MULTI_VAL_ATTR_DELIMITER;
                    }
                    attributeStringValue += *stringValueIter;
                }
                std::string attributeStringValueLowercase = attributeStringValue;
                std::transform(attributeStringValueLowercase.begin(), attributeStringValueLowercase.end(), attributeStringValueLowercase.begin(), ::tolower);
                record->setRefiningAttributeValue(attributeKeyName, attributeStringValueLowercase);
            }else{
                if(attributeIter->second.required){
                    // ERROR
                    error << "\nRequired refining attribute is null.";
                    return false;// Raise Error
                }else{
                    if(attributeStringValues.empty()){
                        attributeStringValues.push_back("");
                    }
                    std::replace(attributeStringValues.begin() , attributeStringValues.end() , (string)"NULL" , attributeIter->second.defaultValue);
                    std::replace(attributeStringValues.begin() , attributeStringValues.end() , (string)"" , attributeIter->second.defaultValue);
                    // set the default value
                    string attributeStringValue = "";
                    for(vector<string>::iterator stringValueIter = attributeStringValues.begin() ; stringValueIter != attributeStringValues.end() ; ++stringValueIter){
                        if(stringValueIter != attributeStringValues.begin()){
                            attributeStringValue += MULTI_VAL_ATTR_DELIMITER;
                        }
                        attributeStringValue += *stringValueIter;
                    }
                    std::string attributeStringValueLowercase = attributeStringValue;
                    std::transform(attributeStringValueLowercase.begin(), attributeStringValueLowercase.end(), attributeStringValueLowercase.begin(), ::tolower);
                    record->setRefiningAttributeValue(attributeKeyName,attributeStringValueLowercase);
                }
            }
        }

    }

    // Creating in-memory compact representation below by using the Record object. Sanity check of input
    // data is done before creating the record object.
    // 1. storing variable length attributes
    string compressedInputLine;
    typedef map<string , unsigned>::const_iterator SearchableAttrIter;
    // Note: storageSchema is a schema for in-memory data and it differs from actual schema populated
    // from config file and kept in the index.
    // In storage schema: Var Length Attributes (including MultiValAtr) => Searchable Attributes
    //                    fixed Length Attrbutes (only float and int) => refining Attributes.
    const Schema& storageSchema = compactRecSerializer.getStorageSchema();
    for (SearchableAttrIter iter = storageSchema.getSearchableAttribute().begin();
    		iter != storageSchema.getSearchableAttribute().end(); ++iter)
    {
    	vector<string> attributeStringValues;
    	record->getSearchableAttributeValues(iter->first, attributeStringValues);
    	string singleString;
    	if (attributeStringValues.size() > 0) {
    		singleString = boost::algorithm::join(attributeStringValues, " $$ ");
    	} else {
    		record->getRefiningAttributeValue(iter->first, singleString);
    	}
    	snappy::Compress(singleString.c_str(), singleString.length(), &compressedInputLine);
    	compactRecSerializer.addSearchableAttribute(iter->first, compressedInputLine);
    }
    // 2. Now we need to store the Fixed attributes (int and float)
    typedef map<string , unsigned>::const_iterator  RefineAttrIter;
    for (RefineAttrIter iter = storageSchema.getRefiningAttributes()->begin();
    		iter != storageSchema.getRefiningAttributes()->end(); ++iter) {
    	string attributeStringValue;
    	record->getRefiningAttributeValue(iter->first, attributeStringValue);
    	srch2is::FilterType type = storageSchema.getTypeOfRefiningAttribute(iter->second);
		switch (type) {
		case srch2is::ATTRIBUTE_TYPE_UNSIGNED:
		{
			unsigned val = atoi(attributeStringValue.c_str());
			compactRecSerializer.addRefiningAttribute(iter->first, val);
			break;
		}
		case srch2is::ATTRIBUTE_TYPE_FLOAT:
		{
			float val = atof(attributeStringValue.c_str());
			compactRecSerializer.addRefiningAttribute(iter->first, val);
			break;
		}
		default:
		{
			// not possible
			break;
		}
		}
    }
    RecordSerializerBuffer compactBuffer = compactRecSerializer.serialize();
    record->setInMemoryData(compactBuffer.start, compactBuffer.length);
    compactRecSerializer.nextRecord();


    // Add recordBoost, setSortableAttribute and setScoreAttribute
    record->setRecordBoost(1); // default record boost: 1
    if (indexDataContainerConf->isRecordBoostAttributeSet() == true)
    {
        // use "score" as boost
        string attributeKeyName = indexDataContainerConf->getAttributeRecordBoostName();
        double recordBoost;
        getJsonValueDouble(root, attributeKeyName, recordBoost, "attribute-record-boost");

        // Change an invalid (negative) boost value to the default value 1.
        if (recordBoost > 0.0)
            record->setRecordBoost(recordBoost);
    }

    if (indexDataContainerConf->getIndexType() == 1)
    {
        string latitudeAttributeKeyName = indexDataContainerConf->getAttributeLatitude();
        string longitudeAttributeKeyName = indexDataContainerConf->getAttributeLongitude();
        //double recordLatitude = root.get(latitudeAttributeKeyName, "NULL" ).asDouble();
        double recordLatitude;
        getJsonValueDouble(root, latitudeAttributeKeyName, recordLatitude, "attribute-latitude");

        if(recordLatitude > 200.0 || recordLatitude < -200.0) {
            Logger::warn("bad x: %f, set to 40.0 for testing purposes.\n", recordLatitude);
            recordLatitude = 40.0;
        }
        //double recordLongitude = root.get(longitudeAttributeKeyName, "NULL" ).asDouble();
        double recordLongitude;
        getJsonValueDouble(root, longitudeAttributeKeyName, recordLongitude, "attribute-longitude");

        if(recordLongitude > 200.0 || recordLongitude < -200.0)
        {
            Logger::warn("bad y: %f, set to -120.0 for testing purposes.\n", recordLongitude);
            recordLongitude = -120.0;
        }
        record->setLocationAttributeValue(recordLatitude, recordLongitude);
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

    schema->setPrimaryKey(primaryKey); // integer, not searchable unless set as an attribute using setSearchableAttribute() function.

    if (indexDataContainerConf->getIsPrimSearchable())
    {
        schema->setSearchableAttribute(primaryKey); // searchable primaryKey
    }

    // Set SearchableAttributes
    // map<string, pair<bool, pair<string, pair<unsigned,pair<unsigned , bool> > > > >
    map<string, SearchableAttributeInfoContainer>::const_iterator searchableAttributeIter = indexDataContainerConf->getSearchableAttributes()->begin();
    for ( ; searchableAttributeIter != indexDataContainerConf->getSearchableAttributes()->end();
                    searchableAttributeIter++)
    {
        schema->setSearchableAttribute(searchableAttributeIter->first,
        		searchableAttributeIter->second.boost ,
        		searchableAttributeIter->second.isMultiValued,
        		searchableAttributeIter->second.highlight); // searchable text
    }


    // Set NonSearchableAttributes
    map<string, RefiningAttributeInfoContainer >::const_iterator
    	nonSearchableAttributeIter = indexDataContainerConf->getRefiningAttributes()->begin();

    for ( ; nonSearchableAttributeIter != indexDataContainerConf->getRefiningAttributes()->end(); ++nonSearchableAttributeIter)
    {

        schema->setRefiningAttribute(nonSearchableAttributeIter->first,
        		nonSearchableAttributeIter->second.attributeType,
        		nonSearchableAttributeIter->second.defaultValue,
        		nonSearchableAttributeIter->second.isMultiValued);
    }


    std::string scoringExpressionString = indexDataContainerConf->getScoringExpressionString();
    schema->setScoringExpression(scoringExpressionString);
    schema->setSupportSwapInEditDistance(indexDataContainerConf->getSupportSwapInEditDistance());

    return schema;
}

/*
 *  Create indexes using records from json file and return the total indexed records.
 */
unsigned DaemonDataSource::createNewIndexFromFile(srch2is::Indexer* indexer, Schema * storedAttrSchema,
		const CoreInfo_t *indexDataContainerConf, const string & filePath)
{
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
void convertValueToString(Json::Value value, vector< string > &stringValues){
    if (value.isString())
        stringValues.push_back(value.asString()) ;
    else if(value.isDouble())
        stringValues.push_back(convertToStr<double>(value.asDouble()));
    else if(value.isInt())
        stringValues.push_back(convertToStr<int>(value.asInt()));
    else if(value.isArray())
    {
        for(Json::Value::iterator iter = value.begin(); iter != value.end(); iter++)
        {
            convertValueToString(*iter, stringValues);
        }
    }else if (value.isObject()){
        // for certain data sources such as mongo db, the field value may be
        // JSON object ( e.g mongo db primary key "_id")
        // For JSON object, recursively concatenate all keys' value
        vector<string> keys = value.getMemberNames();
        for (int i= 0; i < keys.size(); ++i) {
            convertValueToString(value.get(keys[i], "NULL"), stringValues);
        }
    }
    else // if the type is not string, set it to the empty string
        stringValues.clear();
}

  // get the string from a json value based on a key value.  Check the type first before
  // calling "asString()" to deal with the case where the input data was not formatted
  // properly.
  // if the type is int or double we convert it to string
  // parameter configName is used to be included in error/warning messages to make them meaningful ...
void JSONRecordParser::getJsonValueString(const Json::Value &jsonValue,
		const std::string &key,
		std::vector<std::string> &stringValues,
		const string &configName)
{
    if(!jsonValue.isMember(key)) {
        stringValues.clear();
        Logger::warn("[Warning] Wrong value setting for %s. There is no such attribute %s.\n", configName.c_str(), key.c_str());
        Logger::warn("Please set it to IGNORE in the configure file if you don't need it.");
        return;
    }
    Json::Value value = jsonValue.get(key, "NULL");
    convertValueToString(value, stringValues);
}


// get the string from a json value based on a key value.
// check to see if it is proper date/time format.
void JSONRecordParser::getJsonValueDateAndTime(const Json::Value &jsonValue,
		const std::string &key,
		vector< std::string> &stringValues,
		const string &configName){
    if(!jsonValue.isMember(key)){

        stringValues.clear();
        Logger::warn("[Warning] Wrong value setting for %s. There is no such attribute %s.\n", configName.c_str(), key.c_str());
        Logger::warn("Please set it to IGNORE in the configure file if you don't need it.");
        return;
    }
    vector<string> temp;
    Json::Value value = jsonValue.get(key, "NULL");
    convertValueToString(value, temp);

    // now check to see if it has proper date/time format

    string stringValue = "";
    for(vector<string>::iterator valueToken = temp.begin() ; valueToken != temp.end() ; ++valueToken){
        boost::algorithm::trim(*valueToken);
        if(srch2is::DateAndTimeHandler::verifyDateTimeString(*valueToken , srch2is::DateTimeTypePointOfTime)
           || srch2is::DateAndTimeHandler::verifyDateTimeString(*valueToken , srch2is::DateTimeTypeDurationOfTime) ){
            stringValues.push_back(*valueToken);
        }else{
        	if (*valueToken == "") {
        		stringValues.push_back("NULL");
        	} else {
        		stringValues.clear();
        		return;
        	}
        }
    }
    return;

}

// get the double from a json value based on a key value.  Check the type first before
// calling "asDouble()" to deal with the case where the input data was not formatted
// properly.
// Written by CHENLI
void JSONRecordParser::getJsonValueDouble(const Json::Value &jsonValue, 
                                          const std::string &key,
                                          double &doubleValue,
                                          const string& configName)
{
    if(!jsonValue.isMember(key))
    {
        doubleValue = 0;
        Logger::warn("value setting for %s. There is no such attribute %s.", configName.c_str(), key.c_str());
        Logger::warn("Please set it to IGNORE in the configure file if you don't need it.\n");
        return;
    }
    Json::Value value = jsonValue.get(key, "NULL");
    if (value.isDouble())
        doubleValue = value.asDouble();
    else if (value.isInt())
        doubleValue = value.asInt();
    else if (value.isUInt())
        doubleValue = value.asUInt();
    else // if the type is not double not int, set it to 0
    {
        doubleValue = 0;
        Logger::warn("The attribute %s was set to be %s.", key.c_str(), value.asString().c_str());
        Logger::warn("It should be a float or integer.\n");
    }
}

}
}
