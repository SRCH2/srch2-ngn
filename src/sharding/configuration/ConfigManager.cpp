
//$Id: ConfigManager.h 2013-07-5 02:11:13Z iman $

#include "ConfigManager.h"

#include <algorithm>
#include "src/server/util/xmlParser/pugixml.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/program_options.hpp>
#include <assert.h>
#include "src/core/util/Logger.h"
#include <boost/algorithm/string.hpp>
#include <sys/stat.h>

#include "src/core/util/DateAndTimeHandler.h"
#include "src/wrapper/ParserUtility.h"
#include "src/core/util/Assert.h"
#include "src/core/analyzer/CharSet.h"

#include "boost/algorithm/string_regex.hpp"
#include "boost/filesystem/path.hpp"

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace pugi;
// it is related to the pgixml.hpp which is a xml parser.

using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper { 

// configuration file tag and attribute names for ConfigManager
// *MUST* be lowercase

const char* const ConfigManager::nodeListeningHostNameTag = "listeninghostname";
const char* const ConfigManager::nodeListeningPortTag = "listeningport";
const char* const ConfigManager::nodeCurrentTag = "this-is-me";
const char* const ConfigManager::nodeNameTag = "node-name";
const char* const ConfigManager::nodeMasterTag = "node-master";
const char* const ConfigManager::nodeDataTag = "node-data";
const char* const ConfigManager::nodeHomeTag = "srch2home";
const char* const ConfigManager::nodeDataDirTag = "datadir";
const char* const ConfigManager::primaryShardTag = "core-number_of_shards";
const char* const ConfigManager::replicaShardTag = "core-number_of_replicas";
const char* const ConfigManager::clusterNameTag = "cluster-name";
const int ConfigManager::DefaultNumberOfPrimaryShards = 5;
const int ConfigManager::DefaultNumberOfReplicas = 1;
const char* const ConfigManager::DefaultClusterName = "SRCH2Cluster";
const char* const ConfigManager::discoveryNodeTag = "discovery";
const char* const ConfigManager::pingIntervalTag = "ping-interval";
const char* const ConfigManager:: pingTimeoutTag= "ping-timeout";
const char* const ConfigManager::retryCountTag = "retry-count";


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
const char* const ConfigManager::enableCharOffsetIndexString = "enablecharoffsetindex";
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
const char* const ConfigManager::multipleCoresString = "cores";
const char* const ConfigManager::singleCoreString = "core";
const char* const ConfigManager::defaultCoreNameString = "defaultcorename";
const char *const ConfigManager::allowedRecordSpecialCharactersString = "allowedrecordspecialcharacters";

const char* const ConfigManager::searchPortString = "searchport";
const char* const ConfigManager::suggestPortString = "suggestport";
const char* const ConfigManager::infoPortString = "infoport";
const char* const ConfigManager::docsPortString = "docsport";
const char* const ConfigManager::updatePortString = "updateport";
const char* const ConfigManager::savePortString = "saveport";
const char* const ConfigManager::exportPortString = "exportport";
const char* const ConfigManager::resetLoggerPortString = "resetloggerport";

const char* const ConfigManager::highLightString = "highlight";
const char* const ConfigManager::highLighterString = "highlighter";
const char* const ConfigManager::exactTagPre = "exacttagpre";
const char* const ConfigManager::exactTagPost = "exacttagpost";
const char* const ConfigManager::fuzzyTagPre = "fuzzytagpre";
const char* const ConfigManager::fuzzyTagPost = "fuzzytagpost";
const char* const ConfigManager::snippetSize = "snippetsize";

const char* const ConfigManager::defaultFuzzyPreTag = "<b>";
const char* const ConfigManager::defaultFuzzyPostTag = "</b>";
const char* const ConfigManager::defaultExactPreTag = "<b>";
const char* const ConfigManager::defaultExactPostTag = "</b>";

//In later version, this should be handled by SM
void ConfigManager::setNodeId(){
    vector<Node>* nodes = this->cluster.getNodes();
    for(int i = 0; i < nodes->size(); i++){
        (*nodes)[i].setId(i);
    }
}

bool ConfigManager::isLocal(ShardId& shardId){
	Shard s = this->cluster.shardMap[shardId];
	return this->getCurrentNodeId() == s.getNodeId();
}
//Function Definition for verifyConsistency; it checks if the port number of core is different
//from the port number being used by the node for communication with other nodes
bool ConfigManager::verifyConsistency()
{
    Cluster* currentCluster = this->getCluster();
    vector<Node>* nodes = currentCluster->getNodes();
    Node currentNode;

    //The for loop below gets the current node
    for(int i = 0; i < nodes->size(); i++){
        if(nodes->at(i).thisIsMe == true)
    	    currentNode = nodes->at(i);
    }

    //The for loop below compares the current node's port number with the port number of cores
    for(CoreInfoMap_t::iterator it = this->coreInfoIterateBegin(); it != this->coreInfoIterateEnd(); it++){
        int num = (uint)atol(it->second->getHTTPServerListeningPort().c_str());
        if(num == currentNode.getPortNumber()){
    	    return false;
        }
    }
    return true;
}

ConfigManager::ConfigManager(const string& configFile)
{
    this->configFile = configFile;
    defaultCoreName = "__DEFAULTCORE__";
    defaultCoreSetFlag = false;
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

    //The below function sets node Id for all the nodes, in later version this should be done by synchronization manager
    this->setNodeId();

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
        parseWarnings << "Trimmed whitespace from the variable " << fieldName << " \"" << oldValue << "\"\n";
    }
    if (append != NULL) {
        fieldValue += append;
    }
}

const ConfigManager::CoreInfoMap_t& ConfigManager::getCoreInfoMap() const
{
  return coreInfoMap;
}

