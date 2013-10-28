
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

using namespace snappy;

using namespace std;
namespace srch2is = srch2::instantsearch;

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
                        const ConfigManager *indexDataContainerConf,
                        std::stringstream &error)
{
    if (not (root.type() == Json::objectValue))
    {
        error << "\nFailed to parse JSON - No primary key found.";
        return false;// Raise Error
    }

    //string primaryKeyName = StringToWString(indexDataContainerConf->getPrimaryKey());

    string primaryKeyName = indexDataContainerConf->getPrimaryKey();

    //string primaryKeyStringValue = root.get(primaryKeyName, "NULL").asString(); // CHENLI
    string primaryKeyStringValue;
    getJsonValueString(root, primaryKeyName, primaryKeyStringValue, "primary-key");

    if (primaryKeyStringValue.compare("NULL") != 0 && primaryKeyStringValue.compare("") != 0)
    {
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

    if (indexDataContainerConf->getSearchResponseFormat() == 0)
    {
        string compressedInputLine;
        snappy::Compress(inputLine.c_str(), inputLine.size(), &compressedInputLine);
    
        record->setInMemoryData(compressedInputLine);
    }
    else if (indexDataContainerConf->getSearchResponseFormat() == 2)
    {
        Json::FastWriter local_writer;
        Json::Value local_root;

        const vector<std::string> *attributesToReturnName = indexDataContainerConf->getAttributesToReturnName();
        for (int i=0; i<attributesToReturnName->size(); i++)
        {
            local_root[attributesToReturnName->at(i)] = root[attributesToReturnName->at(i)];
        }

        const string local_inputLine = local_writer.write(local_root);

        string compressedInputLine;
        snappy::Compress(local_inputLine.c_str(), local_inputLine.size(), &compressedInputLine);
    
        record->setInMemoryData(compressedInputLine);
    }

    for (map<string , SearchableAttributeInfoContainer>::const_iterator attributeIter
    		= indexDataContainerConf->getSearchableAttributes()->begin();
    		attributeIter != indexDataContainerConf->getSearchableAttributes()->end();++attributeIter)
    {
    	string attributeKeyName = attributeIter->first;

        string attributeStringValue;
        getJsonValueString(root, attributeKeyName, attributeStringValue, "attributes-search");

        if (attributeStringValue.compare("NULL") != 0)
        {
            record->setSearchableAttributeValue(attributeKeyName,attributeStringValue);
        }else{ // error if required or set to default
        	if(attributeIter->second.required){ // true means required
        		// ERROR
                error << "\nRequired field has a null value.";
                return false;// Raise Error
        	}else{
        		// passing the default value from config file
        		record->setSearchableAttributeValue(attributeKeyName,attributeIter->second.defaultValue);
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
        if( attributeIter->second.type == srch2is::ATTRIBUTE_TYPE_TIME){
        	string attributeStringValue;
        	getJsonValueDateAndTime(root, attributeKeyName, attributeStringValue,"refining-attributes" , attributeIter->second.isMultiValued);
        	if(attributeStringValue==""){
        		// ERROR
                error << "\nDATE/TIME field has non recognizable format.";
                return false;// Raise Error
        	}else{
                if (attributeStringValue.compare("NULL") != 0){
                    record->setRefiningAttributeValue(attributeKeyName, attributeStringValue);
                }else{
                    if(attributeIter->second.required){
                        // ERROR
                        error << "\nRequired refining attribute is null.";
                        return false;// Raise Error
                    }else{
                        // set the default value
                        record->setRefiningAttributeValue(attributeKeyName,attributeIter->second.defaultValue);
                    }
                }
        	}
        }else{

            string attributeStringValue;
            getJsonValueString(root, attributeKeyName, attributeStringValue, "refining-attributes");

            if (attributeStringValue.compare("NULL") != 0 && attributeStringValue.compare("") != 0)
            {
				std::string attributeStringValueLowercase = attributeStringValue;
				std::transform(attributeStringValueLowercase.begin(), attributeStringValueLowercase.end(), attributeStringValueLowercase.begin(), ::tolower);
                record->setRefiningAttributeValue(attributeKeyName, attributeStringValueLowercase);
            }else{
                if(attributeIter->second.required){
                    // ERROR
                    error << "\nRequired refining attribute is null.";
                    return false;// Raise Error
                }else{
                    // set the default value
    				std::string attributeStringValueLowercase = attributeIter->second.defaultValue;
    				std::transform(attributeStringValueLowercase.begin(), attributeStringValueLowercase.end(), attributeStringValueLowercase.begin(), ::tolower);
                    record->setRefiningAttributeValue(attributeKeyName,attributeStringValueLowercase);
                }
            }
        }

    }

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

        if(recordLatitude > 200.0 || recordLatitude < -200.0)
        {
            cout << "bad x: " << recordLatitude << ", set to 40.0 for testing purpose" << endl;
            recordLatitude = 40.0;
        }
        //double recordLongitude = root.get(longitudeAttributeKeyName, "NULL" ).asDouble();
        double recordLongitude;
        getJsonValueDouble(root, longitudeAttributeKeyName, recordLongitude, "attribute-longitude");

        if(recordLongitude > 200.0 || recordLongitude < -200.0)
        {
            cout << "bad y: " << recordLongitude << ", set to -120.0 for testing purpose" << endl;
            recordLongitude = -120.0;
        }
        record->setLocationAttributeValue(recordLatitude, recordLongitude);
    }
    return true;
}

bool JSONRecordParser::populateRecordFromJSON( const string &inputLine, const ConfigManager *indexDataContainerConf, srch2is::Record *record, std::stringstream &error)
{
    string::const_iterator end_it = utf8::find_invalid(inputLine.begin(), inputLine.end());
    if (end_it != inputLine.end()) {
        error << "Invalid UTF-8 encoding detected.";
        return false;
    }

    // Parse example data
    Json::Value root;
    Json::Reader reader;

    //std::cout << "[" << inputLine << "]" << std::endl;

    bool parseSuccess = reader.parse(inputLine, root, false);

    if (!parseSuccess)
    {
        error << "\nFailed to parse JSON - " << reader.getFormatedErrorMessages();
        return false;
    }
    else
    {
    	parseSuccess = JSONRecordParser::_JSONValueObjectToRecord(record, inputLine, root, indexDataContainerConf, error);
    }
    return parseSuccess;
}

srch2is::Schema* JSONRecordParser::createAndPopulateSchema( const ConfigManager *indexDataContainerConf)
{
    srch2::instantsearch::IndexType indexType;
    srch2::instantsearch::PositionIndexType positionIndexType;

    if (indexDataContainerConf->getIndexType() == 0)
    {
        indexType = srch2is::DefaultIndex;
    }
    else
    {
        indexType = srch2is::LocationIndex;
    }

    // if position index is ebabled then attribute based search is also enabled
    // so check whether position index is enabled first
    if (indexDataContainerConf->isPositionIndexEnabled()){
    	positionIndexType = srch2::instantsearch::POSITION_INDEX_FULL ;
    }
    else if (indexDataContainerConf->getSupportAttributeBasedSearch())
    {
        positionIndexType = srch2::instantsearch::POSITION_INDEX_FIELDBIT;
    }
    else
    {
        positionIndexType = srch2::instantsearch::POSITION_INDEX_NONE;
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
        		searchableAttributeIter->second.isMultiValued ); // searchable text
    }


    // Set NonSearchableAttributes
    map<string, RefiningAttributeInfoContainer >::const_iterator
    	nonSearchableAttributeIter = indexDataContainerConf->getRefiningAttributes()->begin();

    for ( ; nonSearchableAttributeIter != indexDataContainerConf->getRefiningAttributes()->end(); ++nonSearchableAttributeIter)
    {

        schema->setRefiningAttribute(nonSearchableAttributeIter->first,
        		nonSearchableAttributeIter->second.type,
        		nonSearchableAttributeIter->second.defaultValue,
        		nonSearchableAttributeIter->second.isMultiValued);
    }


    std::string scoringExpressionString = indexDataContainerConf->getScoringExpressionString();
    schema->setScoringExpression(scoringExpressionString);
    schema->setSupportSwapInEditDistance(indexDataContainerConf->getSupportSwapInEditDistance());

    return schema;
}

void DaemonDataSource::createNewIndexFromFile(srch2is::Indexer* indexer, const ConfigManager *indexDataContainerConf)
{
    string filePath = indexDataContainerConf->getFilePath();
    ifstream in(filePath.c_str());
    if (in.fail())
    {
        Logger::error("DataSource file not found at: %s", filePath.c_str());
        return;
    }

    string line;
    srch2is::Record *record = new srch2is::Record(indexer->getSchema());

    unsigned lineCounter = 0;
    unsigned indexedCounter = 0;
    // use same analyzer object for all the records
    srch2is::Analyzer *analyzer = AnalyzerFactory::createAnalyzer(indexDataContainerConf); 
    if(in.good()){
        while(getline(in, line))
        {
            bool parseSuccess = false;

            std::stringstream error;
            parseSuccess = JSONRecordParser::populateRecordFromJSON(line, indexDataContainerConf, record, error);

            if(parseSuccess)
            {
                // Add the record to the index
                //indexer->addRecordBeforeCommit(record, 0);
                indexer->addRecord(record, analyzer);
                indexedCounter++;
            }
            else
            {
                Logger::error("at line: %d" , lineCounter);
                Logger::error("%s", error.str().c_str());
            }
            record->clear();
            int reportFreq = 10000;
            ++lineCounter;
            if (lineCounter % reportFreq == 0)
            {
              std::cout << "Indexing first " << lineCounter << " records" << "\r";
            }
        }
    }
    std::cout<<"                                                     \r";
    Logger::console("Indexed %d / %d records.", indexedCounter, lineCounter);

    in.close();

    // Step 4: Commit the index, after which no more records can
    // be added
    indexer->commit();

    if (indexedCounter > 0) {
    	Logger::console("Saving Indexes.....");
    	indexer->save();
    	Logger::console("Indexes saved.");
    }
    delete analyzer;
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
void convertValueToString(Json::Value value, string &stringValue){
	if (value.isString())
	    	stringValue += value.asString();
	    else if(value.isDouble())
	    	stringValue += convertToStr<double>(value.asDouble());
	    else if(value.isInt())
	    	stringValue += convertToStr<int>(value.asInt());
	    else if(value.isArray())
	    {
	    	for(Json::Value::iterator iter = value.begin(); iter != value.end(); iter++)
	    	{
	    		if(iter != value.begin()){
					stringValue += srch2is::MULTI_VALUED_ATTRIBUTES_VALUE_DELIMITER;
	    		}
	    		convertValueToString(*iter, stringValue);
	    	}
	    }else if (value.isObject()){
	    	// for certain data sources such as mongo db, the field value may be
	    	// JSON object ( e.g mongo db primary key "_id")
	    	// For JSON object, recursively concatenate all keys' value
	    	vector<string> keys = value.getMemberNames();
	    	for (int i= 0; i < keys.size(); ++i) {
	    		if(i != 0){
					stringValue += srch2is::MULTI_VALUED_ATTRIBUTES_VALUE_DELIMITER;
	    		}
	    		convertValueToString(value.get(keys[i], "NULL"), stringValue);
	    	}
	    }
	    else // if the type is not string, set it to the empty string
	    	stringValue += "";
}

  // get the string from a json value based on a key value.  Check the type first before
  // calling "asString()" to deal with the case where the input data was not formatted
  // properly.
  // if the type is int or double we convert it to string
  // parameter configName is used to be included in error/warning messages to make them meaningful ...
void JSONRecordParser::getJsonValueString(const Json::Value &jsonValue,
		const std::string &key,
		std::string &stringValue,
		const string &configName)
{
	if(!jsonValue.isMember(key))
	{
		stringValue = "";
		cout << "[Warning] Wrong value setting for " << configName << ". There is no such attribute <" << key << ">.\n Please set it to IGNORE in the configure file if you don't need it." << endl;
		return;
	}
	Json::Value value = jsonValue.get(key, "NULL");
	convertValueToString(value, stringValue);
}


// get the string from a json value based on a key value.
// check to see if it is proper date/time format.
void JSONRecordParser::getJsonValueDateAndTime(const Json::Value &jsonValue,
		const std::string &key,
		std::string &stringValue,
		const string &configName, bool isMultiValued){
	if(!jsonValue.isMember(key)){
		stringValue = "";
		cout << "[Warning] Wrong value setting for " << configName << ". There is no such attribute <" << key << ">.\n Please set it to IGNORE in the configure file if you don't need it." << endl;
		return;
	}
	string temp;
	Json::Value value = jsonValue.get(key, "NULL");
	convertValueToString(value, temp);

	boost::algorithm::trim(temp);
	// now check to see if it has proper date/time format
	// if the value of the array was ["12:34:45","12:34:24","12:02:45"], it's now changed to
	// "12:34:45,12:34:24,12:02:45".
	// Now we should
	// 1. tokenize it
	// 2. convert it to unix time
	// 3. prepare the string again to become something like "1234245,3654665,56456687"
	vector<string> valueTokens;
	if(isMultiValued == false){
		valueTokens.push_back(temp);
	}else{
		boost::split(valueTokens , temp , boost::is_any_of(srch2is::MULTI_VALUED_ATTRIBUTES_VALUE_DELIMITER) , boost::token_compress_on );
	}

	stringValue = "";
	for(vector<string>::iterator valueToken = valueTokens.begin() ; valueToken != valueTokens.end() ; ++valueToken){
		if(srch2is::DateAndTimeHandler::verifyDateTimeString(*valueToken , srch2is::DateTimeTypePointOfTime)
				|| srch2is::DateAndTimeHandler::verifyDateTimeString(*valueToken , srch2is::DateTimeTypeDurationOfTime) ){
			stringstream buffer;
			buffer << srch2::instantsearch::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(*valueToken);
			if(valueToken == valueTokens.begin()){
				stringValue = buffer.str();
			}else{
				stringValue += srch2is::MULTI_VALUED_ATTRIBUTES_VALUE_DELIMITER+buffer.str();
			}

		}else{
			stringValue = "";
			return;
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
		cout << "[Warning] value setting for " << configName << ". There is no such attribute <" << key << ">.\n Please set it to IGNORE in the configure file if you don't need it." << endl;
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
    	cout << "[Warning] The attribute <" << key << "> was set to be \"" << value.asString() << "\".  It should be a float or integer." << endl;
    }
  }

}
}
