//$Id: ConfigManager.h 2013-07-5 02:11:13Z iman $

#include "ConfigManager.h"

#include <algorithm>
#include "util/xmlParser/pugixml.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/program_options.hpp>
#include <assert.h>
#include "util/Logger.h"
#include <boost/algorithm/string.hpp>
#include <sys/stat.h>

#include "util/DateAndTimeHandler.h"
#include "ParserUtility.h"
#include "util/Assert.h"

#include "boost/algorithm/string_regex.hpp"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace pugi;
// it is related to the pgixml.hpp which is a xml parser.

using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper { 

// configuration file tag and attribute names for ConfigManager
const char* const ConfigManager::accessLogFileString = "accesslogfile";
const char* const ConfigManager::analyzerString = "analyzer";
const char* const ConfigManager::cacheSizeString = "cachesize";
const char* const ConfigManager::collectionString = "collection";
const char* const ConfigManager::configString = "config";
const char* const ConfigManager::dataDirString = "datadir";
const char* const ConfigManager::dataFileString = "datafile";
const char* const ConfigManager::dataSourceTypeString = "datasourcetype";
const char* const ConfigManager::dbString = "db";
const char* const ConfigManager::defaultString = "default";
const char* const ConfigManager::defaultQueryTermBoostString = "defaultquerytermboost";
const char* const ConfigManager::dictionaryString = "dictionary";
const char* const ConfigManager::enablePositionIndexString = "enablepositionindex";
const char* const ConfigManager::expandString = "expand";
const char* const ConfigManager::facetEnabledString = "facetenabled";
const char* const ConfigManager::facetEndString = "facetend";
const char* const ConfigManager::facetFieldString = "facetfield";
const char* const ConfigManager::facetFieldsString = "facetfields";
const char* const ConfigManager::facetGapString = "facetgap";
const char* const ConfigManager::facetStartString = "facetstart";
const char* const ConfigManager::facetTypeString = "facettype";
const char* const ConfigManager::fieldString = "field";
const char* const ConfigManager::fieldBasedSearchString = "fieldbasedsearch";
const char* const ConfigManager::fieldBoostString = "fieldboost";
const char* const ConfigManager::fieldsString = "fields";
const char* const ConfigManager::fieldTypeString = "fieldtype";
const char* const ConfigManager::filterString = "filter";
const char* const ConfigManager::fuzzyMatchPenaltyString = "fuzzymatchpenalty";
const char* const ConfigManager::hostString = "host";
const char* const ConfigManager::indexConfigString = "indexconfig";
const char* const ConfigManager::indexedString = "indexed";
const char* const ConfigManager::multiValuedString = "multivalued";
const char* const ConfigManager::indexTypeString = "indextype";
const char* const ConfigManager::licenseFileString = "licensefile";
const char* const ConfigManager::listenerWaitTimeString = "listenerwaittime";
const char* const ConfigManager::listeningHostStringString = "listeninghostname";
const char* const ConfigManager::listeningPortString = "listeningport";
const char* const ConfigManager::locationLatitudeString = "location_latitude";
const char* const ConfigManager::locationLongitudeString = "location_longitude";
const char* const ConfigManager::logLevelString = "loglevel";
const char* const ConfigManager::maxDocsString = "maxdocs";
const char* const ConfigManager::maxMemoryString = "maxmemory";
const char* const ConfigManager::maxRetryOnFailureString = "maxretryonfailure";
const char* const ConfigManager::maxSearchThreadsString = "maxsearchthreads";
const char* const ConfigManager::mergeEveryMWritesString = "mergeeverymwrites";
const char* const ConfigManager::mergeEveryNSecondsString = "mergeeverynseconds";
const char* const ConfigManager::mergePolicyString = "mergepolicy";
const char* const ConfigManager::mongoDbString = "mongodb";
const char* const ConfigManager::nameString = "name";
const char* const ConfigManager::portString = "port";
const char* const ConfigManager::porterStemFilterString = "PorterStemFilter";
const char* const ConfigManager::prefixMatchPenaltyString = "prefixmatchpenalty";
const char* const ConfigManager::queryString = "query";
const char* const ConfigManager::queryResponseWriterString = "queryresponsewriter";
const char* const ConfigManager::queryTermLengthBoostString = "querytermlengthboost";
const char* const ConfigManager::queryTermFuzzyTypeString = "querytermfuzzytype";
const char* const ConfigManager::queryTermSimilarityThresholdString = "querytermsimilaritythreshold";
const char* const ConfigManager::queryTermPrefixTypeString = "querytermprefixtype";
const char* const ConfigManager::rankingAlgorithmString = "rankingalgorithm";
const char* const ConfigManager::recordBoostFieldString = "recordboostfield";
const char* const ConfigManager::recordScoreExpressionString = "recordscoreexpression";
const char* const ConfigManager::refiningString = "refining";
const char* const ConfigManager::requiredString = "required";
const char* const ConfigManager::responseContentString = "responsecontent";
const char* const ConfigManager::responseFormatString = "responseformat";
const char* const ConfigManager::rowsString = "rows";
const char* const ConfigManager::schemaString = "schema";
const char* const ConfigManager::searchableString = "searchable";
const char* const ConfigManager::searcherTypeString = "searchertype";
const char* const ConfigManager::srch2HomeString = "srch2home";
const char* const ConfigManager::stopFilterString = "StopFilter";
const char* const ConfigManager::protectedWordFilterString = "protectedKeyWordsFilter";
const char* const ConfigManager::supportSwapInEditDistanceString = "supportswapineditdistance";
const char* const ConfigManager::synonymFilterString = "SynonymFilter";
const char* const ConfigManager::synonymsString = "synonyms";
const char* const ConfigManager::textEnString = "text_en";
const char* const ConfigManager::typeString = "type";
const char* const ConfigManager::typesString = "types";
const char* const ConfigManager::uniqueKeyString = "uniquekey";
const char* const ConfigManager::updateHandlerString = "updatehandler";
const char* const ConfigManager::updateLogString = "updatelog";
const char* const ConfigManager::wordsString = "words";
const char* const ConfigManager::keywordPopularityThresholdString = "keywordpopularitythreshold";
const char* const ConfigManager::getAllResultsMaxResultsThreshold = "getallresultsmaxresultsthreshold";
const char* const ConfigManager::getAllResultsKAlternative = "getallresultskalternative";
const char* const ConfigManager::coresString = "cores";
const char* const ConfigManager::coreString = "core";
const char* const ConfigManager::defaultCoreNameString = "defaultCoreName";
const char* const ConfigManager::hostPortString = "hostPort";
const char* const ConfigManager::instanceDirString = "instanceDir";
const char* const ConfigManager::schemaFileString = "schemaFile";
const char* const ConfigManager::uLogDirString = "uLogDir";

ConfigManager::ConfigManager(const string& configFile)
{
    this->configFile = configFile;
    defaultCoreName = "__DEFAULT__";
}

void ConfigManager::loadConfigFile()
{
    Logger::debug("Reading config file: %s\n", this->configFile.c_str());
    xml_document configDoc;
    // Checks if the xml file is parsed correctly or not.
    pugi::xml_parse_result result = configDoc.load_file(this->configFile.c_str());
    // Add a comment to this line
    if (!result) {
		Logger::error("Parsing errors in XML configuration file '%s'", this->configFile.c_str());
		Logger::error("error: %s", result.description());
		exit(-1);
    }

    // make XML node names and attribute names lowercase so we are case insensitive
    lowerCaseNodeNames(configDoc);

    bool configSuccess = true;
    std::stringstream parseError;
    std::stringstream parseWarnings;
    // parse the config file and set the variables.
    this->parse(configDoc, configSuccess, parseError, parseWarnings);

    Logger::debug("WARNINGS while reading the configuration file:");
    Logger::debug("%s\n", parseWarnings.str().c_str());
    if (!configSuccess) {
        Logger::error("ERRORS while reading the configuration file");
        Logger::error("%s\n", parseError.str().c_str());
        cout << endl << parseError.str() << endl;
        exit(-1);
    }
}

class XmlLowerCaseWalker : public xml_tree_walker
{
public:
    virtual bool for_each(xml_node &node);
};

// iterator calls this method once for each node in tree
bool XmlLowerCaseWalker::for_each(xml_node &node)
{
    // lowercase the name of each node
    const char_t *oldName = node.name();

    if (oldName && oldName[0]) {
        unsigned int length = strlen(oldName);

	if (length > 0) {
	    // duplicate name, but in lowercase
	    char_t *newName = new char_t[length + 1];
	    for (unsigned int i = 0; i < length; i++)
	        newName[i] = tolower(oldName[i]);
	    newName[length] = '\000';

	    (void) node.set_name(newName); // discard return - no need to interrupt traversal
	    delete[] newName;
	}
    }

    // lowercase attribute names too
    for (pugi::xml_attribute_iterator attribute = node.attributes_begin(); attribute != node.attributes_end(); ++attribute) {
        oldName = attribute->name();
	unsigned int length = strlen(oldName);

	if (length > 0) {
	    // duplicate name, but in lowercase
	    char_t *newName = new char_t[length + 1];
	    for (unsigned int i = 0; i < length; i++)
	        newName[i] = tolower(oldName[i]);
	    newName[length] = '\000';

	    (void) attribute->set_name(newName);  // ignoring return value to avoid interrupting tree traversal
	    delete[] newName;
	}
    }

    return true;  // always allow tree traversal to continue uninterrupted
}

void ConfigManager::lowerCaseNodeNames(xml_node &document)
{
    XmlLowerCaseWalker nodeTraversal;

    document.traverse(nodeTraversal);
}

void ConfigManager::trimSpacesFromValue(string &fieldValue, const char *fieldName, std::stringstream &parseWarnings, const char *append)
{
    string oldValue(fieldValue);
    trim(fieldValue);
    if (fieldValue.length() != oldValue.length()) {
        parseWarnings << "Trimmed whitespace from the variable " << fieldName << "\"" << oldValue << "\"\n";
    }
    if (append != NULL) {
        fieldValue += append;
    }
}

CoreInfo_t *ConfigManager::getCoreSettings(const string &coreName) const
{
    if (coreName.compare("") != 0) {
        return ((CoreInfoMap_t) coreSettings)[coreName];
    }
    return getDefaultDataSource();
}

CoreInfo_t::CoreInfo_t(const CoreInfo_t &src)
{
    name = src.name;

    configManager = src.configManager;

    dataDir = src.dataDir;
    dataSourceType = src.dataSourceType;
    dataFile = src.dataFile;
    filePath = src.filePath;

    mongoHost = src.mongoHost;
    mongoPort = src.mongoPort;
    mongoDbName = src.mongoDbName;
    mongoCollection = src.mongoCollection;
    mongoListenerWaitTime = src.mongoListenerWaitTime;
    mongoListenerMaxRetryOnFailure = src.mongoListenerMaxRetryOnFailure;

    isPrimSearchable = src.isPrimSearchable;

    primaryKey = src.primaryKey;

    fieldLatitude = src.fieldLatitude;
    fieldLongitude = src.fieldLongitude;
    indexType = src.indexType;

    searchType = src.searchType;

    // TODO schema
}

void ConfigManager::parseIndexConfig(const xml_node &indexConfigNode, CoreInfo_t *settings, map<string, unsigned> &boostsMap, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    xml_node childNode = indexConfigNode.child(indexTypeString);
    if (childNode && childNode.text()) {
        string it = string(childNode.text().get());
        if (isValidIndexType(it)) {
            settings->indexType = childNode.text().as_int();
        } else {
            parseError << "Index Type's value can be only 0 or 1.\n";
            configSuccess = false;
            return;
        }
    } else {
        parseError << "Index Type is not set.\n";
        configSuccess = false;
        return;
    }

    settings->supportSwapInEditDistance = true; // by default it is true
    childNode = indexConfigNode.child(supportSwapInEditDistanceString);
    if (childNode && childNode.text()) {
        string qtmt = childNode.text().get();
        if (isValidBool(qtmt)) {
	    settings->supportSwapInEditDistance = childNode.text().as_bool();
        } else {
            parseError << "The provided supportSwapInEditDistance flag is not valid";
            configSuccess = false;
            return;
        }
    }

    settings->enablePositionIndex = false; // by default it is false
    childNode = indexConfigNode.child(enablePositionIndexString);
    if (childNode && childNode.text()) {
        string configValue = childNode.text().get();
        if (isValidBooleanValue(configValue)) {
            settings->enablePositionIndex = childNode.text().as_bool();
        } else {
            parseError << "enablePositionIndex should be either 0 or 1.\n";
            configSuccess = false;
            return;
        }
        Logger::info("turning on attribute based search because position index is enabled");
        settings->supportAttributeBasedSearch = true;
    }

    childNode = indexConfigNode.child(fieldBoostString);
    // splitting the field boost input and put them in boostsMap
    if (childNode && childNode.text()) {
        string boostString = string(childNode.text().get());
        boost::algorithm::trim(boostString);
        splitBoostFieldValues(boostString, boostsMap);
    }

    // recordBoostField is an optional field
    settings->recordBoostFieldFlag = false;
    childNode = indexConfigNode.child(recordBoostFieldString);
    if (childNode && childNode.text()) {
        settings->recordBoostFieldFlag = true;
        settings->recordBoostField = string(childNode.text().get());
    }

    // queryTermBoost is an optional field
    settings->queryTermBoost = 1; // By default it is 1
    childNode = indexConfigNode.child(defaultQueryTermBoostString);
    if (childNode && childNode.text()) {
        string qtb = childNode.text().get();
        if (isValidQueryTermBoost(qtb)) {
            settings->queryTermBoost = childNode.text().as_uint();
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermBoost is not a (non-negative)number.";
            return;
        }
    }
}

void ConfigManager::parseMongoDb(const xml_node &mongoDbNode, CoreInfo_t *settings, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    xml_node childNode = mongoDbNode.child(hostString);
    if (childNode && childNode.text()) {
        settings->mongoHost = string(childNode.text().get());
    } else {
        parseError << "mongo host is not set.\n";
	configSuccess = false;
	return;
    }

    childNode = mongoDbNode.child(portString);
    if (childNode && childNode.text()) {
        settings->mongoPort = string(childNode.text().get());
    } else {
        settings->mongoPort = ""; // use default port
    }

    childNode = mongoDbNode.child(dbString);
    if (childNode && childNode.text()) {
        settings->mongoDbName = string(childNode.text().get());
    } else {
        parseError << "mongo data base name is not set.\n";
	configSuccess = false;
	return;
    }

    childNode = mongoDbNode.child(collectionString);
    if (childNode && childNode.text()) {
        settings->mongoCollection = string(childNode.text().get());
    } else {
        parseError << "mongo collection name is not set.\n";
	configSuccess = false;
	return;
    }

    childNode = mongoDbNode.child(listenerWaitTimeString);
    if (childNode && childNode.text()) {
        settings->mongoListenerWaitTime = childNode.text().as_uint(1);
    } else {
        settings->mongoListenerWaitTime = 1;
    }

    childNode = mongoDbNode.child(maxRetryOnFailureString);
    if (childNode && childNode.text()) {
        settings->mongoListenerMaxRetryOnFailure = childNode.text().as_uint(3);
    } else {
        settings->mongoListenerMaxRetryOnFailure = 3;
    }

    // For MongoDB as a data source , primary key must be "_id" which is a unique key generated
    // by MongoDB. It is important to set primary key to "_id" because oplog entries for inserts
    // and deletes in MongoDB can be identified by _id only.
    settings->primaryKey = "_id";
}

void ConfigManager::parseQuery(const xml_node &queryNode,
			       CoreInfo_t *settings,
			       bool &configSuccess,
			       std::stringstream &parseError,
			       std::stringstream &parseWarnings)
{
    // scoringExpressionString is an optional field
    scoringExpressionString = "1"; // By default it is 1
    xml_node childNode = queryNode.child(rankingAlgorithmString).child(recordScoreExpressionString);
    if (childNode && childNode.text()) {
        string exp = childNode.text().get();
        boost::algorithm::trim(exp);
        if (isValidRecordScoreExpession(exp)) {
            scoringExpressionString = exp;
        } else {
            configSuccess = false;
            parseError << "The expression provided for recordScoreExpression is not a valid.";
            return;
        }
    }

    // fuzzyMatchPenalty is an optional field
    fuzzyMatchPenalty = 1; // By default it is 1
    childNode = queryNode.child(fuzzyMatchPenaltyString);
    if (childNode && childNode.text()) {
        string qtsb = childNode.text().get();
        if (isValidFuzzyMatchPenalty(qtsb)) {
            fuzzyMatchPenalty = childNode.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The expression provided for fuzzyMatchPenalty is not a valid.";
            return;
        }
    }

    // queryTermSimilarityThreshold is an optional field
    //By default it is 0.5.
    queryTermSimilarityThreshold = 0.5;
    childNode = queryNode.child(queryTermSimilarityThresholdString);
    if (childNode && childNode.text()) {
        string qtsb = childNode.text().get();
        if (isValidQueryTermSimilarityThreshold(qtsb)) {
            queryTermSimilarityThreshold = childNode.text().as_float();
            if (queryTermSimilarityThreshold < 0 || queryTermSimilarityThreshold > 1 ){
                queryTermSimilarityThreshold = 0.5;
                parseError << "The value provided for queryTermSimilarityThreshold is not in [0,1].";
            }
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermSimilarityThreshold is not a valid.";
            return;
        }
    }

    // queryTermLengthBoost is an optional field
    queryTermLengthBoost = 0.5; // By default it is 0.5
    childNode = queryNode.child(queryTermLengthBoostString);
    if (childNode && childNode.text()) {
        string qtlb = childNode.text().get();
        if (isValidQueryTermLengthBoost(qtlb)) {
            queryTermLengthBoost = childNode.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The expression provided for queryTermLengthBoost is not a valid.";
            return;
        }
    }

    // prefixMatchPenalty is an optional field.
    prefixMatchPenalty = 0.95; // By default it is 0.5
    childNode = queryNode.child(prefixMatchPenaltyString);
    if (childNode && childNode.text()) {
        string pm = childNode.text().get();

        if (isValidPrefixMatch(pm)) {
            prefixMatchPenalty = childNode.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The value provided for prefixMatch is not a valid.";
            return;
        }
    }

    // cacheSize is an optional field
    cacheSizeInBytes = 50 * 1048576;
    childNode = queryNode.child(cacheSizeString);
    if (childNode && childNode.text()) {
        string cs = childNode.text().get();
        if (isValidCacheSize(cs)) {
            cacheSizeInBytes = childNode.text().as_uint();
        } else {
            parseError << "cache size provided is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // rows is an optional field
    resultsToRetrieve = 10; // by default it is 10
    childNode = queryNode.child(rowsString);
    if (childNode && childNode.text()) {
        string row = childNode.text().get();
        if (isValidRows(row)) {
            resultsToRetrieve = childNode.text().as_int();
        } else {
            parseError << "rows is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // maxSearchThreads is an optional field
    numberOfThreads = 1; // by default it is 1
    childNode = queryNode.child(maxSearchThreadsString);
    if (childNode && childNode.text()) {
        string mst = childNode.text().get();
        if (isValidMaxSearchThreads(mst)) {
            numberOfThreads = childNode.text().as_int();
        } else {
            parseError << "maxSearchThreads is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // fieldBasedSearch is an optional field
    if (settings->enablePositionIndex == false) {
        settings->supportAttributeBasedSearch = false; // by default it is false
        childNode = queryNode.child(fieldBasedSearchString);
        if (childNode && childNode.text()) {
            string configValue = childNode.text().get();
            if (isValidBooleanValue(configValue)) {
                settings->supportAttributeBasedSearch = childNode.text().as_bool();
            } else {
                parseError << "fieldBasedSearch is not set correctly.\n";
                configSuccess = false;
                return;
            }
        }
    } else {
        // attribute based search is enabled if positional index is enabled
        settings->supportAttributeBasedSearch = true;
    }

    // queryTermFuzzyType is an optional field
    exactFuzzy = false; // by default it is false
    childNode = queryNode.child(queryTermFuzzyTypeString);
    if (childNode && childNode.text()) {
        string qtmt = childNode.text().get();
        if (isValidQueryTermFuzzyType(qtmt)) {
            exactFuzzy = childNode.text().as_bool();
        } else {
            parseError << "The queryTermFuzzyType that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    // queryTermPrefixType is an optional field
    queryTermPrefixType = false;
    childNode = queryNode.child(queryTermPrefixTypeString);
    if (childNode && childNode.text()) {
        string qt = childNode.text().get();
        if (isValidQueryTermPrefixType(qt)) {
            queryTermPrefixType = childNode.text().as_bool();
        } else {
            parseError << "The queryTerm that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    // responseFormat is an optional field
    searchResponseJsonFormat = 0; // by default it is 10
    childNode = queryNode.child(queryResponseWriterString).child(responseFormatString);
    if (childNode && childNode.text()) {
        string rf = childNode.text().get();
        if (isValidResponseFormat(rf)) {
            searchResponseJsonFormat = childNode.text().as_int();
        } else {
            parseError << "The provided responseFormat is not valid";
            configSuccess = false;
            return;
        }
    }

    // responseContent is an optional field
    searchResponseFormat = (ResponseType)0; // by default it is 0
    childNode = queryNode.child(queryResponseWriterString).child(responseContentString);
    if (childNode) {
        string type = childNode.attribute(typeString).value();
        if (isValidResponseContentType(type)) {
            searchResponseFormat = (ResponseType)childNode.attribute("type").as_int();
        } else {
            parseError << "The type provided for responseContent is not valid";
            configSuccess = false;
            return;
        }

        if (searchResponseFormat == 2) {
            if (childNode.text()) {
                splitString(string(childNode.text().get()), ",", attributesToReturn);
            } else {
                parseError << "For specified response content type, return fields should be provided.";
                configSuccess = false;
                return;
            }
        }
    }
}

// only called by parseCores()
void ConfigManager::parseCore(const xml_node &parentNode, CoreInfo_t *settings, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    string tempUse = "";

    // <core name="core0"
    if (parentNode.attribute(nameString) && string(parentNode.attribute(nameString).value()).compare("") != 0) {
        settings->name = parentNode.attribute(nameString).value();
    } else {
        parseError << "Core must have a name attribute";
	configSuccess = false;
	return;
    }

     // <core dataDir="core0/data"
    if (parentNode.attribute(dataDirString) && string(parentNode.attribute(dataDirString).value()).compare("") != 0) {
        settings->dataDir = parentNode.attribute(dataDirString).value();
	settings->indexPath = srch2Home + settings->dataDir;
    }

    parseDataSource(parentNode, settings, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }
}

// only called by parseDataConfiguration()
void ConfigManager::parseCores(const xml_node &coresNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    if (coresNode) {

        // <cores defaultCoreName = "foo">
        if (coresNode.attribute(defaultCoreNameString) && string(coresNode.attribute(defaultCoreNameString).value()).compare("") != 0) {
            defaultCoreName = coresNode.attribute(defaultCoreNameString).value();
	} else {
	    parseWarnings << "Cores defaultCoreName never set";
	}

	// parse zero or more individual core settings
        for (xml_node coreNode = coresNode.first_child(); coreNode; coreNode = coreNode.next_sibling()) {
	  CoreInfo_t *newSettings = new CoreInfo_t(this);

	    parseCore(coreNode, newSettings, configSuccess, parseError, parseWarnings);
	    if (configSuccess) {
	        coreSettings[newSettings->name] = newSettings;
	    } else {
	        delete newSettings;
	        return;
	    }
	}	
    }
}

/*
 * parentNode is either <config> or <core> - but only called by parseDataConfiguration()
 */
void ConfigManager::parseDataSource(const xml_node &parentNode, CoreInfo_t *settings, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    string tempUse = "";
    ParseState_t parseState;

    // <config><dataDir>core0/data OR <core><dataDir>
    if (parentNode.child(dataDirString) && parentNode.child(dataDirString).text()) {
        settings->dataDir = parentNode.child(dataDirString).value();
	settings->indexPath = srch2Home + settings->dataDir;
    }

    xml_node childNode = parentNode.child(dataSourceTypeString);
    if (childNode && childNode.text()) {
        int dataSourceValue = childNode.text().as_int(DATA_SOURCE_JSON_FILE);
	switch(dataSourceValue) {
	case 0:
	    settings->dataSourceType = DATA_SOURCE_NOT_SPECIFIED;
	    break;
	case 1:
	    settings->dataSourceType = DATA_SOURCE_JSON_FILE;
	    break;
	case 2:
	    settings->dataSourceType = DATA_SOURCE_MONGO_DB;
	    break;
	default:
	    // if user forgets to specify this option, we will assume data source is JSON file
	    settings->dataSourceType = DATA_SOURCE_JSON_FILE;
	    break;
	}
    } else {
        settings->dataSourceType = DATA_SOURCE_JSON_FILE;
    }

    if (settings->dataSourceType == DATA_SOURCE_JSON_FILE) {
        // dataFile is a required field only if JSON file is specified as data source.
        childNode = parentNode.child(dataFileString);
	if (childNode && childNode.text()) { // checks if the config/dataFile has any text in it or not
	    tempUse = string(childNode.text().get());
	    trimSpacesFromValue(tempUse, dataFileString, parseWarnings);
	    settings->filePath = this->srch2Home + tempUse;
	} else {
	    parseError << (settings->name.compare("") != 0 ? settings->name : "default") <<
	        " core path to the data file is not set. "
	        "You should set it as <dataFile>path/to/data/file</dataFile> in the config file.\n";
	  configSuccess = false;
	  return;
	}
    }

    if (settings->dataSourceType == DATA_SOURCE_MONGO_DB) {
        childNode = parentNode.child(mongoDbString);
	parseMongoDb(childNode, settings, configSuccess, parseError, parseWarnings);
	if (configSuccess == false) {
	    return;
	}
    }

    // uniqueKey is required
    childNode = parentNode.child(schemaString).child(uniqueKeyString);
    if (childNode && childNode.text()) {
        settings->primaryKey = string(childNode.text().get());
    } else {
        parseError << (settings->name.compare("") != 0 ? settings->name : "default") <<
	    " core uniqueKey (primary key) is not set.\n";
        configSuccess = false;
        return;
    }

    xml_node indexConfigNode = parentNode.child(indexConfigString);
    map<string, unsigned> boostsMap;
    parseIndexConfig(indexConfigNode, settings, boostsMap, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }

    // <schema>
    childNode = parentNode.child(schemaString);
    if (childNode) {
        parseSchema(childNode, &parseState, settings, configSuccess, parseError, parseWarnings);
	if (configSuccess == false) {
	    return;
	}
    }

    if (settings->indexType == 1) {
        // If index type is 1, it means it is geo. So both latitude and longitude should be provided
        if (!(parseState.hasLatitude && parseState.hasLongitude)) {
            parseError << "Both Geo related attributes should set together. Currently only one of them is set.\n";
            configSuccess = false;
            return;
        }
        settings->searchType = 2; // GEO search
    } else if (settings->indexType == 0){
        settings->searchType = 0;
        settings->fieldLongitude = "IGNORE"; // IN URL parser these fields are being checked with "IGNORE". We should get rid of them.
        settings->fieldLatitude = "IGNORE"; // IN URL parser these fields are being checked with "IGNORE". We should get rid of them.

        childNode = parentNode.child(queryString).child(searcherTypeString);
        if (childNode && childNode.text()) {
            string st = childNode.text().get();
            if (isValidSearcherType(st)) {
                settings->searchType = childNode.text().as_int();
            } else {
                parseError << "The Searcher Type only can get 0 or 1";
                configSuccess = false;
                return;
            }
        }
    }

    // <config>
    //   <indexconfig>
    //        <fieldBoost>
    // filling the searchableAttributesInfo map
    // this depends upon parseIndexConfig() to have loaded boostsMap and
    // searchableFieldsVector & company to have been loaded elsewhere from <schema>
    for (int i = 0; i < parseState.searchableFieldsVector.size(); i++) {
        if (boostsMap.find(parseState.searchableFieldsVector[i]) == boostsMap.end()) {
            settings->searchableAttributesInfo[parseState.searchableFieldsVector[i]] =
            		SearchableAttributeInfoContainer(parseState.searchableFieldsVector[i] ,
            				parseState.searchableAttributesRequiredFlagVector[i] ,
            				parseState.searchableAttributesDefaultVector[i] ,
            				0 , 1 , parseState.searchableAttributesIsMultiValued[i]);
        } else {
            settings->searchableAttributesInfo[parseState.searchableFieldsVector[i]] =
            		SearchableAttributeInfoContainer(parseState.searchableFieldsVector[i] ,
            				parseState.searchableAttributesRequiredFlagVector[i] ,
            				parseState.searchableAttributesDefaultVector[i] ,
            				0 , boostsMap[parseState.searchableFieldsVector[i]] ,
					parseState.searchableAttributesIsMultiValued[i]);
        }
    }

    // give each searchable attribute an id based on the order in the info map
    // should be consistent with the id in the schema
    map<string, SearchableAttributeInfoContainer>::iterator searchableAttributeIter = settings->searchableAttributesInfo.begin();
    for(unsigned idIter = 0; searchableAttributeIter != settings->searchableAttributesInfo.end() ; ++searchableAttributeIter, ++idIter){
    	searchableAttributeIter->second.offset = idIter;

	parseError << "Attribute: " << searchableAttributeIter->second.attributeName;
    }

    // checks the validity of the boost fields in boostsMap
    // must occur after parseIndexConfig() AND parseSchema()
    if (!isValidBoostFields(settings, boostsMap)) {
        configSuccess = false;
        parseError << "In core " << settings->name << ": Fields that are provided in the boostField do not match with the defined fields.";
	
        return;
    }

    // checks the validity of the boost values in boostsMap
    if (!isValidBoostFieldValues(boostsMap)) {
        configSuccess = false;
        parseError << "Boost values that are provided in the boostField are not in the range [1 to 100].";
        return;
    }
}

void ConfigManager::parseDataConfiguration(const xml_node &configNode,
					   bool &configSuccess,
					   std::stringstream &parseError,
					   std::stringstream &parseWarnings)
{
    CoreInfo_t *settings = NULL;

    // check for data source settings outside of <cores>
    if (configNode.child(dataDirString) || configNode.child(dataSourceTypeString) ||
	configNode.child(dataFileString)) {

        // create a default core for settings outside of <cores>
        bool created = false;
        if (coreSettings.find(getDefaultCoreName()) == coreSettings.end()) {
	  settings = new CoreInfo_t(this);
	    created= true;
	} else {
	    settings = coreSettings[getDefaultCoreName()];
	}

	parseDataSource(configNode, settings, configSuccess, parseError, parseWarnings);
	if (configSuccess == false) {
	    if (created) {
	        delete settings;
	    }
	    return;
	}
    }

    // if no errors, add settings to map
    if (settings != NULL) {
        settings->name = getDefaultCoreName();
	coreSettings[settings->name] = settings;
    }

    // <cores>
    xml_node childNode = configNode.child(coresString);
    parseCores(childNode, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }
}

/*
 * Parse a <schema> node, either directly under the root <config> or under <core>
 */
void ConfigManager::parseSchema(const xml_node &schemaNode, ParseState_t *parseState, CoreInfo_t *settings, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    string tempUse = "";

    vector<string> RefiningFieldsVector;
    vector<srch2::instantsearch::FilterType> RefiningFieldTypesVector;
    vector<bool> RefiningAttributesRequiredFlagVector;
    vector<string> RefiningAttributesDefaultVector;
    vector<bool> RefiningAttributesIsMultiValued;

    /*
     * <field>  in config.xml file
     */
    settings->isPrimSearchable = 0;

    xml_node fieldsNode = schemaNode.child(fieldsString);
    if (fieldsNode) {
        for (xml_node field = fieldsNode.first_child(); field; field = field.next_sibling()) {
            if (string(field.name()).compare(fieldString) == 0) {

            	/*
            	 * The following code decides whether this field is multi-valued or not.
            	 * If multivalued="true" in field tag, this field is multi-valued.
            	 */
            	bool isMultiValued = false;
                if(string(field.attribute(multiValuedString).value()).compare("") != 0){
		    tempUse = string(field.attribute(multiValuedString).value());
		    if(isValidBool(tempUse)){
		        isMultiValued = field.attribute(multiValuedString).as_bool();
		    }else{
		        parseError << "Config File Error: Unknown value for property '"<< multiValuedString <<"'.\n";
                        configSuccess = false;
                        return;
		    }
                }// We do not need the "else" part since multivalued property is not
                 // there so this field is not a multivalued field (false by default)
            	/*
            	 * The following code decides whether this field is searchable/refining or not.
            	 * It uses this logic:
            	 * if ( indexed == true ) {
				 *		  => attribute is both searchable and used for post processing
				 * } else {
			     *       if(searchable == true) => attribute is searchable
				 *       else => attribute is not searchable
				 *
				 *       if(refining == true) => attribute is used for post-processing
				 *       else => attribute is not used for post processing
			     * }
            	 */
                bool isSearchable = false;
                bool isRefining = false;
                if(string(field.attribute(indexedString).value()).compare("") != 0){
                    tempUse = string(field.attribute(indexedString).value());
                    if(isValidBool(tempUse)){
                        if(field.attribute(indexedString).as_bool()){ // indexed = true
                            isSearchable = true;
                            isRefining = true;
                        }else{ // indexed = false
                            if(string(field.attribute(searchableString).value()).compare("") != 0){
                                tempUse = string(field.attribute(searchableString).value());
                                if(isValidBool(tempUse)){
                                    if(field.attribute(searchableString).as_bool()){
                                        isSearchable = true;
                                    }else{
                                        isSearchable = false;
                                    }
                                }else{
                                    parseError << "Config File Error: Unknown value for property 'searchable'.\n";
                                    configSuccess = false;
                                    return;
                                }
                            }

                            if(string(field.attribute(refiningString).value()).compare("") != 0){
                                tempUse = string(field.attribute(refiningString).value());
                                if(isValidBool(tempUse)){
                                    if(field.attribute(refiningString).as_bool()){
                                        isRefining = true;
                                    }else{
                                        isRefining = false;
                                    }
                                }else{
                                    parseError << "Config File Error: Unknown value for property 'refining'.\n";
                                    configSuccess = false;
                                    return;
                                }
                            }
                        }
                    }else{
                        parseError << "Config File Error: Unknown value for property 'indexed'.\n";
                        configSuccess = false;
                        return;
                    }
                }else{ // indexed property is not there ...
                    if(string(field.attribute(searchableString).value()).compare("") != 0){
                        tempUse = string(field.attribute(searchableString).value());
                        if(isValidBool(tempUse)){
                            if(field.attribute(searchableString).as_bool()){
                                isSearchable = true;
                            }else{
                                isSearchable = false;
                            }
                        }else{
                            parseError << "Config File Error: Unknown value for property 'searchable'.\n";
                            configSuccess = false;
                            return;
                        }
                    }

                    if(string(field.attribute(refiningString).value()).compare("") != 0){
                        tempUse = string(field.attribute(refiningString).value());
                        if(isValidBool(tempUse)){
                            if(field.attribute(refiningString).as_bool()){
                                isRefining = true;
                            }else{
                                isRefining = false;
                            }
                        }else{
                            parseError << "Config File Error: Unknown value for property 'refining'.\n";
                            configSuccess = false;
                            return;
                        }
                    }
                }

                // If this field is the primary key, we only care about searchable and/or refining options.
                // We want to set primaryKey as a searchable and/or refining field
                // We assume the primary key is text, we don't get any type from user.
                // And no default value is accepted from user.
                if(string(field.attribute(nameString).value()).compare(settings->primaryKey) == 0){
                    if(isSearchable){
                        settings->isPrimSearchable = 1;
                        parseState->searchableFieldsVector.push_back(string(field.attribute(nameString).value()));
                        // there is no need for default value for primary key
                        parseState->searchableAttributesDefaultVector.push_back("");
                        // primary key is always required.
                        parseState->searchableAttributesRequiredFlagVector.push_back(true);
                    }

                    if(isRefining){
                        RefiningFieldsVector.push_back(settings->primaryKey);
                        RefiningFieldTypesVector.push_back(srch2::instantsearch::ATTRIBUTE_TYPE_TEXT);
                        RefiningAttributesDefaultVector.push_back("");
                        RefiningAttributesRequiredFlagVector.push_back(true);
                    }
                    continue;
                }
                // Checking if the values are empty or not
                if (string(field.attribute(nameString).value()).compare("") != 0
                        && string(field.attribute(typeString).value()).compare("") != 0) {
                    if(isSearchable){ // it is a searchable field
                        parseState->searchableFieldsVector.push_back(string(field.attribute(nameString).value()));
                        // Checking the validity of field type
                        tempUse = string(field.attribute(typeString).value());
                        if (! this->isValidFieldType(tempUse , true)) {
                            parseError << "Config File Error: " << tempUse << " is not a valid field type for searchable fields.\n";
                            parseError << " Note: searchable fields only accept 'text' type. Setting 'searchable' or 'indexed' to true makes a field searchable.\n";
                            configSuccess = false;
                            return;
                        }

                        if (string(field.attribute(defaultString).value()).compare("") != 0){
                            parseState->searchableAttributesDefaultVector.push_back(string(field.attribute(defaultString).value()));
                        }else{
                            parseState->searchableAttributesDefaultVector.push_back("");
                        }

                        tempUse = string(field.attribute(requiredString).value());
                        if (string(field.attribute("required").value()).compare("") != 0 && isValidBool(tempUse)){
                            parseState->searchableAttributesRequiredFlagVector.push_back(field.attribute(requiredString).as_bool());
                        }else{
			    parseState->searchableAttributesRequiredFlagVector.push_back(false);
                        }
                        parseState->searchableAttributesIsMultiValued.push_back(isMultiValued);
                    }

                    if(isRefining){ // it is a refining field
                        RefiningFieldsVector.push_back(string(field.attribute(nameString).value()));
                        // Checking the validity of field type
                        tempUse = string(field.attribute(typeString).value());
                        if (this->isValidFieldType(tempUse , false)) {
                            RefiningFieldTypesVector.push_back(parseFieldType(tempUse));
                        } else {
                            parseError << "Config File Error: " << tempUse << " is not a valid field type for refining fields.\n";
                            parseError << " Note: refining fields only accept 'text', 'integer', 'float' and 'time'. Setting 'refining' or 'indexed' to true makes a field refining.\n";
                            configSuccess = false;
                            return;
                        }


                        // Check the validity of field default value based on it's type
                        if (string(field.attribute(defaultString).value()).compare("") != 0){
                            tempUse = string(field.attribute("default").value());
                            if(isValidFieldDefaultValue(tempUse , RefiningFieldTypesVector.at(RefiningFieldTypesVector.size()-1) , isMultiValued)){

			        if(RefiningFieldTypesVector.at(RefiningFieldTypesVector.size()-1) == srch2::instantsearch::ATTRIBUTE_TYPE_TIME){
				    if(isMultiValued == false){
				        long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(tempUse);
                                        std::stringstream buffer;
                                        buffer << timeValue;
                                        tempUse = buffer.str();
				    }else{ // in the case of multivalued date and time we need to convert all values and reconstruct the list
				        // For example: ["01/01/1980","01/01/1980","01/01/1990","01/01/1982"]
                                        string convertedDefaultValues = "";
                                        vector<string> defaultValueTokens;
                                        splitString(tempUse , "," , defaultValueTokens);
                                        for(vector<string>::iterator defaultValueToken = defaultValueTokens.begin() ;
					    defaultValueToken != defaultValueTokens.end() ; ++defaultValueToken){
					    long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(*defaultValueToken);
					    std::stringstream buffer;
                                            buffer << timeValue;
					    if(defaultValueToken == defaultValueTokens.begin()){
					        convertedDefaultValues = buffer.str();
					    }else{
					        convertedDefaultValues = ","+buffer.str();
					    }
					}
				    }
                                }
                            }else{
                                parseError << "Config File Error: " << tempUse << " is not compatible with the type used for this field.\n";
                                tempUse = "";
                            }
                            RefiningAttributesDefaultVector.push_back(tempUse);
                        }else{
                            RefiningAttributesDefaultVector.push_back("");
                        }

                        tempUse = string(field.attribute(requiredString).value());
                        if (string(field.attribute(requiredString).value()).compare("") != 0 && isValidBool(tempUse)){
                            RefiningAttributesRequiredFlagVector.push_back(field.attribute("required").as_bool());
                        }else{
                            RefiningAttributesRequiredFlagVector.push_back(false);
                        }

                        RefiningAttributesIsMultiValued.push_back(isMultiValued);
                    }

                    // Checks for geo types. location_latitude and location_longitude are geo types
                    if (string(field.attribute(typeString).value()).compare(locationLatitudeString) == 0) {
                        parseState->hasLatitude = true;
                        settings->fieldLatitude = string(field.attribute(nameString).value());
                    }
                    if (string(field.attribute(typeString).value()).compare(locationLongitudeString) == 0) {
                        parseState->hasLongitude = true;
                        settings->fieldLongitude = string(field.attribute(nameString).value());
                    }

                } else { // if one of the values of name, type or indexed is empty
                    parseError << "For the searchable fields, "
                            << "providing values for 'name' and 'type' is required\n ";
                    configSuccess = false;
                    return;
                }
            }
        }
    } else { // No searchable fields provided.
        parseError << "No fields are provided.\n";
        configSuccess = false;
        return;
    }

    // Checking if there is any field or not.
    if (parseState->searchableFieldsVector.size() == 0) {
        parseError << "No searchable fields are provided.\n";
        configSuccess = false;
        return;
    }

    if(RefiningFieldsVector.size() != 0){
        for (unsigned iter = 0; iter < RefiningFieldsVector.size(); iter++) {
            settings->refiningAttributesInfo[RefiningFieldsVector[iter]] =
            		RefiningAttributeInfoContainer(RefiningFieldsVector[iter] ,
            				RefiningFieldTypesVector[iter] ,
            				RefiningAttributesDefaultVector[iter] ,
            				RefiningAttributesRequiredFlagVector[iter],
            				RefiningAttributesIsMultiValued[iter]);
        }
    }

    /*
     * <facetEnabled>  in config.xml file
     */
    settings->facetEnabled = false; // by default it is false
    xml_node childNode = schemaNode.child(facetEnabledString);
    if (childNode && childNode.text()) {
        string qtmt = childNode.text().get();
        if (isValidBool(qtmt)) {
            settings->facetEnabled = childNode.text().as_bool();
        } else {
            parseError << "The facetEnabled that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    /*
     * <facetFields>  in config.xml file
     */

    if(settings->facetEnabled){
        childNode = schemaNode.child(facetFieldsString);
        if (childNode) {
            for (xml_node field = childNode.first_child(); field; field = field.next_sibling()) {
                if (string(field.name()).compare(facetFieldString) == 0) {
		    if (string(field.attribute(nameString).value()).compare("") != 0
                            && string(field.attribute(facetTypeString).value()).compare("") != 0){
                        // insert the name of the facet
		        settings->facetAttributes.push_back(string(field.attribute(nameString).value()));
                        // insert the type of the facet
                        tempUse = string(field.attribute(facetTypeString).value());
                        int facetType = parseFacetType(tempUse);
                        if(facetType == 0){ // categorical
                            settings->facetTypes.push_back(facetType);
                            // insert place holders for start,end and gap
                            settings->facetStarts.push_back("");
                            settings->facetEnds.push_back("");
                            settings->facetGaps.push_back("");
                        }else if(facetType == 1){ // range
                            settings->facetTypes.push_back(facetType);
                            // insert start
                            string startTextValue = string(field.attribute(facetStartString).value());
                            string facetAttributeString = string(field.attribute(nameString).value());
                            srch2::instantsearch::FilterType facetAttributeType ;
                            if(settings->refiningAttributesInfo.find(facetAttributeString) != settings->refiningAttributesInfo.end()){
                                facetAttributeType = settings->refiningAttributesInfo.find(facetAttributeString)->second.attributeType;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                settings->facetEnabled = false;
                                break;
                            }
                            if(facetAttributeType == srch2is::ATTRIBUTE_TYPE_TIME){
                                if(srch2is::DateAndTimeHandler::verifyDateTimeString(startTextValue , srch2is::DateTimeTypePointOfTime)
                                        || srch2is::DateAndTimeHandler::verifyDateTimeString(startTextValue , srch2is::DateTimeTypeNow) ){
                                    long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(startTextValue);
                                    std::stringstream buffer;
                                    buffer << timeValue;
                                    startTextValue = buffer.str();
                                }else{
                                    parseError << "Facet attribute start value is in wrong format.Facet disabled.\n";
                                    settings->facetEnabled = false;
                                    break;
                                }
                            }
                            settings->facetStarts.push_back(startTextValue);

                            // insert end
                            string endTextValue = string(field.attribute(facetEndString).value());
                            if(settings->refiningAttributesInfo.find(facetAttributeString) != settings->refiningAttributesInfo.end()){
                                facetAttributeType = settings->refiningAttributesInfo.find(facetAttributeString)->second.attributeType;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                settings->facetEnabled = false;
                                break;
                            }
                            if(facetAttributeType == srch2is::ATTRIBUTE_TYPE_TIME){
                                if(srch2is::DateAndTimeHandler::verifyDateTimeString(endTextValue , srch2is::DateTimeTypePointOfTime)
                                        || srch2is::DateAndTimeHandler::verifyDateTimeString(endTextValue , srch2is::DateTimeTypeNow) ){
                                    long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(endTextValue);
                                    std::stringstream buffer;
                                    buffer << timeValue;
                                    endTextValue = buffer.str();
                                }else{
                                    parseError << "Facet attribute start value is in wrong format.Facet disabled.\n";
                                    settings->facetEnabled = false;
                                    break;
                                }
                            }
                            settings->facetEnds.push_back(endTextValue);

                            // insert gap
                            string gapTextValue = string(field.attribute(facetGapString).value());
                            if(settings->refiningAttributesInfo.find(facetAttributeString) != settings->refiningAttributesInfo.end()){
                                facetAttributeType = settings->refiningAttributesInfo.find(facetAttributeString)->second.attributeType;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                settings->facetEnabled = false;
                                break;
                            }
                            if(facetAttributeType == srch2is::ATTRIBUTE_TYPE_TIME){
                                if(!srch2is::DateAndTimeHandler::verifyDateTimeString(gapTextValue , srch2is::DateTimeTypeDurationOfTime) ){
                                    parseError << "Facet attribute end value is in wrong format.Facet disabled.\n";
                                    settings->facetEnabled = false;
                                    break;
                                }
                            }
                            settings->facetGaps.push_back(gapTextValue);
                        }else{
                            parseError << "Facet type is not recognized. Facet disabled.";
                            settings->facetEnabled = false;
                            break;
                        }

                    }
                }
            }
        }
    }

    if(! settings->facetEnabled){
        settings->facetAttributes.clear();
        settings->facetTypes.clear();
        settings->facetStarts.clear();
        settings->facetEnds.clear();
        settings->facetGaps.clear();
    }

    // Analyzer flags : Everything is disabled by default.
    settings->stemmerFlag = false;
    settings->stemmerFile = "";
    settings->stopFilterFilePath = "";
    settings->synonymFilterFilePath = "";
    settings->protectedWordsFilePath = "";
    settings->synonymKeepOrigFlag = false;

    childNode = schemaNode.child(typesString);
    if (childNode) {        // Checks if <schema><types> exists or not
        for (xml_node fieldType = childNode.first_child(); fieldType; fieldType = fieldType.next_sibling()) { // Going on the children
            if ((string(fieldType.name()).compare(fieldTypeString) == 0)) { // Finds the fieldTypes
                if (string(fieldType.attribute(nameString).value()).compare(textEnString) == 0) {
                    // Checking if the values are empty or not
                    xml_node childNodeTemp = fieldType.child(analyzerString); // looks for analyzer
                    for (xml_node field = childNodeTemp.first_child(); field; field = field.next_sibling()) {
                        if (string(field.name()).compare(filterString) == 0) {
                            if (string(field.attribute("name").value()).compare(porterStemFilterString) == 0) { // STEMMER FILTER
                                if (string(field.attribute(dictionaryString).value()).compare("") != 0) { // the dictionary for porter stemmer is set.
				    settings->stemmerFlag = true;
				    tempUse = string(field.attribute(dictionaryString).value());
				    trimSpacesFromValue(tempUse, porterStemFilterString, parseWarnings);
				    settings->stemmerFile = this->srch2Home + tempUse;
                                }
                            } else if (string(field.attribute(nameString).value()).compare(stopFilterString) == 0) { // STOP FILTER
                                if (string(field.attribute(wordsString).value()).compare("") != 0) { // the words file for stop filter is set.
				    tempUse = string(field.attribute("words").value());
				    trimSpacesFromValue(tempUse, stopFilterString, parseWarnings);
				    settings->stopFilterFilePath = this->srch2Home + tempUse;
                                }
                            } /*else if (string(field.attribute(nameString).value()).compare(SynonymFilterString) == 0) {
                                if (string(field.attribute(synonymsString).value()).compare("") != 0) { // the dictionary file for synonyms is set
                                    this->synonymFilterFilePath = this->srch2Home
                                            + string(field.attribute(synonymsString).value());
                                    // checks the validity of boolean provided for 'expand'
                                    tempUse = string(field.attribute(expandString).value());
                                    if (this->isValidBool(tempUse)) {
                                        this->synonymKeepOrigFlag = field.attribute(expandString).as_bool();
                                    } else {
                                        parseError << "Config File Error: can not convert from '" << tempUse
                                                << "' to boolean\n";
                                        configSuccess = false;
                                        return;
                                    }
                              }
                            }*/
                            else if (string(field.attribute(nameString).value()).compare(protectedWordFilterString) == 0) {
                                if (string(field.attribute(wordsString).value()).compare("") != 0) { // the words file for stop filter is set.
				    tempUse = string(field.attribute("words").value());
				    trimSpacesFromValue(tempUse, protectedWordFilterString, parseWarnings);
				    settings->protectedWordsFilePath = this->srch2Home + tempUse;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        parseWarnings << "Analyzer Filters will be disabled.\n";
    }
    /*
     * <Schema/>: End
     */
}

void ConfigManager::parse(const pugi::xml_document& configDoc,
			  bool &configSuccess,
			  std::stringstream &parseError,
			  std::stringstream &parseWarnings)
{
    string tempUse = ""; // This is just for temporary use.

    // create a default core for settings outside of <cores>
    CoreInfo_t *defaultSettings = NULL;
    if (coreSettings.find(getDefaultCoreName()) == coreSettings.end()) {
        defaultSettings = new CoreInfo_t(this);
	defaultSettings->name = getDefaultCoreName();
	coreSettings[defaultSettings->name] = defaultSettings;
    } else {
        defaultSettings = coreSettings[getDefaultCoreName()];
    }

    xml_node configNode = configDoc.child(configString);

    // srch2Home is a required field
    xml_node childNode = configNode.child(srch2HomeString);
    if (childNode && childNode.text()) { // checks if the config/srch2Home has any text in it or not
        tempUse = string(childNode.text().get());
	trimSpacesFromValue(tempUse, srch2HomeString, parseWarnings, "/");
	srch2Home = tempUse;
    } else {
        parseError << "srch2Home is not set.\n";
        configSuccess = false;
        return;
    }

    // parse all data source settings - no core (default core) and multiples core handled
    // requires indexType to have been loaded by parseIndexConfig()
    parseDataConfiguration(configNode, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }

    /*
     * <Config> in config.xml file
     */
    // licenseFile is a required field
    childNode = configNode.child(licenseFileString);
    if (childNode && childNode.text()) { // checks if config/licenseFile exists and have any text value or not
        tempUse = string(childNode.text().get());
	trimSpacesFromValue(tempUse, licenseFileString, parseWarnings);
	this->licenseKeyFile = this->srch2Home + tempUse;
    } else {
        parseError << "License key is not set.\n";
        configSuccess = false;
        return;
    }

    // listeningHostname is a required field
    childNode = configNode.child(listeningHostStringString);
    if (childNode && childNode.text()) { // checks if config/listeningHostname exists and have any text value or not
        this->httpServerListeningHostname = string(childNode.text().get());
    } else {
        parseError << "listeningHostname is not set.\n";
        configSuccess = false;
        return;
    }

    // listeningPort is a required field
    childNode = configNode.child(listeningPortString);
    if (childNode && childNode.text()) { // checks if the config/listeningPort has any text in it or not
        this->httpServerListeningPort = string(childNode.text().get());
    } else {
        parseError << "listeningPort is not set.\n";
        configSuccess = false;
        return;
    }

    childNode = configNode.child(queryString);
    parseQuery(childNode, defaultSettings, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }

    this->writeApiType = HTTPWRITEAPI;

    childNode = configNode.child(updateHandlerString).child(maxDocsString);
    bool mdflag = false;
    if (childNode && childNode.text()) {
        string md = childNode.text().get();
        if (this->isValidMaxDoc(md)) {
            this->documentLimit = childNode.text().as_uint();
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
    childNode = configNode.child(updateHandlerString).child(maxMemoryString);
    if (childNode && childNode.text()) {
        string mm = childNode.text().get();
        if (this->isValidMaxMemory(mm)) {
            this->memoryLimit = childNode.text().as_uint();
            mmflag = true;
        }
    }
    if (!mmflag) {
        parseError << "MaxDoc is not set correctly\n";
        configSuccess = false;
        return;
    }

    // mergeEveryNSeconds
    childNode = configNode.child(updateHandlerString).child(mergePolicyString).child(mergeEveryNSecondsString);
    bool mensflag = false;
    if (childNode && childNode.text()) {
        string mens = childNode.text().get();
        if (this->isValidMergeEveryNSeconds(mens)) {
            this->mergeEveryNSeconds = childNode.text().as_uint();
            mensflag = true;
        }
    }
    if (!mensflag) {
        parseError << "mergeEveryNSeconds is not set.\n";
        configSuccess = false;
        return;
    }

    // mergeEveryMWrites
    childNode = configNode.child(updateHandlerString).child(mergePolicyString).child(mergeEveryMWritesString);
    bool memwflag = false;
    if (childNode && childNode.text()) {
        string memw = childNode.text().get();

        if (this->isValidMergeEveryMWrites(memw)) {
            this->mergeEveryMWrites = childNode.text().as_uint();
            memwflag = true;
        }
    }
    if (!memwflag) {
        parseError << "mergeEveryMWrites is not set.\n";
        configSuccess = false;
        return;
    }

    // logLevel is required
    this->loglevel = Logger::SRCH2_LOG_INFO;
    childNode = configNode.child(updateHandlerString).child(updateLogString).child(logLevelString);
    bool llflag = true;
    if (childNode && childNode.text()) {
        string ll = childNode.text().get();
        if (this->isValidLogLevel(ll)) {
            this->loglevel = static_cast<Logger::LogLevel>(childNode.text().as_int());
        } else {
            llflag = false;
        }
    }
    if (!llflag) {
        parseError << "Log Level is not set correctly\n";
        configSuccess = false;
        return;
    }

    // accessLogFile is required
    childNode = configNode.child(updateHandlerString).child(updateLogString).child(accessLogFileString);
    if (childNode && childNode.text()) {
        tempUse = string(childNode.text().get());
	trimSpacesFromValue(tempUse, updateLogString, parseWarnings);
	this->httpServerAccessLogFile = this->srch2Home + tempUse;
    } else {
        parseError << "httpServerAccessLogFile is not set.\n";
        configSuccess = false;
        return;
    }

    /*
     * query: END
     */

    if (defaultSettings->supportAttributeBasedSearch && defaultSettings->searchableAttributesInfo.size() > 31) {
        parseError
                << "To support attribute-based search, the number of searchable attributes cannot be bigger than 31.\n";
        configSuccess = false;
        return;
    }


    defaultSettings->allowedRecordTokenizerCharacters = "";
    this->ordering = 0;
    this->attributeToSort = 0;

    // set default value for updateHistogramEveryPSeconds and updateHistogramEveryQWrites because there
    // is no option in xml for this one yet
    float updateHistogramWorkRatioOverTime = 0.1; // 10 percent of background thread process is spent for updating histogram
    this->updateHistogramEveryPMerges = (unsigned)
    		( 1.0 / updateHistogramWorkRatioOverTime) ; // updateHistogramEvery 10 Merges
    this->updateHistogramEveryQWrites =
    		(unsigned)((this->mergeEveryMWrites * 1.0 ) / updateHistogramWorkRatioOverTime); // 10000 for mergeEvery 1000 Writes

    // set default number of suggestions because we don't have any config options for this yet
    this->defaultNumberOfSuggestions = 5;

    // setting default values for getAllResults optimization parameters
    // <getAllResultsMaxResultsThreshold>10000</getAllResultsMaxResultsThreshold>
    childNode = configNode.child(getAllResultsMaxResultsThreshold);
    if (childNode && childNode.text()) {
        string kpt = childNode.text().get();

        if (this->isValidGetAllResultsMaxResultsThreshold(kpt)) {
            this->getAllResultsNumberOfResultsThreshold = childNode.text().as_uint();
        }else{
        	parseError << "getAllResultsMaxResultsThreshold has an invalid format. Use 10000 as its value.\n";
        	this->getAllResultsNumberOfResultsThreshold = 10000;
        }
    }else{
    	this->getAllResultsNumberOfResultsThreshold = 10000;
    }

    // <getAllResultsKAlternative>2000</getAllResultsKAlternative>
    childNode = configNode.child(getAllResultsKAlternative);
    if (childNode && childNode.text()) {
        string kpt = childNode.text().get();

        if (this->isValidGetAllResultsKAlternative(kpt)) {
            this->getAllResultsNumberOfResultsToFindInEstimationMode = childNode.text().as_uint();
        }else{
        	parseError << "getAllResultsKAlternative has an invalid format. Use 2000 as its value.\n";
        	this->getAllResultsNumberOfResultsToFindInEstimationMode = 2000;
        }
    }else{
    	this->getAllResultsNumberOfResultsToFindInEstimationMode = 2000;
    }


    childNode = configNode.child(keywordPopularityThresholdString);
    if (childNode && childNode.text()) {
        string kpt = childNode.text().get();

        if (this->isValidKeywordPopularityThreshold(kpt)) {
            this->keywordPopularityThreshold = childNode.text().as_uint();
        }else{
        	parseError << "keywordPopularityThreshold has an invalid format. Use 50000 as its value. \n";
        	keywordPopularityThreshold = 50000;
        }
    }else{
    	keywordPopularityThreshold = 50000;
    }
}

void ConfigManager::_setDefaultSearchableAttributeBoosts(const string &coreName, const vector<string> &searchableAttributesVector)
{
    CoreInfo_t *settings = NULL;
    if (coreName.compare("") != 0) {
        settings = ((CoreInfoMap_t) coreSettings)[coreName];
    } else {
        settings = getDefaultDataSource();
    }

    for (unsigned iter = 0; iter < searchableAttributesVector.size(); iter++) {
        settings->searchableAttributesInfo[searchableAttributesVector[iter]] =
	    SearchableAttributeInfoContainer(searchableAttributesVector[iter] , false, "" , iter , 1 , false);
    }
}

ConfigManager::~ConfigManager()
{
    for (CoreInfoMap_t::iterator iterator = coreSettings.begin(); iterator != coreSettings.end(); iterator++) {
        delete iterator->second;
    }
    coreSettings.clear();
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

uint32_t ConfigManager::getUpdateHistogramEveryPMerges() const {
    return updateHistogramEveryPMerges;
}

uint32_t ConfigManager::getUpdateHistogramEveryQWrites() const {
    return updateHistogramEveryQWrites;
}

unsigned ConfigManager::getKeywordPopularityThreshold() const {
	return keywordPopularityThreshold;
}

int ConfigManager::getIndexType(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->indexType;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->indexType;
}

bool ConfigManager::getSupportSwapInEditDistance(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->supportSwapInEditDistance;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->supportSwapInEditDistance;
}

const string& ConfigManager::getAttributeLatitude(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->fieldLatitude;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->fieldLatitude;
}

const string& ConfigManager::getAttributeLongitude(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->fieldLongitude;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->fieldLongitude;
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

WriteApiType ConfigManager::getWriteApiType() const {
    return writeApiType;
}

const string& ConfigManager::getIndexPath(const string &coreName) const {
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->indexPath;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->indexPath;
}

const string& ConfigManager::getFilePath() const {
    return this->filePath;
}

const string& ConfigManager::getPrimaryKey(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->primaryKey;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->primaryKey;
}

const map<string, SearchableAttributeInfoContainer > * ConfigManager::getSearchableAttributes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultDataSource()->searchableAttributesInfo;
    }
    return &((CoreInfoMap_t) coreSettings)[coreName]->searchableAttributesInfo;
}

const map<string, RefiningAttributeInfoContainer > * ConfigManager::getRefiningAttributes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultDataSource()->refiningAttributesInfo;
    }
    return &((CoreInfoMap_t) coreSettings)[coreName]->refiningAttributesInfo;
}

bool ConfigManager::isFacetEnabled(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->facetEnabled;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->facetEnabled;
}

const vector<string> * ConfigManager::getFacetAttributes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultDataSource()->facetAttributes;
    }
    return &((CoreInfoMap_t) coreSettings)[coreName]->facetAttributes;
}

const vector<int> * ConfigManager::getFacetTypes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultDataSource()->facetTypes;
    }
    return &((CoreInfoMap_t) coreSettings)[coreName]->facetTypes;
}

const vector<string> * ConfigManager::getFacetStarts(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultDataSource()->facetStarts;
    }
    return &((CoreInfoMap_t) coreSettings)[coreName]->facetStarts;
}

const vector<string> * ConfigManager::getFacetEnds(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultDataSource()->facetEnds;
    }
    return &((CoreInfoMap_t) coreSettings)[coreName]->facetEnds;
}

const vector<string> * ConfigManager::getFacetGaps(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultDataSource()->facetGaps;
    }
    return &((CoreInfoMap_t) coreSettings)[coreName]->facetGaps;
}


string ConfigManager::getSrch2Home() const {
    return srch2Home;
}

bool ConfigManager::getStemmerFlag(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->stemmerFlag;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->stemmerFlag;
}

const string &ConfigManager::getStemmerFile(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->stemmerFile;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->stemmerFile;
}

const string &ConfigManager::getSynonymFilePath(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->synonymFilterFilePath;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->synonymFilterFilePath;
}

const string &ConfigManager::getProtectedWordsFilePath(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->protectedWordsFilePath;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->protectedWordsFilePath;
}

bool ConfigManager::getSynonymKeepOrigFlag(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->synonymKeepOrigFlag;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->synonymKeepOrigFlag;
}

const string &ConfigManager::getStopFilePath(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->stopFilterFilePath;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->stopFilterFilePath;
}

const string& ConfigManager::getAttributeRecordBoostName(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->recordBoostField;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->recordBoostField;
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

const string& ConfigManager::getRecordAllowedSpecialCharacters(const string &coreName) const {
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->allowedRecordTokenizerCharacters;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->allowedRecordTokenizerCharacters;
}

int ConfigManager::getSearchType(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->searchType;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->searchType;
}

int ConfigManager::getIsPrimSearchable(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->isPrimSearchable;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->isPrimSearchable;
}

bool ConfigManager::getIsFuzzyTermsQuery() const {
    return exactFuzzy;
}

bool ConfigManager::getQueryTermPrefixType() const {
    return queryTermPrefixType;
}

unsigned ConfigManager::getQueryTermBoost(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->queryTermBoost;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->queryTermBoost;
}

float ConfigManager::getFuzzyMatchPenalty() const {
    return fuzzyMatchPenalty;
}

float ConfigManager::getQueryTermSimilarityThreshold() const {
    return queryTermSimilarityThreshold;
}

float ConfigManager::getQueryTermLengthBoost() const {
    return queryTermLengthBoost;
}

float ConfigManager::getPrefixMatchPenalty() const {
    return prefixMatchPenalty;
}

bool ConfigManager::getSupportAttributeBasedSearch(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->supportAttributeBasedSearch;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->supportAttributeBasedSearch;
}

ResponseType ConfigManager::getSearchResponseFormat() const {
    return searchResponseFormat;
}

const string& ConfigManager::getLicenseKeyFileName() const {
    return licenseKeyFile;
}

const string& ConfigManager::getHTTPServerListeningHostname() const {
    return httpServerListeningHostname;
}

const string& ConfigManager::getHTTPServerListeningPort() const {
    return httpServerListeningPort;
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

bool ConfigManager::isRecordBoostAttributeSet(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->recordBoostFieldFlag;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->recordBoostFieldFlag;
}

const string& ConfigManager::getHTTPServerAccessLogFile() const {
    return httpServerAccessLogFile;
}

const Logger::LogLevel& ConfigManager::getHTTPServerLogLevel() const {
    return loglevel;
}

unsigned ConfigManager::getCacheSizeInBytes() const {
    return cacheSizeInBytes;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////// Validate & Helper functions
///////////////////////////////////////////////////////////////////////////////////////////////

// splitString gets a string as its input and a delimiter. It splits the string based on the delimiter and pushes back the values to the result
void ConfigManager::splitString(string str, const string& delimiter, vector<string>& result) {
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
            string boost = boostTokens[i].substr(pos + 1, boostTokens[i].length());
            boosts[field] = (unsigned) atoi(boost.c_str());
            if(boosts[field] < 1 || boosts[field] > 100){
            	boosts[field] = 1;
            }
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

bool ConfigManager::isFloat(string str) {
    std::size_t found = str.find(".");
    if (found != std::string::npos) {
        str.erase(found, 1);
        if (str.find(".") != string::npos) {
            return false;
        }
    }
    return this->isOnlyDigits(str);
}

bool ConfigManager::isValidFieldType(string& fieldType , bool isSearchable) {
    if(isSearchable){
        // supported types are: text, location_latitude, location_longitude
        if ((fieldType.compare("text") == 0) || (fieldType.compare(locationLatitudeString) == 0)
                || (fieldType.compare("location_longitude") == 0)) {
            return true;
        }
        return false;
    }else{
        // supported types are: text, integer, float, time
        if ((fieldType.compare("text") == 0) || (fieldType.compare("integer") == 0)
                || (fieldType.compare("float") == 0) || (fieldType.compare("time") == 0)) {
            return true;
        }
        return false;
    }
}

bool ConfigManager::isValidFieldDefaultValue(string& defaultValue, srch2::instantsearch::FilterType fieldType , bool isMultiValued)
{
    if(isMultiValued == false){
	return validateValueWithType(fieldType , defaultValue);
    }

    // if it is a multi-valued attribute, default value is a comma separated list of default values. example : "tag1,tag2,tag3"
    vector<string> defaultValueTokens;
    splitString(defaultValue , "," , defaultValueTokens);
    for(vector<string>::iterator defaultValueToken = defaultValueTokens.begin() ; defaultValueToken != defaultValueTokens.end() ; ++defaultValueToken){
	if( validateValueWithType(fieldType , *defaultValueToken) == false){
	    return false;
	}
    }
    return true;
}

bool ConfigManager::isValidBool(string& fieldType) {
    // supported types are: text, location_latitude, location_longitude
    if ((fieldType.compare("true") == 0) || (fieldType.compare("True") == 0) || (fieldType.compare("TRUE") == 0)
            || (fieldType.compare("false") == 0) || (fieldType.compare("False") == 0)
            || (fieldType.compare("FALSE") == 0)) {
        return true;
    }
    return false;
}

// validates if all the fields from boosts Fields are in the Triple or not.
bool ConfigManager::isValidBoostFields(const CoreInfo_t *settings,
				       map<string, unsigned>& boostFields)
{
    map<string, unsigned>::iterator iter;
    for (iter = boostFields.begin(); iter != boostFields.end(); ++iter) {
        if (settings->searchableAttributesInfo.count(iter->first) > 0) {
            continue;
        }
        return false;
    }
    return true;
}

bool ConfigManager::isValidBoostFieldValues(map<string, unsigned>& boostMap) {
    map<string, unsigned>::iterator iter;
    for (iter = boostMap.begin(); iter != boostMap.end(); ++iter) {
        if (iter->second <= 100 && iter->second >= 1) {
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
    if (indexCreateLoad.compare("0") == 0 || indexCreateLoad.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidRecordScoreExpession(string& recordScoreExpression) {
    // We should validate this too.
    return true;
}

bool ConfigManager::isValidFuzzyMatchPenalty(string& fuzzyMatchPenalty) {
    return this->isFloat(fuzzyMatchPenalty);
}

bool ConfigManager::isValidQueryTermSimilarityThreshold(string & qTermSimilarityThreshold){
    return this->isFloat(qTermSimilarityThreshold);
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
    return (this->isOnlyDigits(maxSearchThreads) && (atoi(maxSearchThreads.c_str()) > 0)); // should be number and greater that 1
}

bool ConfigManager::isValidBooleanValue(string& fieldValue) {
    if (fieldValue.compare("0") == 0 || fieldValue.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidQueryTermFuzzyType(string& queryTermFuzzyType) {
    if (queryTermFuzzyType.compare("0") == 0 || queryTermFuzzyType.compare("1") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidQueryTermPrefixType(string& queryTermPrefixType) {
    if (queryTermPrefixType.compare("0") == 0 || queryTermPrefixType.compare("1") == 0) {
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
    if (responseContentType.compare("0") == 0 || responseContentType.compare("1") == 0
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
        if (atoi(mergeEveryNSeconds.c_str()) >= 1) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidMergeEveryMWrites(string& mergeEveryMWrites) {
    if (this->isOnlyDigits(mergeEveryMWrites)) {
        if (atoi(mergeEveryMWrites.c_str()) >= 1) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidKeywordPopularityThreshold(string kpt){
    if (this->isOnlyDigits(kpt)) {
        if (atoi(kpt.c_str()) >= 1) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidGetAllResultsMaxResultsThreshold(string kpt){
    if (this->isOnlyDigits(kpt)) {
        if (atoi(kpt.c_str()) >= 1) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidGetAllResultsKAlternative(string kpt){
    if (this->isOnlyDigits(kpt)) {
        if (atoi(kpt.c_str()) >= 1) {
            return true;
        }
    }
    return false;
}

bool ConfigManager::isValidLogLevel(string& logLevel) {
    if (logLevel.compare("0") == 0 || logLevel.compare("1") == 0 || logLevel.compare("2") == 0
            || logLevel.compare("3") == 0) {
        return true;
    }
    return false;
}

bool ConfigManager::isValidIndexType(string& indexType) {
    if (indexType.compare("0") == 0 || indexType.compare("1") == 0 ){
        return true;
    }
    return false;
}

bool ConfigManager::isValidSearcherType(string& searcherType) {
    if (searcherType.compare("0") == 0 || searcherType.compare("1") == 0 ){
        return true;
    }
    return false;
}

srch2::instantsearch::FilterType ConfigManager::parseFieldType(string& fieldType){
    if (fieldType.compare("integer") == 0)
        return srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED;
    else if (fieldType.compare("float") == 0)
        return srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT;
    else if (fieldType.compare("text") == 0)
        return srch2::instantsearch::ATTRIBUTE_TYPE_TEXT;
    else if (fieldType.compare("time") == 0)
        return srch2::instantsearch::ATTRIBUTE_TYPE_TIME;

    ASSERT(false);
    return srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED;
}

int ConfigManager::parseFacetType(string& facetType){
    string facetTypeLowerCase = boost::to_lower_copy(facetType);
    if(facetTypeLowerCase.compare("categorical") == 0){
        return 0;
    }else if(facetTypeLowerCase.compare("range") == 0){
        return 1;
    }else{
        return -1;
    }
}

// JUST FOR Wrapper TEST
void ConfigManager::setFilePath(const string& dataFile) {
    this->filePath = dataFile;
}


bool ConfigManager::isPositionIndexEnabled(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultDataSource()->enablePositionIndex;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->enablePositionIndex;
}

const string& ConfigManager::getMongoServerHost(const string &coreName) const
{
    if (coreName.compare("") == 0) {
	return getDefaultDataSource()->mongoHost;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->mongoHost;
}

const string& ConfigManager::getMongoServerPort(const string &coreName) const
{
    if (coreName.compare("") == 0) {
	return getDefaultDataSource()->mongoPort;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->mongoPort;
}

const string& ConfigManager::getMongoDbName(const string &coreName) const
{
    if (coreName.compare("") == 0) {
	return getDefaultDataSource()->mongoDbName;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->mongoDbName;
}

const string& ConfigManager::getMongoCollection (const string &coreName) const
{
    if (coreName.compare("") == 0) {
	return getDefaultDataSource()->mongoCollection;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->mongoCollection;
}

const unsigned ConfigManager::getMongoListenerWaitTime (const string &coreName) const
{
    if (coreName.compare("") == 0) {
	return getDefaultDataSource()->mongoListenerWaitTime;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->mongoListenerWaitTime;
}

const unsigned ConfigManager::getMongoListenerMaxRetryCount(const string &coreName) const
{
    if (coreName.compare("") == 0) {
	return getDefaultDataSource()->mongoListenerMaxRetryOnFailure;
    }
    return ((CoreInfoMap_t) coreSettings)[coreName]->mongoListenerMaxRetryOnFailure;
}

CoreInfo_t *ConfigManager::getDefaultDataSource() const
{
    string n = getDefaultCoreName();
    CoreInfo_t *settings = ((CoreInfoMap_t) coreSettings)[n];
    return settings;
    //        return coreSettings[getDefaultCoreName()];
}

// end of namespaces
}
}