void ConfigManager::parseIndexConfig(const xml_node &indexConfigNode, CoreInfo_t *coreInfo, map<string, unsigned> &boostsMap, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    xml_node childNode = indexConfigNode.child(indexTypeString);
    if (childNode && childNode.text()) {
        string it = string(childNode.text().get());
        if (isValidIndexType(it)) {
            coreInfo->indexType = childNode.text().as_int();
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

    coreInfo->supportSwapInEditDistance = true; // by default it is true
    childNode = indexConfigNode.child(supportSwapInEditDistanceString);
    if (childNode && childNode.text()) {
        string qtmt = childNode.text().get();
        if (isValidBool(qtmt)) {
            coreInfo->supportSwapInEditDistance = childNode.text().as_bool();
        } else {
            parseError << "The provided supportSwapInEditDistance flag is not valid";
            configSuccess = false;
            return;
        }
    }

    coreInfo->enableWordPositionIndex = false; // by default it is false
    childNode = indexConfigNode.child(enablePositionIndexString);
    if (childNode && childNode.text()) {
        string configValue = childNode.text().get();
        if (isValidBooleanValue(configValue)) {
            coreInfo->enableWordPositionIndex = childNode.text().as_bool();
        } else {
            parseError << "enablePositionIndex should be either 0 or 1.\n";
            configSuccess = false;
            return;
        }
        if (coreInfo->enableWordPositionIndex) {
            Logger::info("turning on attribute based search because position index is enabled");
            coreInfo->supportAttributeBasedSearch = true;
        } // else leave supportAttributeBasedSearch set to previous value
    }
    coreInfo->enableCharOffsetIndex = false; // by default it is false
    childNode = indexConfigNode.child(enableCharOffsetIndexString);
    if (childNode && childNode.text()) {
    	string configValue = childNode.text().get();
    	if (isValidBooleanValue(configValue)) {
    		coreInfo->enableCharOffsetIndex = childNode.text().as_bool();
    	} else {
    		parseError << "enableCharOffsetIndex should be either 0 or 1.\n";
    		configSuccess = false;
    		return;
    	}
    	if (!coreInfo->enableWordPositionIndex && coreInfo->enableCharOffsetIndex) {
    		Logger::info("turning on attribute based search because position index is enabled");
    		coreInfo->supportAttributeBasedSearch = true;
    	} // else leave supportAttributeBasedSearch set to previous value
    }

    childNode = indexConfigNode.child(fieldBoostString);
    // splitting the field boost input and put them in boostsMap
    if (childNode && childNode.text()) {
        string boostString = string(childNode.text().get());
        boost::algorithm::trim(boostString);
        splitBoostFieldValues(boostString, boostsMap);
    }

    // recordBoostField is an optional field
    coreInfo->recordBoostFieldFlag = false;
    childNode = indexConfigNode.child(recordBoostFieldString);
    if (childNode && childNode.text()) {
        coreInfo->recordBoostFieldFlag = true;
        coreInfo->recordBoostField = string(childNode.text().get());
    }

    // queryTermBoost is an optional field
    coreInfo->queryTermBoost = 1; // By default it is 1
    childNode = indexConfigNode.child(defaultQueryTermBoostString);
    if (childNode && childNode.text()) {
        string qtb = childNode.text().get();
        if (isValidQueryTermBoost(qtb)) {
            coreInfo->queryTermBoost = childNode.text().as_uint();
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermBoost is not a (non-negative)number.";
            return;
        }
    }
}

void ConfigManager::parseMongoDb(const xml_node &mongoDbNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    xml_node childNode = mongoDbNode.child(hostString);
    if (childNode && childNode.text()) {
        coreInfo->mongoHost = string(childNode.text().get());
    } else {
        parseError << "mongo host is not set.\n";
        configSuccess = false;
        return;
    }

    childNode = mongoDbNode.child(portString);
    if (childNode && childNode.text()) {
        coreInfo->mongoPort = string(childNode.text().get());
        int value = atoi(coreInfo->mongoPort.c_str());
        if (value <= 0 || value > USHRT_MAX) {
            parseError << "mongoPort must be between 1 and " << USHRT_MAX;
            configSuccess = false;
            return;
        }
    } else {
        coreInfo->mongoPort = ""; // use default port
    }

    childNode = mongoDbNode.child(dbString);
    if (childNode && childNode.text()) {
        coreInfo->mongoDbName = string(childNode.text().get());
    } else {
        parseError << "mongo data base name is not set.\n";
        configSuccess = false;
        return;
    }

    childNode = mongoDbNode.child(collectionString);
    if (childNode && childNode.text()) {
        coreInfo->mongoCollection = string(childNode.text().get());
    } else {
        parseError << "mongo collection name is not set.\n";
        configSuccess = false;
        return;
    }

    childNode = mongoDbNode.child(listenerWaitTimeString);
    if (childNode && childNode.text()) {
        coreInfo->mongoListenerWaitTime = childNode.text().as_uint(1);
    } else {
        coreInfo->mongoListenerWaitTime = 1;
    }

    childNode = mongoDbNode.child(maxRetryOnFailureString);
    if (childNode && childNode.text()) {
        coreInfo->mongoListenerMaxRetryOnFailure = childNode.text().as_uint(3);
    } else {
        coreInfo->mongoListenerMaxRetryOnFailure = 3;
    }

    // For MongoDB as a data source , primary key must be "_id" which is a unique key generated
    // by MongoDB. It is important to set primary key to "_id" because oplog entries for inserts
    // and deletes in MongoDB can be identified by _id only.
    coreInfo->primaryKey = "_id";
}

void ConfigManager::parseQuery(const xml_node &queryNode,
                               CoreInfo_t *coreInfo,
                               bool &configSuccess,
                               std::stringstream &parseError,
                               std::stringstream &parseWarnings)
{
    // scoringExpressionString is an optional field
    coreInfo->scoringExpressionString = "1"; // By default it is 1
    xml_node childNode = queryNode.child(rankingAlgorithmString).child(recordScoreExpressionString);
    if (childNode && childNode.text()) {
        string exp = childNode.text().get();
        boost::algorithm::trim(exp);
        if (isValidRecordScoreExpession(exp)) {
            coreInfo->scoringExpressionString = exp;
        } else {
            configSuccess = false;
            parseError << "The expression provided for recordScoreExpression is not a valid.";
            return;
        }
    }

    // fuzzyMatchPenalty is an optional field
    coreInfo->fuzzyMatchPenalty = 1; // By default it is 1
    childNode = queryNode.child(fuzzyMatchPenaltyString);
    if (childNode && childNode.text()) {
        string qtsb = childNode.text().get();
        if (isValidFuzzyMatchPenalty(qtsb)) {
            coreInfo->fuzzyMatchPenalty = childNode.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The expression provided for fuzzyMatchPenalty is not a valid.";
            return;
        }
    }

    // queryTermSimilarityThreshold is an optional field
    //By default it is 0.5.
    coreInfo->queryTermSimilarityThreshold = 0.5;
    childNode = queryNode.child(queryTermSimilarityThresholdString);
    if (childNode && childNode.text()) {
        string qtsb = childNode.text().get();
        if (isValidQueryTermSimilarityThreshold(qtsb)) {
            coreInfo->queryTermSimilarityThreshold = childNode.text().as_float();
            if (coreInfo->queryTermSimilarityThreshold < 0 || coreInfo->queryTermSimilarityThreshold > 1 ){
                coreInfo->queryTermSimilarityThreshold = 0.5;
                parseError << "The value provided for queryTermSimilarityThreshold is not in [0,1].";
            }
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermSimilarityThreshold is not a valid.";
            return;
        }
    }

    // queryTermLengthBoost is an optional field
    coreInfo->queryTermLengthBoost = 0.5; // By default it is 0.5
    childNode = queryNode.child(queryTermLengthBoostString);
    if (childNode && childNode.text()) {
        string qtlb = childNode.text().get();
        if (isValidQueryTermLengthBoost(qtlb)) {
            coreInfo->queryTermLengthBoost = childNode.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The expression provided for queryTermLengthBoost is not a valid.";
            return;
        }
    }

    // prefixMatchPenalty is an optional field.
    coreInfo->prefixMatchPenalty = 0.95; // By default it is 0.5
    childNode = queryNode.child(prefixMatchPenaltyString);
    if (childNode && childNode.text()) {
        string pm = childNode.text().get();

        if (isValidPrefixMatch(pm)) {
            coreInfo->prefixMatchPenalty = childNode.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The value provided for prefixMatch is not a valid.";
            return;
        }
    }

    // cacheSize is an optional field
    coreInfo->cacheSizeInBytes = 50 * 1048576;
    childNode = queryNode.child(cacheSizeString);
    if (childNode && childNode.text()) {
        string cs = childNode.text().get();
        if (isValidCacheSize(cs)) {
            coreInfo->cacheSizeInBytes = childNode.text().as_uint();
        } else {
            parseError << "cache size provided is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // rows is an optional field
    coreInfo->resultsToRetrieve = 10; // by default it is 10
    childNode = queryNode.child(rowsString);
    if (childNode && childNode.text()) {
        string row = childNode.text().get();
        if (isValidRows(row)) {
            coreInfo->resultsToRetrieve = childNode.text().as_int();
        } else {
            parseError << "rows is not set correctly.\n";
            configSuccess = false;
            return;
        }
    }

    // fieldBasedSearch is an optional field
    if (coreInfo->enableWordPositionIndex == false &&
    		coreInfo->enableCharOffsetIndex == false ) {
        coreInfo->supportAttributeBasedSearch = false; // by default it is false
        childNode = queryNode.child(fieldBasedSearchString);
        if (childNode && childNode.text()) {
            string configValue = childNode.text().get();
            if (isValidBooleanValue(configValue)) {
                coreInfo->supportAttributeBasedSearch = childNode.text().as_bool();
            } else {
                parseError << "fieldBasedSearch is not set correctly.\n";
                configSuccess = false;
                return;
            }
        }
    } else {
        // attribute based search is enabled if positional index is enabled
        coreInfo->supportAttributeBasedSearch = true;
    }

    // queryTermFuzzyType is an optional field
    coreInfo->exactFuzzy = false; // by default it is false
    childNode = queryNode.child(queryTermFuzzyTypeString);
    if (childNode && childNode.text()) {
        string qtmt = childNode.text().get();
        if (isValidQueryTermFuzzyType(qtmt)) {
            coreInfo->exactFuzzy = childNode.text().as_bool();
        } else {
            parseError << "The queryTermFuzzyType that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    // queryTermPrefixType is an optional field
    coreInfo->queryTermPrefixType = false;
    childNode = queryNode.child(queryTermPrefixTypeString);
    if (childNode && childNode.text()) {
        string qt = childNode.text().get();
        if (isValidQueryTermPrefixType(qt)) {
            coreInfo->queryTermPrefixType = childNode.text().as_bool();
        } else {
            parseError << "The queryTerm that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    // responseFormat is an optional field
    coreInfo->searchResponseJsonFormat = 0; // by default it is 0
    childNode = queryNode.child(queryResponseWriterString).child(responseFormatString);
    if (childNode && childNode.text()) {
        string rf = childNode.text().get();
        if (isValidResponseFormat(rf)) {
            coreInfo->searchResponseJsonFormat = childNode.text().as_int();
        } else {
            parseError << "The provided responseFormat is not valid";
            configSuccess = false;
            return;
        }
    }

    coreInfo->fuzzyHighlightMarkerPre = defaultFuzzyPreTag;
    coreInfo->fuzzyHighlightMarkerPost = defaultFuzzyPostTag;
    coreInfo->exactHighlightMarkerPre = defaultExactPreTag;
    coreInfo->exactHighlightMarkerPost = defaultExactPostTag;
    coreInfo->highlightSnippetLen = 150;

    childNode = queryNode.child(highLighterString).child(snippetSize);
    if (childNode && childNode.text()) {
    	coreInfo->highlightSnippetLen = childNode.text().as_int();
    }
    childNode = queryNode.child(highLighterString).child(exactTagPre);
    if (childNode) {
    	string marker = childNode.attribute("value").value();
    	boost::algorithm::trim(marker);
    	if (marker.length() > 0){
    		coreInfo->exactHighlightMarkerPre = marker;
    	} else {
    		parseError << "The highlighter pre marker is an empty string. Using the default marker";
    		return;
    	}
    }
    childNode = queryNode.child(highLighterString).child(exactTagPost);
    if (childNode) {
    	string marker = childNode.attribute("value").value();
    	boost::algorithm::trim(marker);
    	if (marker.length() > 0){
    		coreInfo->exactHighlightMarkerPost = marker;
    	} else {
    		parseError << "The highlighter post marker is an empty string. Using the default marker";
    		return;
    	}
	}
    childNode = queryNode.child(highLighterString).child(fuzzyTagPre);
    if (childNode) {
    	string marker = childNode.attribute("value").value();
    	boost::algorithm::trim(marker);
    	if (marker.length() > 0){
    		coreInfo->fuzzyHighlightMarkerPre = marker;
    	} else {
    		parseError << "The highlighter pre marker is an empty string. Using the default marker";
    		return;
    	}
    }
    childNode = queryNode.child(highLighterString).child(fuzzyTagPost);
    if (childNode) {
    	string marker = childNode.attribute("value").value();
    	boost::algorithm::trim(marker);
    	if (marker.length() > 0){
    		coreInfo->fuzzyHighlightMarkerPost = marker;
    	} else {
    		parseError << "The highlighter post marker is an empty string. Using the default marker";
    		return;
    	}
	}

    // responseContent is an optional field
    coreInfo->searchResponseContent = (ResponseType)0; // by default it is 0
    childNode = queryNode.child(queryResponseWriterString).child(responseContentString);
    if (childNode) {
        string type = childNode.attribute(typeString).value();
        if (isValidResponseContentType(type)) {
            coreInfo->searchResponseContent = (ResponseType)childNode.attribute(typeString).as_int();
        } else {
            parseError << "The type provided for responseContent is not valid";
            configSuccess = false;
            return;
        }

        if (coreInfo->searchResponseContent == 2) {
            if (childNode.text()) {
                splitString(string(childNode.text().get()), ",", coreInfo->attributesToReturn);
            } else {
                parseError << "For specified response content type, return fields should be provided.";
                configSuccess = false;
                return;
            }
        }
    }

    // maxSearchThreads used to be contained in <query> so warn if we find it here
    childNode = queryNode.child(maxSearchThreadsString);
    if (childNode && childNode.text()) {
        Logger::warn("maxSearchThreads is no longer in <query>.  Move it under <config>");
    }
}

/*
 * Only called by parseMultipleCores().  This function is specific to parsing the <core> node defining
 * a single core (data source).  However, it doesn't do much itself.  It relies on parseDataFieldSettings() to
 * parse most of the values, including schema, because those specifications can occur under <config>
 * directly as well as under <core>.
 */
void ConfigManager::parseSingleCore(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    // <core name="core0"
	string tempUse = "";
    if (parentNode.attribute(nameString) && string(parentNode.attribute(nameString).value()).compare("") != 0) {
        coreInfo->name = parentNode.attribute(nameString).value();
    } else {
        parseError << "Core must have a name attribute";
        configSuccess = false;
        return;
    }

    // Solr compatability - dataDir can be an attribute: <core dataDir="core0/data"
    if (parentNode.attribute(dataDirString) && string(parentNode.attribute(dataDirString).value()).compare("") != 0) {
        coreInfo->dataDir = parentNode.attribute(dataDirString).value();
        coreInfo->indexPath = srch2Home + coreInfo->dataDir;
    }

    parseDataFieldSettings(parentNode, coreInfo, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }
}

// only called by parseDataConfiguration()
void ConfigManager::parseMultipleCores(const xml_node &coresNode, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    if (coresNode) {

        // <cores defaultCoreName = "foo">
        if (coresNode.attribute(defaultCoreNameString) && string(coresNode.attribute(defaultCoreNameString).value()).compare("") != 0) {
            defaultCoreName = coresNode.attribute(defaultCoreNameString).value();
            defaultCoreSetFlag = true;
        } else {
            parseWarnings << "Cores defaultCoreName not set <cores defaultCoreName=...>";
        }

        // parse zero or more individual core settings
        for (xml_node coreNode = coresNode.first_child(); coreNode; coreNode = coreNode.next_sibling()) {
            CoreInfo_t *newCore = new CoreInfo_t(this);
            parseSingleCore(coreNode, newCore, configSuccess, parseError, parseWarnings);
            if (configSuccess) {
                coreInfoMap[newCore->name] = newCore;
            } else {
                delete newCore;
                return;
            }
        }
    }
}

/*
 * parentNode is either <config> or <core>.  parseDataFieldSettings() is responsible for loading the settings
 * from either <config> or <core>, but only those settings that are common to both.
 * parseDataConfiguration() calls this with <config> and parseSingleCore() calls it with <core>.
 */
void ConfigManager::parseDataFieldSettings(const xml_node &parentNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    string tempUse = "";
    CoreConfigParseState_t coreParseState;
    // <config><dataDir>core0/data OR <core><dataDir>

    xml_node childNodeOfCores = parentNode.child(primaryShardTag);


    if(childNodeOfCores && childNodeOfCores.text()){
    	string temp = (childNodeOfCores.text().get());
    	trimSpacesFromValue(temp, primaryShardTag, parseWarnings);
    	coreInfo->numberOfPrimaryShards = (uint)atol(temp.c_str());
    }
    else{
    	coreInfo->numberOfPrimaryShards = DefaultNumberOfPrimaryShards;
        parseWarnings << "Number of primary shards is not defined. The engine will use the default value " << coreInfo->numberOfPrimaryShards << "\n";
    }

    childNodeOfCores = parentNode.child(primaryShardTag);
    xml_node primaryShardSibling = childNodeOfCores.next_sibling(primaryShardTag);
    if(primaryShardSibling){
        parseWarnings << "Duplicate definition of \"" << primaryShardTag << "\".  The engine will use the first value " << coreInfo->numberOfPrimaryShards << "\n";
    }

    childNodeOfCores = parentNode.child(replicaShardTag);

    if(childNodeOfCores && childNodeOfCores.text()){
    	string temp = (childNodeOfCores.text().get());
    	trimSpacesFromValue(temp, replicaShardTag, parseWarnings);
    	coreInfo->numberOfReplicas = (uint)atol(temp.c_str());
    }
    else{
    	coreInfo->numberOfReplicas = DefaultNumberOfReplicas;
    	parseWarnings << "Number of replicas is not defined. The engine will use the default value " << DefaultNumberOfReplicas << "\n";
    }

    xml_node replicaSibling = childNodeOfCores.next_sibling(replicaShardTag);
    if(replicaSibling){
    	parseWarnings << "Duplicate definition of \"" << replicaShardTag << "\".  The engine will use the first value " << coreInfo->numberOfReplicas << "\n";
    }

    xml_node childNode = parentNode.child(dataDirString);
    if (childNode && childNode.text()) {
        coreInfo->dataDir = string(childNode.text().get());
        coreInfo->indexPath = srch2Home + coreInfo->dataDir;
    }

    if (coreInfo->dataDir.length() == 0) {
        parseWarnings << "Core " << coreInfo->name.c_str() << " has null dataDir\n";
    }

    childNode = parentNode.child(dataSourceTypeString);
    if (childNode && childNode.text()) {
        int dataSourceValue = childNode.text().as_int(DATA_SOURCE_JSON_FILE);
        switch(dataSourceValue) {
        case 0:
            coreInfo->dataSourceType = DATA_SOURCE_NOT_SPECIFIED;
            break;
        case 1:
            coreInfo->dataSourceType = DATA_SOURCE_JSON_FILE;
            break;
        case 2:
            coreInfo->dataSourceType = DATA_SOURCE_MONGO_DB;
            break;
        default:
            // if user forgets to specify this option, we will assume data source is JSON file
            coreInfo->dataSourceType = DATA_SOURCE_JSON_FILE;
            break;
        }
    } else {
        coreInfo->dataSourceType = DATA_SOURCE_JSON_FILE;
    }

    if (coreInfo->dataSourceType == DATA_SOURCE_JSON_FILE) {
        // dataFile is a required field only if JSON file is specified as data source.
        childNode = parentNode.child(dataFileString);
        if (childNode && childNode.text()) { // checks if the config/dataFile has any text in it or not
            tempUse = string(childNode.text().get());
            trimSpacesFromValue(tempUse, dataFileString, parseWarnings);
            coreInfo->dataFilePath = srch2Home + string("") + coreInfo->getName() + string("/") + tempUse;
        } else {
            parseError << (coreInfo->name.compare("") != 0 ? coreInfo->name : "default") <<
                " core path to the data file is not set. "
                "You should set it as <dataFile>path/to/data/file</dataFile> in the config file.\n";
            configSuccess = false;
            return;
        }
    }

    // map of port type enums to strings to simplify code
    struct portNameMap_t {
        enum PortType_t portType;
        const char *portName;
    };
    static portNameMap_t portNameMap[] = {
        { SearchPort, searchPortString },
        { SuggestPort, suggestPortString },
        { InfoPort, infoPortString },
        { DocsPort, docsPortString },
        { UpdatePort, updatePortString },
        { SavePort, savePortString },
        { ExportPort, exportPortString },
        { ResetLoggerPort, resetLoggerPortString },
        { EndOfPortType, NULL }
    };

    for (unsigned int i = 0; portNameMap[i].portName != NULL; i++) {
        childNode = parentNode.child(portNameMap[i].portName);
        if (childNode && childNode.text()) { // checks if the config/port has any text in it or not
            int portValue = childNode.text().as_int();
            if (portValue <= 0 || portValue > USHRT_MAX) {
                parseError << portNameMap[i].portName << " must be between 1 and " << USHRT_MAX;
                configSuccess = false;
                return;
            }
            coreInfo->setPort(portNameMap[i].portType, portValue);
        }
    }

    if (coreInfo->dataSourceType == DATA_SOURCE_MONGO_DB) {
        childNode = parentNode.child(mongoDbString);
        parseMongoDb(childNode, coreInfo, configSuccess, parseError, parseWarnings);
        if (configSuccess == false) {
            return;
        }
    }

    // uniqueKey is required
    childNode = parentNode.child(schemaString).child(uniqueKeyString);
    if (childNode && childNode.text()) {
        coreInfo->primaryKey = string(childNode.text().get());
    } else {
        parseError << (coreInfo->name.compare("") != 0 ? coreInfo->name : "default") <<
            " core uniqueKey (primary key) is not set.\n";
        configSuccess = false;
        return;
    }

    coreInfo->allowedRecordTokenizerCharacters = "";
    coreInfo->attributeToSort = 0;

    // set default number of suggestions because we don't have any config options for this yet
    coreInfo->defaultNumberOfSuggestions = 5;

    xml_node indexConfigNode = parentNode.child(indexConfigString);
    map<string, unsigned> boostsMap;
    parseIndexConfig(indexConfigNode, coreInfo, boostsMap, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }

    childNode = parentNode.child(queryString);
    if (childNode) {
        parseQuery(childNode, coreInfo, configSuccess, parseError, parseWarnings);
        if (configSuccess == false) {
            return;
        }
    }

    // <schema>
    childNode = parentNode.child(schemaString);
    if (childNode) {
        parseSchema(childNode, &coreParseState, coreInfo, configSuccess, parseError, parseWarnings);
        if (configSuccess == false) {
            return;
        }
    }

    if (coreInfo->indexType == 1) {
        // If index type is 1, it means it is geo. So both latitude and longitude should be provided
        if (!(coreParseState.hasLatitude && coreParseState.hasLongitude)) {
            parseError << "Both Geo related attributes should set together. Currently only one of them is set.\n";
            configSuccess = false;
            return;
        }
        coreInfo->searchType = 2; // GEO search
    } else if (coreInfo->indexType == 0){
        coreInfo->searchType = 0;
        coreInfo->fieldLongitude = "IGNORE"; // IN URL parser these fields are being checked with "IGNORE". We should get rid of them.
        coreInfo->fieldLatitude = "IGNORE"; // IN URL parser these fields are being checked with "IGNORE". We should get rid of them.

        childNode = parentNode.child(queryString).child(searcherTypeString);
        if (childNode && childNode.text()) {
            string st = childNode.text().get();
            if (isValidSearcherType(st)) {
                coreInfo->searchType = childNode.text().as_int();
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
    for (int i = 0; i < coreParseState.searchableFieldsVector.size(); i++) {
        if (boostsMap.find(coreParseState.searchableFieldsVector[i]) == boostsMap.end()) {
            coreInfo->searchableAttributesInfo[coreParseState.searchableFieldsVector[i]] =
                SearchableAttributeInfoContainer(coreParseState.searchableFieldsVector[i] ,
                                                 coreParseState.searchableAttributesRequiredFlagVector[i] ,
                                                 coreParseState.searchableAttributesDefaultVector[i] ,
                                                 0 , 1 , coreParseState.searchableAttributesIsMultiValued[i],
                                                 coreParseState.highlight[i]);
        } else {
            coreInfo->searchableAttributesInfo[coreParseState.searchableFieldsVector[i]] =
                SearchableAttributeInfoContainer(coreParseState.searchableFieldsVector[i] ,
                                                 coreParseState.searchableAttributesRequiredFlagVector[i] ,
                                                 coreParseState.searchableAttributesDefaultVector[i] ,
                                                 0 , boostsMap[coreParseState.searchableFieldsVector[i]] ,
                                                 coreParseState.searchableAttributesIsMultiValued[i],
                                                 coreParseState.highlight[i]);
        }
    }

    // give each searchable attribute an id based on the order in the info map
    // should be consistent with the id in the schema
    map<string, SearchableAttributeInfoContainer>::iterator searchableAttributeIter = coreInfo->searchableAttributesInfo.begin();
    for(unsigned idIter = 0; searchableAttributeIter != coreInfo->searchableAttributesInfo.end() ; ++searchableAttributeIter, ++idIter){
        searchableAttributeIter->second.offset = idIter;
    }

    // checks the validity of the boost fields in boostsMap
    // must occur after parseIndexConfig() AND parseSchema()
    if (!isValidBoostFields(coreInfo, boostsMap)) {
        configSuccess = false;
        parseError << "In core " << coreInfo->name << ": Fields that are provided in the boostField do not match with the defined fields.";
    
        return;
    }

    // checks the validity of the boost values in boostsMap
    if (!isValidBoostFieldValues(boostsMap)) {
        configSuccess = false;
        parseError << "Boost values that are provided in the boostField are not in the range [1 to 100].";
        return;
    }

    // <config><updateHandler> OR <core><updateHandler>
    childNode = parentNode.child(updateHandlerString);
    if (childNode) {
        parseUpdateHandler(childNode, coreInfo, configSuccess, parseError, parseWarnings);
        if (configSuccess == false) {
            return;
        }
    }
}

void ConfigManager::parseDataConfiguration(const xml_node &configNode,
                                           bool &configSuccess,
                                           std::stringstream &parseError,
                                           std::stringstream &parseWarnings)
{
    CoreInfo_t *coreInfo = NULL;

    // check for data source coreInfo outside of <cores>
    if (configNode.child(dataDirString) || configNode.child(dataSourceTypeString) ||
        configNode.child(dataFileString)) {

        // create a default core for coreInfo outside of <cores>
        bool created = false;
        if (coreInfoMap.find(getDefaultCoreName()) == coreInfoMap.end()) {
            coreInfo = new CoreInfo_t(this);
            created= true;
        } else {
            coreInfo = coreInfoMap[getDefaultCoreName()];
        }

        parseDataFieldSettings(configNode, coreInfo, configSuccess, parseError, parseWarnings);
        if (configSuccess == false) {
            if (created) {
                delete coreInfo;
            }
            return;
        }
    }

    // if no errors, add coreInfo to map
    if (coreInfo != NULL) {
        coreInfo->name = getDefaultCoreName();
        coreInfoMap[coreInfo->name] = coreInfo;
    }

    // maxSearchThreads is an optional field
    numberOfThreads = 1; // by default it is 1
    xml_node childNode = configNode.child(maxSearchThreadsString);
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

    // <cores>
    childNode = configNode.child(multipleCoresString);
    if (childNode) {
        parseMultipleCores(childNode, configSuccess, parseError, parseWarnings);
        if (configSuccess == false) {
            return;
        }
    }
}

/*
 * Parse a <schema> node, either directly under the root <config> or under <core>
 */
void ConfigManager::parseSchema(const xml_node &schemaNode, CoreConfigParseState_t *coreParseState, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
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
    coreInfo->isPrimSearchable = 0;

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
                 *        => attribute is both searchable and used for post processing
                 * } else {
                 *       if(searchable == true) => attribute is searchable
                 *       else => attribute is not searchable
                 *
                 *       if(refining == true) => attribute is used for post-processing
                 *       else => attribute is not used for post processing
                 * }
                 */
                coreParseState->highlight.push_back(false);
                if(string(field.attribute(highLightString).value()).compare("") != 0){
                	tempUse = string(field.attribute(highLightString).value());
                	if (isValidBool(tempUse)){
                		if(field.attribute(indexedString).as_bool()){
                			coreParseState->highlight[coreParseState->highlight.size() - 1] = true;
                		}
                	}
                }

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
                if(string(field.attribute(nameString).value()).compare(coreInfo->primaryKey) == 0){
                    if(isSearchable){
                        coreInfo->isPrimSearchable = 1;
                        coreParseState->searchableFieldsVector.push_back(string(field.attribute(nameString).value()));
                        // there is no need for default value for primary key
                        coreParseState->searchableAttributesDefaultVector.push_back("");
                        // primary key is always required.
                        coreParseState->searchableAttributesRequiredFlagVector.push_back(true);
                    }

                    if(isRefining){
                        RefiningFieldsVector.push_back(coreInfo->primaryKey);
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
                        coreParseState->searchableFieldsVector.push_back(string(field.attribute(nameString).value()));
                        // Checking the validity of field type
                        tempUse = string(field.attribute(typeString).value());
                        if (isValidFieldType(tempUse , true)) {
                            coreParseState->searchableFieldTypesVector.push_back(tempUse);
                        } else {
                            parseError << "Config File Error: " << tempUse << " is not a valid field type for searchable fields.\n";
                            parseError << " Note: searchable fields only accept 'text' type. Setting 'searchable' or 'indexed' to true makes a field searchable.\n";
                            configSuccess = false;
                            return;
                        }

                        if (string(field.attribute(defaultString).value()).compare("") != 0){
                            coreParseState->searchableAttributesDefaultVector.push_back(string(field.attribute(defaultString).value()));
                        }else{
                            coreParseState->searchableAttributesDefaultVector.push_back("");
                        }

                        tempUse = string(field.attribute(requiredString).value());
                        if (string(field.attribute(requiredString).value()).compare("") != 0 && isValidBool(tempUse)){
                            coreParseState->searchableAttributesRequiredFlagVector.push_back(field.attribute(requiredString).as_bool());
                        }else{
                            coreParseState->searchableAttributesRequiredFlagVector.push_back(false);
                        }
                        coreParseState->searchableAttributesIsMultiValued.push_back(isMultiValued);
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
                        coreParseState->hasLatitude = true;
                        coreInfo->fieldLatitude = string(field.attribute(nameString).value());
                    }
                    if (string(field.attribute(typeString).value()).compare(locationLongitudeString) == 0) {
                        coreParseState->hasLongitude = true;
                        coreInfo->fieldLongitude = string(field.attribute(nameString).value());
                    }

                } else { // if one of the values of name, type or indexed is empty
                    parseError << "For the searchable fields, "
                               << "providing values for 'name' and 'type' is required\n ";
                    configSuccess = false;
                    return;
                }
            } else {
                parseWarnings << "Unexpected XML node " << field.name() << " within <fields>";
            }
        }
    } else { // No searchable fields provided.
        parseError << "No fields are provided.\n";
        configSuccess = false;
        return;
    }

    // Checking if there is any field or not.
    if (coreParseState->searchableFieldsVector.size() == 0) {
        parseError << "No searchable fields are provided.\n";
        configSuccess = false;
        return;
    }

    if(RefiningFieldsVector.size() != 0){
        for (unsigned iter = 0; iter < RefiningFieldsVector.size(); iter++) {
            coreInfo->refiningAttributesInfo[RefiningFieldsVector[iter]] =
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
    coreInfo->facetEnabled = false; // by default it is false
    xml_node childNode = schemaNode.child(facetEnabledString);
    if (childNode && childNode.text()) {
        string qtmt = childNode.text().get();
        if (isValidBool(qtmt)) {
            coreInfo->facetEnabled = childNode.text().as_bool();
        } else {
            parseError << "The facetEnabled that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    /*
     * <facetFields>  in config.xml file
     */

    if(coreInfo->facetEnabled){
        childNode = schemaNode.child(facetFieldsString);
        if (childNode) {
            for (xml_node field = childNode.first_child(); field; field = field.next_sibling()) {
                if (string(field.name()).compare(facetFieldString) == 0) {
                    if (string(field.attribute(nameString).value()).compare("") != 0
                        && string(field.attribute(facetTypeString).value()).compare("") != 0){

                        // insert the name of the facet
                        coreInfo->facetAttributes.push_back(string(field.attribute(nameString).value()));
                        // insert the type of the facet
                        tempUse = string(field.attribute(facetTypeString).value());
                        int facetType = parseFacetType(tempUse);
                        if(facetType == 0){ // categorical
                            coreInfo->facetTypes.push_back(facetType);
                            // insert place holders for start,end and gap
                            coreInfo->facetStarts.push_back("");
                            coreInfo->facetEnds.push_back("");
                            coreInfo->facetGaps.push_back("");
                        }else if(facetType == 1){ // range
                            coreInfo->facetTypes.push_back(facetType);
                            // insert start
                            string startTextValue = string(field.attribute(facetStartString).value());
                            string facetAttributeString = string(field.attribute(nameString).value());
                            srch2::instantsearch::FilterType facetAttributeType ;
                            if(coreInfo->refiningAttributesInfo.find(facetAttributeString) != coreInfo->refiningAttributesInfo.end()){
                                facetAttributeType = coreInfo->refiningAttributesInfo.find(facetAttributeString)->second.attributeType;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                coreInfo->facetEnabled = false;
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
                                    coreInfo->facetEnabled = false;
                                    break;
                                }
                            }
                            coreInfo->facetStarts.push_back(startTextValue);

                            // insert end
                            string endTextValue = string(field.attribute(facetEndString).value());
                            if(coreInfo->refiningAttributesInfo.find(facetAttributeString) != coreInfo->refiningAttributesInfo.end()){
                                facetAttributeType = coreInfo->refiningAttributesInfo.find(facetAttributeString)->second.attributeType;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                coreInfo->facetEnabled = false;
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
                                    coreInfo->facetEnabled = false;
                                    break;
                                }
                            }
                            coreInfo->facetEnds.push_back(endTextValue);

                            // insert gap
                            string gapTextValue = string(field.attribute(facetGapString).value());
                            if(coreInfo->refiningAttributesInfo.find(facetAttributeString) != coreInfo->refiningAttributesInfo.end()){
                                facetAttributeType = coreInfo->refiningAttributesInfo.find(facetAttributeString)->second.attributeType;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                coreInfo->facetEnabled = false;
                                break;
                            }
                            if(facetAttributeType == srch2is::ATTRIBUTE_TYPE_TIME){
                                if(!srch2is::DateAndTimeHandler::verifyDateTimeString(gapTextValue , srch2is::DateTimeTypeDurationOfTime) ){
                                    parseError << "Facet attribute end value is in wrong format.Facet disabled.\n";
                                    coreInfo->facetEnabled = false;
                                    break;
                                }
                            }
                            coreInfo->facetGaps.push_back(gapTextValue);
                        }else{
                            parseError << "Facet type is not recognized. Facet disabled.";
                            coreInfo->facetEnabled = false;
                            break;
                        }

                    }
                }
            }
        }
    }

    if(! coreInfo->facetEnabled){
        coreInfo->facetAttributes.clear();
        coreInfo->facetTypes.clear();
        coreInfo->facetStarts.clear();
        coreInfo->facetEnds.clear();
        coreInfo->facetGaps.clear();
    }

    // Analyzer flags : Everything is disabled by default.
    coreInfo->stemmerFlag = false;
    coreInfo->stemmerFile = "";
    coreInfo->stopFilterFilePath = "";
    coreInfo->synonymFilterFilePath = "";
    coreInfo->protectedWordsFilePath = "";
    coreInfo->synonymKeepOrigFlag = false;

    childNode = schemaNode.child(typesString);
    if (childNode) {        // Checks if <schema><types> exists or not
        for (xml_node fieldType = childNode.first_child(); fieldType; fieldType = fieldType.next_sibling()) { // Going on the children
            if ((string(fieldType.name()).compare(fieldTypeString) == 0)) { // Finds the fieldTypes
                if (string(fieldType.attribute(nameString).value()).compare(textEnString) == 0) {
                    // Checking if the values are empty or not
                    xml_node childNodeTemp = fieldType.child(analyzerString); // looks for analyzer
                    for (xml_node field = childNodeTemp.first_child(); field; field = field.next_sibling()) {
                        if (string(field.name()).compare(filterString) == 0) {
                            if (string(field.attribute(nameString).value()).compare(porterStemFilterString) == 0) { // STEMMER FILTER
                                if (string(field.attribute(dictionaryString).value()).compare("") != 0) { // the dictionary for porter stemmer is set.
                                    coreInfo->stemmerFlag = true;
                                    tempUse = string(field.attribute(dictionaryString).value());
                                    trimSpacesFromValue(tempUse, porterStemFilterString, parseWarnings);
                                    coreInfo->stemmerFile = boost::filesystem::path(this->srch2Home + tempUse).normalize().string();
                                }
                            } else if (string(field.attribute(nameString).value()).compare(stopFilterString) == 0) { // STOP FILTER
                                if (string(field.attribute(wordsString).value()).compare("") != 0) { // the words file for stop filter is set.
                                    tempUse = string(field.attribute(wordsString).value());
                                    trimSpacesFromValue(tempUse, stopFilterString, parseWarnings);
                                    coreInfo->stopFilterFilePath = boost::filesystem::path(srch2Home + tempUse).normalize().string();
                                }
                            } /*else if (string(field.attribute(nameString).value()).compare(SynonymFilterString) == 0) {
                                if (string(field.attribute(synonymsString).value()).compare("") != 0) { // the dictionary file for synonyms is set
                                this->synonymFilterFilePath = boost::filesystem::path(this->srch2Home).normalize().string();
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
                                if (string(field.attribute(wordsString).value()).compare("") != 0) { // the file for protected words filter is set.
                                    tempUse = string(field.attribute(wordsString).value());
                                    trimSpacesFromValue(tempUse, protectedWordFilterString, parseWarnings);
                                    coreInfo->protectedWordsFilePath = boost::filesystem::path(srch2Home + tempUse).normalize().string();
                                }
                            }
                        } else if (string(field.name()).compare(allowedRecordSpecialCharactersString) == 0) {
                            CharSet charTyper;
                            string in = field.text().get(), out; // TODO: Using type string NOT multi-lingual?

                            // validate allowed characters
                            for (std::string::iterator iterator = in.begin(); iterator != in.end(); iterator++) {
                                switch (charTyper.getCharacterType(*iterator)) {
                                case CharSet::DELIMITER_TYPE:
                                    out += *iterator;
                                    break;
                                case CharSet::LATIN_TYPE:
                                case CharSet::BOPOMOFO_TYPE:
                                case CharSet::HANZI_TYPE:
                                    Logger::warn("%s character %c already included in terms, ignored", allowedRecordSpecialCharactersString, *iterator);
                                    break;
                                case CharSet::WHITESPACE:
                                    Logger::warn("%s character %c is whitespace and cannot be treated as part of a term, ignored", allowedRecordSpecialCharactersString, *iterator);
                                    break;
                                default:
                                    Logger::warn("%s character %c of unexpected type %d, ignored", allowedRecordSpecialCharactersString, *iterator, static_cast<int> (charTyper.getCharacterType(*iterator)));
                                    break;
                                }
                            }

                            coreInfo->allowedRecordTokenizerCharacters = out;
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

void ConfigManager::parseUpdateHandler(const xml_node &updateHandlerNode, CoreInfo_t *coreInfo, bool &configSuccess, std::stringstream &parseError, std::stringstream &parseWarnings)
{
    string tempUse = "";

    xml_node childNode = updateHandlerNode.child(maxDocsString);
    bool mdflag = false;
    if (childNode && childNode.text()) {
        string md = childNode.text().get();
        if (this->isValidMaxDoc(md)) {
            coreInfo->documentLimit = childNode.text().as_uint();
            mdflag = true;
        }
    }
    if (!mdflag) {
        parseError << "MaxDoc is not set correctly\n";
        configSuccess = false;
        return;
    }

    coreInfo->memoryLimit = 100000;
    bool mmflag = false;
    childNode = updateHandlerNode.child(maxMemoryString);
    if (childNode && childNode.text()) {
        string mm = childNode.text().get();
        if (this->isValidMaxMemory(mm)) {
            coreInfo->memoryLimit = childNode.text().as_uint();
            mmflag = true;
        }
    }
    if (!mmflag) {
        parseError << "MaxDoc is not set correctly\n";
        configSuccess = false;
        return;
    }

    // mergeEveryNSeconds
    childNode = updateHandlerNode.child(mergePolicyString).child(mergeEveryNSecondsString);
    bool mensflag = false;
    if (childNode && childNode.text()) {
        string mens = childNode.text().get();
        if (this->isValidMergeEveryNSeconds(mens)) {
            coreInfo->mergeEveryNSeconds = childNode.text().as_uint();
            mensflag = true;
        }
    }
    if (!mensflag) {
        parseError << "mergeEveryNSeconds is not set.\n";
        configSuccess = false;
        return;
    }

    // mergeEveryMWrites
    childNode = updateHandlerNode.child(mergePolicyString).child(mergeEveryMWritesString);
    bool memwflag = false;
    if (childNode && childNode.text()) {
        string memw = childNode.text().get();

        if (this->isValidMergeEveryMWrites(memw)) {
            coreInfo->mergeEveryMWrites = childNode.text().as_uint();
            memwflag = true;
        }
    }
    if (!memwflag) {
        parseError << "mergeEveryMWrites is not set.\n";
        configSuccess = false;
        return;
    }

    // set default value for updateHistogramEveryPSeconds and updateHistogramEveryQWrites because there
    // is no option in xml for this one yet
    float updateHistogramWorkRatioOverTime = 0.1; // 10 percent of background thread process is spent for updating histogram
    coreInfo->updateHistogramEveryPMerges = (unsigned)
        ( 1.0 / updateHistogramWorkRatioOverTime) ; // updateHistogramEvery 10 Merges
    coreInfo->updateHistogramEveryQWrites =
        (unsigned)((coreInfo->mergeEveryMWrites * 1.0 ) / updateHistogramWorkRatioOverTime); // 10000 for mergeEvery 1000 Writes

    // TODO - logging per core
    // logLevel is required
    this->loglevel = Logger::SRCH2_LOG_INFO;
    childNode = updateHandlerNode.child(updateLogString).child(logLevelString);
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
    childNode = updateHandlerNode.child(updateLogString).child(accessLogFileString);
    if (childNode && childNode.text()) {
        tempUse = string(childNode.text().get());
        trimSpacesFromValue(tempUse, updateLogString, parseWarnings);
        this->httpServerAccessLogFile = this->srch2Home + "/" + coreInfo->getName() + "/" + tempUse;
    } else {
        parseError << "httpServerAccessLogFile is not set.\n";
        configSuccess = false;
        return;
    }
}

bool ConfigManager::isNumber(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void ConfigManager::parse(const pugi::xml_document& configDoc,
                          bool &configSuccess,
                          std::stringstream &parseError,
                          std::stringstream &parseWarnings)
{
    string tempUse = ""; // This is just for temporary use.

    int flag_cluster = 0;
    CoreInfo_t *defaultCoreInfo = NULL;

    xml_node configNode = configDoc.child(configString);

    xml_node discoveryNode = configNode.child(discoveryNodeTag);
    if(discoveryNode){
        xml_node pingInterval = discoveryNode.child(pingIntervalTag);
        if(pingInterval && pingInterval.text()){
           tempUse = string(pingInterval.text().get());
           trimSpacesFromValue(tempUse, "pingInterval", parseWarnings);
           if(isNumber(tempUse))
               discovery.setPingInterval((uint)atol(tempUse.c_str()));
           else{
        	   parseWarnings<<"Ping interval specified is not valid, engine will use the default value 1";
           }
        }

        xml_node pingTimeout = discoveryNode.child(pingTimeoutTag);
        if(pingTimeout && pingTimeout.text()){
            tempUse = string(pingTimeout.text().get());
            trimSpacesFromValue(tempUse, "pingTimeout", parseWarnings);
            if(isNumber(tempUse))
                discovery.setPingTimeout((uint)atol(tempUse.c_str()));
            else{
         	   parseWarnings<<"Ping timeout specified is not valid, engine will use the default value 1";
            }
        }

        xml_node retryCount = discoveryNode.child(retryCountTag);
        if(retryCount && retryCount.text()){
            tempUse = string(retryCount.text().get());
            trimSpacesFromValue(tempUse, "retryCount", parseWarnings);
            if(isNumber(tempUse))
                discovery.setRetryCount((uint)atol(tempUse.c_str()));
            else{
                parseWarnings<<"Retry count specified is not valid, engine will use the default value 1";
            }
         }
    }

    xml_node clusterName = configNode.child(clusterNameTag);
    if (clusterName && clusterName.text()) {
    	tempUse = string(clusterName.text().get());
    	trimSpacesFromValue(tempUse, clusterNameTag, parseWarnings);
    	cluster.setClusterName(tempUse);
    }else{
    	cluster.setClusterName(string(DefaultClusterName));
    	parseWarnings << "Cluster name is not specified. Engine will use the default value " << cluster.getClusterName() << "\n";
    }

    xml_node clusterNameSibling = clusterName.next_sibling(clusterNameTag);
    if (clusterNameSibling && clusterNameSibling.text()){
          parseWarnings << "Duplicate definition of \"" << clusterNameTag << "\".  The engine will use the first value: " << cluster.getClusterName() << "\n";    }

    tempUse = "";


    std::vector<Node>* nodes = cluster.getNodes();

    xml_node nodeTag = configNode.child("node");
    if (nodeTag)
      ConfigManager::parseNode(nodes, nodeTag, parseWarnings);

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

    // check if data source exists at the top level
    xml_node topDataFileNode = childNode.child(dataFileString);
    if (topDataFileNode) {
        // create a default core for settings outside of <cores>
        if (coreInfoMap.find(getDefaultCoreName()) == coreInfoMap.end()) {
            defaultCoreInfo = new CoreInfo_t(this);
            defaultCoreInfo->name = getDefaultCoreName();
            coreInfoMap[defaultCoreInfo->name] = defaultCoreInfo;
        } else {
            defaultCoreInfo = coreInfoMap[getDefaultCoreName()];
        }
    }

    // parse all data source settings - no core (default core) and multiples core handled
    // requires indexType to have been loaded by parseIndexConfig()
    parseDataConfiguration(configNode, configSuccess, parseError, parseWarnings);
    if (configSuccess == false) {
        return;
    }

    defaultCoreInfo = coreInfoMap[getDefaultCoreName()];
    if (defaultCoreInfo == NULL) {
        parseError << "Default core " << getDefaultCoreName() << " not found\n";
        configSuccess = false;
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
        int value = atoi(httpServerListeningPort.c_str());
        if (value <= 0 || value > USHRT_MAX) {
            parseError << listeningPortString << " must be between 1 and " << USHRT_MAX;
            configSuccess = false;
            return;
        }
    } else {
        parseError << "listeningPort is not set.\n";
        configSuccess = false;
        return;
    }

    if (defaultCoreInfo->supportAttributeBasedSearch && defaultCoreInfo->searchableAttributesInfo.size() > 31) {
        parseError
            << "To support attribute-based search, the number of searchable attributes cannot be bigger than 31.\n";
        configSuccess = false;
        return;
    }

    // TODO - move to individual cores?
    this->ordering = 0;

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


//TODO: Pass by referencem, space after =
void ConfigManager::parseNode(std::vector<Node>* nodes, xml_node& nodeTag, std::stringstream &parseWarnings) {

    for (xml_node nodeTemp = nodeTag; nodeTemp; nodeTemp = nodeTemp.next_sibling("node")) {

    	bool nameDefined = false, portDefined = false, ipAddressDefined = false;
    	bool thisIsMeDefined = false, masterDefined = false, nodeDataDefined = false;
    	bool dataDirDefined = false, homeDefined = false;
        std::string ipAddress = "", dataDir = "", nodeName = "", nodeHome = "";
		unsigned nodeId = 0, portNumber = 0, numOfThreads = 0;
		bool nodeMaster, nodeData, thisIsMe;

		for (xml_node childNode = nodeTemp.first_child(); childNode; childNode = childNode.next_sibling()) {

			if (childNode && childNode.text()) {
				std::string name = (string) childNode.name();

				if (name.compare(nodeNameTag) == 0) {
					if(nameDefined == false){
						nodeName = string(childNode.text().get());
						trimSpacesFromValue(nodeName, nodeNameTag, parseWarnings);
						nameDefined = true;
					}
					else{
				        parseWarnings << "Duplicate definition of \"" << nodeNameTag << "\".  The engine will use the first value: " << nodeName << "\n";
					}
				}
				if (name.compare(nodeListeningHostNameTag) == 0) {
					if(ipAddressDefined == false){
						ipAddress = string(childNode.text().get());
						trimSpacesFromValue(ipAddress, nodeListeningHostNameTag, parseWarnings);
						ipAddressDefined = true;
					}
					else{
				        parseWarnings << "Duplicate definition of \"" << nodeListeningHostNameTag << "\".  The engine will use the first value: " << ipAddress << "\n";
					}

				}
				if (name.compare(nodeListeningPortTag) == 0) {
					if(portDefined == false){
						string portNo;
						portNo = string(childNode.text().get());
						trimSpacesFromValue(portNo, nodeListeningPortTag, parseWarnings);
						portNumber = (uint)atol(portNo.c_str());
						portDefined = true;
					}
					else{
				        parseWarnings << "Duplicate definition of \"" << nodeListeningPortTag << "\".  The engine will use the first value: " << portNumber << "\n";
					}
				}
				if (name.compare(nodeCurrentTag) == 0) {
					if(thisIsMeDefined == false){
						string temp = (childNode.text().get());
						trimSpacesFromValue(temp, nodeCurrentTag, parseWarnings);
						if(temp.compare("true") == 0){
							thisIsMe = true;
						}
						else if(temp.compare("false") == 0){
							thisIsMe = false;
						}
						else{
							thisIsMe = true;
							parseWarnings << "Invalid value for " << nodeCurrentTag << "; Using the default value true \n";
						}
						thisIsMeDefined = true;
					}

					else{
					    parseWarnings << "Duplicate definition of \"" << nodeCurrentTag << "\".  The engine will use the first value: ";
						if(thisIsMe)
							parseWarnings << "true "<< "\n";
						else
							parseWarnings << "false "<< "\n";
					}

				}
				if (name.compare(nodeMasterTag) == 0) {
					if(masterDefined == false){
						string temp = (childNode.text().get());
						trimSpacesFromValue(temp, nodeMasterTag, parseWarnings);
						if(temp.compare("true") == 0){
							nodeMaster = true;
						}
						else if(temp.compare("false") == 0){
							nodeMaster = false;
						}
						else{
							nodeMaster = true;
							parseWarnings << "Invalid value for " << nodeMasterTag << "; Using the default value true \n";
						}
						masterDefined = true;
					}
					else{
					    parseWarnings << "Duplicate definition of \"" << nodeMasterTag << "\".  The engine will use the first value: ";
						if(nodeMaster)
							parseWarnings << "true "<< "\n";
						else
							parseWarnings << "false "<< "\n";
						}
					}

				if (name.compare(nodeDataTag) == 0) {
					if(nodeDataDefined == false){
						string temp = (childNode.text().get());
						trimSpacesFromValue(temp, nodeDataTag, parseWarnings);
						if(temp.compare("true") == 0){
							nodeData = true;
						}
						else if(temp.compare("false") == 0){
							nodeData = false;
						}
						else{
							nodeData = true;
							parseWarnings << "Invalid value for " << nodeDataTag << "; Using the default value true \n";
						}
						nodeDataDefined = true;
						}
						else{
							parseWarnings << "Duplicate definition of \"" << nodeDataTag << "\".  The engine will use the first value: ";
							if(nodeData)
								parseWarnings << "true "<< "\n";
							else
								parseWarnings << "false "<< "\n";
							}
				}

				if (name.compare(nodeDataDirTag) == 0) {
					if(dataDirDefined == false){
						dataDir = string(childNode.text().get());
						trimSpacesFromValue(dataDir, nodeDataDirTag, parseWarnings);
						dataDirDefined = true;
					}
					else{
						parseWarnings << "Duplicate definition of \"" << nodeDataDirTag << "\".  The engine will use the first value: " << dataDir << "\n";
					}
				}
				if(name.compare(nodeHomeTag) == 0){
					if(homeDefined == false){
						nodeHome = string(childNode.text().get());
						trimSpacesFromValue(nodeHome, nodeHomeTag, parseWarnings);
						homeDefined = true;
					}
					else{
						parseWarnings << "Duplicate definition of \"" << nodeHomeTag << "\".  The engine will use the first value: " << nodeHome << "\n";
					}
				}
			}
		}

		if (thisIsMe == true) {
			nodes->push_back(Node(nodeName, ipAddress, portNumber, thisIsMe, nodeMaster, nodeData, dataDir, nodeHome));
		} else if (thisIsMe == false) {
			nodes->push_back(Node(nodeName, ipAddress, portNumber, thisIsMe));
		}

	}

}

void ConfigManager::_setDefaultSearchableAttributeBoosts(const string &coreName, const vector<string> &searchableAttributesVector)
{
    CoreInfo_t *coreInfo = NULL;
    if (coreName.compare("") != 0) {
        coreInfo = ((CoreInfoMap_t) coreInfoMap)[coreName];
    } else {
        coreInfo = getDefaultCoreInfo();
    }

    for (unsigned iter = 0; iter < searchableAttributesVector.size(); iter++) {
        coreInfo->searchableAttributesInfo[searchableAttributesVector[iter]] =
            SearchableAttributeInfoContainer(searchableAttributesVector[iter] , false, "" , iter , 1 , false);
    }
}

ConfigManager::~ConfigManager()
{
    for (CoreInfoMap_t::iterator iterator = coreInfoMap.begin(); iterator != coreInfoMap.end(); iterator++) {
        delete iterator->second;
    }
    coreInfoMap.clear();
}

uint32_t CoreInfo_t::getDocumentLimit() const {
    return documentLimit;
}

uint64_t CoreInfo_t::getMemoryLimit() const {
    return memoryLimit;
}

uint32_t CoreInfo_t::getMergeEveryNSeconds() const {
    return mergeEveryNSeconds;
}

uint32_t CoreInfo_t::getMergeEveryMWrites() const {
    return mergeEveryMWrites;
}

uint32_t CoreInfo_t::getUpdateHistogramEveryPMerges() const {
    return updateHistogramEveryPMerges;
}

uint32_t CoreInfo_t::getUpdateHistogramEveryQWrites() const {
    return updateHistogramEveryQWrites;
}

unsigned ConfigManager::getKeywordPopularityThreshold() const {
    return keywordPopularityThreshold;
}

int ConfigManager::getIndexType(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->indexType;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->indexType;
}

bool ConfigManager::getSupportSwapInEditDistance(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->supportSwapInEditDistance;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->supportSwapInEditDistance;
}

const string& ConfigManager::getAttributeLatitude(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->fieldLatitude;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->fieldLatitude;
}

const string& ConfigManager::getAttributeLongitude(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->fieldLongitude;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->fieldLongitude;
}

float ConfigManager::getDefaultSpatialQueryBoundingBox() const {
    return defaultSpatialQueryBoundingBox;
}

unsigned int ConfigManager::getNumberOfThreads() const
{
    return numberOfThreads;
}

const string& ConfigManager::getIndexPath(const string &coreName) const {
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->indexPath;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->indexPath;
}

const string& ConfigManager::getPrimaryKey(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->primaryKey;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->primaryKey;
}

const map<string, SearchableAttributeInfoContainer > * ConfigManager::getSearchableAttributes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultCoreInfo()->searchableAttributesInfo;
    }
    return &((CoreInfoMap_t) coreInfoMap)[coreName]->searchableAttributesInfo;
}

const map<string, RefiningAttributeInfoContainer > * ConfigManager::getRefiningAttributes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultCoreInfo()->refiningAttributesInfo;
    }
    return &((CoreInfoMap_t) coreInfoMap)[coreName]->refiningAttributesInfo;
}

bool ConfigManager::isFacetEnabled(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->facetEnabled;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->facetEnabled;
}

const vector<string> * ConfigManager::getFacetAttributes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultCoreInfo()->facetAttributes;
    }
    return &((CoreInfoMap_t) coreInfoMap)[coreName]->facetAttributes;
}

const vector<int> * ConfigManager::getFacetTypes(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultCoreInfo()->facetTypes;
    }
    return &((CoreInfoMap_t) coreInfoMap)[coreName]->facetTypes;
}

const vector<string> * ConfigManager::getFacetStarts(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultCoreInfo()->facetStarts;
    }
    return &((CoreInfoMap_t) coreInfoMap)[coreName]->facetStarts;
}

const vector<string> * ConfigManager::getFacetEnds(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultCoreInfo()->facetEnds;
    }
    return &((CoreInfoMap_t) coreInfoMap)[coreName]->facetEnds;
}

const vector<string> * ConfigManager::getFacetGaps(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return &getDefaultCoreInfo()->facetGaps;
    }
    return &((CoreInfoMap_t) coreInfoMap)[coreName]->facetGaps;
}


const string &ConfigManager::getSrch2Home() const {
    return srch2Home;
}

bool ConfigManager::getStemmerFlag(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->stemmerFlag;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->stemmerFlag;
}

const string &ConfigManager::getStemmerFile(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->stemmerFile;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->stemmerFile;
}

const string &ConfigManager::getSynonymFilePath(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->synonymFilterFilePath;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->synonymFilterFilePath;
}

const string &ConfigManager::getProtectedWordsFilePath(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->protectedWordsFilePath;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->protectedWordsFilePath;
}

bool ConfigManager::getSynonymKeepOrigFlag(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->synonymKeepOrigFlag;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->synonymKeepOrigFlag;
}

const string &ConfigManager::getStopFilePath(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->stopFilterFilePath;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->stopFilterFilePath;
}

const string& ConfigManager::getAttributeRecordBoostName(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->recordBoostField;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->recordBoostField;
}

/*string getDefaultAttributeRecordBoost() const
  {
  return defaultAttributeRecordBoost;
  }*/

const std::string& CoreInfo_t::getScoringExpressionString() const
{
    return scoringExpressionString;
}

int CoreInfo_t::getSearchResponseJSONFormat() const {
    return searchResponseJsonFormat;
}

const string& ConfigManager::getRecordAllowedSpecialCharacters(const string &coreName) const {
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->allowedRecordTokenizerCharacters;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->allowedRecordTokenizerCharacters;
}

int ConfigManager::getSearchType(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->searchType;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->searchType;
}

int ConfigManager::getIsPrimSearchable(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->isPrimSearchable;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->isPrimSearchable;
}

bool CoreInfo_t::getIsFuzzyTermsQuery() const
{
    return exactFuzzy;
}

bool CoreInfo_t::getQueryTermPrefixType() const
{
    return queryTermPrefixType;
}

unsigned ConfigManager::getQueryTermBoost(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->queryTermBoost;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->queryTermBoost;
}

float CoreInfo_t::getFuzzyMatchPenalty() const
{
    return fuzzyMatchPenalty;
}

float CoreInfo_t::getQueryTermSimilarityThreshold() const
{
    return queryTermSimilarityThreshold;
}

float CoreInfo_t::getQueryTermLengthBoost() const
{
    return queryTermLengthBoost;
}

float CoreInfo_t::getPrefixMatchPenalty() const
{
    return prefixMatchPenalty;
}

bool ConfigManager::getSupportAttributeBasedSearch(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->supportAttributeBasedSearch;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->supportAttributeBasedSearch;
}

ResponseType CoreInfo_t::getSearchResponseFormat() const
{
    return searchResponseContent;
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

int CoreInfo_t::getDefaultResultsToRetrieve() const
{
    return resultsToRetrieve;
}

int CoreInfo_t::getAttributeToSort() const
{
    return attributeToSort;
}

int ConfigManager::getOrdering() const {
    return ordering;
}

bool ConfigManager::isRecordBoostAttributeSet(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->recordBoostFieldFlag;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->recordBoostFieldFlag;
}

const string& ConfigManager::getHTTPServerAccessLogFile() const {
    return httpServerAccessLogFile;
}

const Logger::LogLevel& ConfigManager::getHTTPServerLogLevel() const {
    return loglevel;
}

unsigned CoreInfo_t::getCacheSizeInBytes() const
{
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
bool ConfigManager::isValidBoostFields(const CoreInfo_t *coreInfo,
				       map<string, unsigned>& boostFields)
{
    map<string, unsigned>::iterator iter;
    for (iter = boostFields.begin(); iter != boostFields.end(); ++iter) {
        if (coreInfo->searchableAttributesInfo.count(iter->first) > 0) {
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
    unsigned minCacheSize = 50 * 1048576;     // 50MB
    unsigned maxCacheSize = 500 * 1048576;    // 500MB
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
    		|| responseContentType.compare("2") == 0){
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

// Note: For release mode compiles, logLevel can still be 4 (debug), but debug output will be suppressed.
bool ConfigManager::isValidLogLevel(string& logLevel) {
    if (logLevel.compare("0") == 0 || logLevel.compare("1") == 0 || logLevel.compare("2") == 0
        || logLevel.compare("3") == 0 || logLevel.compare("4") == 0) {
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

bool ConfigManager::isPositionIndexEnabled(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return (getDefaultCoreInfo()->enableWordPositionIndex ||
        		getDefaultCoreInfo()->enableCharOffsetIndex);
    }
    return (((CoreInfoMap_t) coreInfoMap)[coreName]->enableWordPositionIndex
    		|| ((CoreInfoMap_t) coreInfoMap)[coreName]->enableCharOffsetIndex);
}

const string& ConfigManager::getMongoServerHost(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->mongoHost;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->mongoHost;
}

const string& ConfigManager::getMongoServerPort(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->mongoPort;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->mongoPort;
}

const string& ConfigManager::getMongoDbName(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->mongoDbName;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->mongoDbName;
}

const string& ConfigManager::getMongoCollection (const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->mongoCollection;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->mongoCollection;
}

const unsigned ConfigManager::getMongoListenerWaitTime (const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->mongoListenerWaitTime;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->mongoListenerWaitTime;
}

const unsigned ConfigManager::getMongoListenerMaxRetryCount(const string &coreName) const
{
    if (coreName.compare("") == 0) {
        return getDefaultCoreInfo()->mongoListenerMaxRetryOnFailure;
    }
    return ((CoreInfoMap_t) coreInfoMap)[coreName]->mongoListenerMaxRetryOnFailure;
}

CoreInfo_t *ConfigManager::getDefaultCoreInfo() const
{
    string n = getDefaultCoreName();
    CoreInfo_t *coreInfo = ((CoreInfoMap_t) coreInfoMap)[n];
    return coreInfo;
    //        return coreInfoMap[getDefaultCoreName()];
}

unsigned short CoreInfo_t::getPort(PortType_t portType) const
{
    if (static_cast<unsigned int> (portType) >= ports.size()) {
        return 0;
    }

    unsigned short portNumber = ports[portType];
    return portNumber;
}

void CoreInfo_t::setPort(PortType_t portType, unsigned short portNumber)
{
    if (static_cast<unsigned int> (portType) >= ports.size()) {
        ports.resize(static_cast<unsigned int> (EndOfPortType), 0);
    }

    switch (portType) {
    case SearchPort:
    case SuggestPort:
    case InfoPort:
    case DocsPort:
    case UpdatePort:
    case SavePort:
    case ExportPort:
    case ResetLoggerPort:
        ports[portType] = portNumber;
        break;

    default:
        Logger::error("Unrecognized HTTP listening port type: %d", static_cast<int> (portType));
        break;
    }
}

// JUST FOR Wrapper TEST
void CoreInfo_t::setDataFilePath(const string& path) {
    dataFilePath = path;
}


// end of namespaces
}
}
