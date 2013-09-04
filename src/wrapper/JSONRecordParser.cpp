
// $Id: JSONRecordParser.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <cstdlib>

#include "JSONRecordParser.h"
#include <instantsearch/GlobalCache.h>

#include "thirdparty/utf8/utf8.h"
#include "thirdparty/snappy-1.0.4/snappy.h"
#include "util/Logger.h"
#include "ParserUtility.h"
#include <instantsearch/Analyzer.h>
#include "AnalyzerFactory.h"
#include "DateAndTimeHandler.h"

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

    if (primaryKeyStringValue.compare("NULL") != 0)
    {
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


    for (map<string, pair<bool, pair<string, pair<unsigned,unsigned> > > >::const_iterator attributeIter
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
        	if(attributeIter->second.first){ // true means required
        		// ERROR
                error << "\nRequired field has a null value.";
                return false;// Raise Error
        	}else{
        		// passing the default value from config file
        		record->setSearchableAttributeValue(attributeKeyName,attributeIter->second.second.first);
        	}
        }
    }


    for (map<string, pair< srch2::instantsearch::FilterType, pair<string, bool> > >::const_iterator attributeIter = indexDataContainerConf->getNonSearchableAttributes()->begin();
            attributeIter != indexDataContainerConf->getNonSearchableAttributes()->end();
            ++attributeIter)
    {

        string attributeKeyName = attributeIter->first;

        // if type is date/time, check the syntax
        if( attributeIter->second.first == srch2is::ATTRIBUTE_TYPE_TIME){
        	string attributeStringValue;
        	getJsonValueDateAndTime(root, attributeKeyName, attributeStringValue,"non-searchable-attributes");
        	if(attributeStringValue==""){
        		// ERROR
                error << "\nDATE/TIME field has non recognizable format.";
                return false;// Raise Error
        	}else{
                if (attributeStringValue.compare("NULL") != 0){
                    record->setNonSearchableAttributeValue(attributeKeyName, attributeStringValue);
                }else{
                    if(attributeIter->second.second.second){
                        // ERROR
                        error << "\nRequired non-searchable attribute is null.";
                        return false;// Raise Error
                    }else{
                        // set the default value
                        record->setNonSearchableAttributeValue(attributeKeyName,attributeIter->second.second.first);
                    }
                }
        	}
        }else{

            string attributeStringValue;
            getJsonValueString(root, attributeKeyName, attributeStringValue, "non-searchable-attributes");
            if (attributeStringValue.compare("") == 0) // if the attribute is int or float, convert it to string
            {
                double attributeDoubleValue;
                getJsonValueDouble(root, attributeKeyName, attributeDoubleValue, "non-searchable-attributes");
                stringstream s;
                s << attributeDoubleValue;
                attributeStringValue = s.str();
            }

            if (attributeStringValue.compare("NULL") != 0)
            {
                record->setNonSearchableAttributeValue(attributeKeyName, attributeStringValue);
            }else{
                if(attributeIter->second.second.second){
                    // ERROR
                    error << "\nRequired non-searchable attribute is null.";
                    return false;// Raise Error
                }else{
                    // set the default value
                    record->setNonSearchableAttributeValue(attributeKeyName,attributeIter->second.second.first);
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
        JSONRecordParser::_JSONValueObjectToRecord(record, inputLine, root, indexDataContainerConf, error);
    }
    return true;
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

    if (indexDataContainerConf->getSupportAttributeBasedSearch())
    {
        positionIndexType = srch2::instantsearch::FIELDBITINDEX;
    }
    else
    {
        positionIndexType = srch2::instantsearch::NOPOSITIONINDEX;
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
    map<string, pair<bool, pair<string, pair<unsigned,unsigned> > > >::const_iterator searchableAttributeIter = indexDataContainerConf->getSearchableAttributes()->begin();
    for ( ; searchableAttributeIter != indexDataContainerConf->getSearchableAttributes()->end();
                    searchableAttributeIter++)
    {
        schema->setSearchableAttribute(searchableAttributeIter->first, searchableAttributeIter->second.second.second.second); // searchable text
    }


    // Set NonSearchableAttributes
    map<string, pair< srch2::instantsearch::FilterType, pair<string, bool> > >::const_iterator
    	nonSearchableAttributeIter = indexDataContainerConf->getNonSearchableAttributes()->begin();

    for ( ; nonSearchableAttributeIter != indexDataContainerConf->getNonSearchableAttributes()->end(); ++nonSearchableAttributeIter)
    {

        schema->setNonSearchableAttribute(nonSearchableAttributeIter->first, nonSearchableAttributeIter->second.first ,
        		nonSearchableAttributeIter->second.second.first);
    }


    std::string scoringExpressionString = indexDataContainerConf->getScoringExpressionString();
    schema->setScoringExpression(scoringExpressionString);

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
                indexer->addRecord(record, analyzer, 0);
            }
            else
            {
                Logger::error("at line: %d" , lineCounter);
                Logger::error("%s", error.str().c_str());
            }
            record->clear();
            int reportFreq = 10000;
            if (lineCounter % reportFreq == 0)
            {
              std::cout << "Indexing first " << lineCounter << " records" << "\r";
            }
            ++lineCounter;
        }
    }
    std::cout<<"                                                     \r";
    Logger::console("Indexed %d records.", lineCounter);

    in.close();

    // Step 4: Commit the index, after which no more records can
    // be added
    indexer->commit();

    Logger::console("Saving Index.....");
    indexer->save();
    Logger::console("Index saved.");
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
	    		convertValueToString(*iter, stringValue);
	    		stringValue += " ";
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
		const string &configName){
	if(!jsonValue.isMember(key)){
		stringValue = "";
		cout << "[Warning] Wrong value setting for " << configName << ". There is no such attribute <" << key << ">.\n Please set it to IGNORE in the configure file if you don't need it." << endl;
		return;
	}
	string temp;
	Json::Value value = jsonValue.get(key, "NULL");
	convertValueToString(value, temp);

	// now check to see if it has proper date/time format
	stringValue = "";
	stringValue += DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(temp);
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
