
// $Id: JSONRecordParser.cpp 3456 2013-06-14 02:11:13Z jiaying $

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

    for (map<string, pair<unsigned, unsigned> >::const_iterator attributeIter = indexDataContainerConf->getSearchableAttributes()->begin();
            attributeIter != indexDataContainerConf->getSearchableAttributes()->end();
            ++attributeIter)
    {
        string attributeKeyName = attributeIter->first;
        //string attributeStringValue = root.get(attributeKeyName, "NULL" ).asString();
        //Json::Value localValue = root.get(attributeKeyName, "NULL" );
        //string attributeStringValue = localValue.asString();
        string attributeStringValue;
        getJsonValueString(root, attributeKeyName, attributeStringValue, "attributes-search");

        if (attributeStringValue.compare("NULL") != 0)
        {
            record->setSearchableAttributeValue(attributeKeyName,attributeStringValue);
        }
    }

    for (vector<string>::const_iterator attributeIter = indexDataContainerConf->getSortableAttributesName()->begin();
            attributeIter != indexDataContainerConf->getSortableAttributesName()->end();
            ++attributeIter)
    {
        string attributeKeyName = *attributeIter;
        //string attributeStringValue = root.get(attributeKeyName, "NULL" ).asString();

        string attributeStringValue;
        getJsonValueString(root, attributeKeyName, attributeStringValue, "attributes-sort");
        if (attributeStringValue=="") // if the attribute is int or float, convert it to string
        {
            double attributeDoubleValue;
            getJsonValueDouble(root, attributeKeyName, attributeDoubleValue, "attributes-sort");
            stringstream s;
            s << attributeDoubleValue;
            attributeStringValue = s.str();
        }

        if (attributeStringValue.compare("NULL") != 0)
        {
            record->setSortableAttributeValue(attributeKeyName, attributeStringValue);
        }
    }

    // Add recordBoost, setSortableAttribute and setScoreAttribute
    record->setRecordBoost(1); // default record boost: 1
    if (indexDataContainerConf->isRecordBoostAttributeSet() == true)
    {
        // use "score" as boost
        string attributeKeyName = indexDataContainerConf->getAttributeRecordBoostName();
        //float recordBoost = (float)root.get(attributeKeyName, "NULL" ).asDouble();
        //float recordBoost;
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
    map<string, pair<unsigned, unsigned> >::const_iterator searchableAttributeIter = indexDataContainerConf->getSearchableAttributes()->begin();
    for ( ; searchableAttributeIter != indexDataContainerConf->getSearchableAttributes()->end();
                    searchableAttributeIter++)
    {
        schema->setSearchableAttribute(searchableAttributeIter->first, searchableAttributeIter->second.second); // searchable text
    }

    // Set SortableAttributes
    vector<string>::const_iterator sortableAttributeNameIter = indexDataContainerConf->getSortableAttributesName()->begin();
    vector<srch2::instantsearch::FilterType>::const_iterator sortableAttributeTypeIter = indexDataContainerConf->getSortableAttributesType()->begin();
    vector<string>::const_iterator sortableAttributeDefaultValueIter = indexDataContainerConf->getSortableAttributesDefaultValue()->begin();
    for ( ; sortableAttributeNameIter != indexDataContainerConf->getSortableAttributesName()->end();
            ++sortableAttributeNameIter, ++sortableAttributeTypeIter, ++sortableAttributeDefaultValueIter)
    {
        //schema->setSortableAttribute("score",srch2::instantsearch::FLOAT, "1");
        schema->setSortableAttribute(*sortableAttributeNameIter,*sortableAttributeTypeIter,*sortableAttributeDefaultValueIter); // sortable attribute
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

    while(getline(in, line)  && in.good() )
    {
        bool parseSuccess = false;

        std::stringstream error;
        parseSuccess = JSONRecordParser::populateRecordFromJSON(line, indexDataContainerConf, record, error);

        if(parseSuccess)
        {
            // Add the record to the index
            //indexer->addRecordBeforeCommit(record, 0);
            indexer->addRecord(record, 0);
        }
        else
        {
            //TODO: cout to logger
            error << "at line:" << lineCounter;
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
    // clean the line by covering up the indexing first XXX records output
    cout << "                                                       " << "\r";
    Logger::console("Indexed %d records.", lineCounter);

    in.close();

    // Step 4: Commit the index, after which no more records can
    // be added
    indexer->commit();

    Logger::console("Saving Index.....");
    indexer->save();
    Logger::console("Index saved.");
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
  // Written by CHENLI
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
    else // if the type is not double not int, set it to 0
    {
    	doubleValue = 0;
    	cout << "[Warning] The attribute <" << key << "> was set to be \"" << value.asString() << "\".  It should be a float or integer." << endl;
    }
  }

}
}
