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

using namespace std;
namespace srch2is = srch2::instantsearch;
using namespace pugi;
// it is related to the pgixml.hpp which is a xml parser.

using namespace srch2::instantsearch;

namespace srch2 {
namespace httpwrapper {

ConfigManager::ConfigManager(const string& configFile) {
    this->configFile = configFile;
}

void ConfigManager::loadConfigFile() {
    Logger::debug("Reading config file: %s\n", this->configFile.c_str());
    xml_document configDoc;
    // Checks if the xml file is parsed correctly or not.
    if (!configDoc.load_file(this->configFile.c_str())) {
        Logger::debug("%s parsed with errors.\n", this->configFile.c_str());
        return;
    }

    bool configSuccess = true;
    std::stringstream parseError;
    std::stringstream parseWarnings;
    // parse the config file and set the variables.
    this->parse(configDoc, configSuccess, parseError, parseWarnings);

    Logger::debug("WARNINGS while reading the configuration file:");
    Logger::debug("%s\n", parseWarnings.str().c_str());
    if (!configSuccess) {
        Logger::debug("ERRORS while reading the configuration file");
        Logger::debug("%s\n", parseError.str().c_str());
        cout << endl << parseError.str() << endl;
        exit(-1);
    }
}

void ConfigManager::parse(const pugi::xml_document& configDoc, bool &configSuccess, std::stringstream &parseError,
        std::stringstream &parseWarnings) {
    string tempUse = ""; // This is just for temporary use.

    // srch2Home is a required field
    xml_node configAttribute = configDoc.child("config").child("srch2Home");
    if (configAttribute && configAttribute.text()) { // checks if the config/srch2Home has any text in it or not
        this->srch2Home = string(configAttribute.text().get()) + "/";
    } else {
        parseError << "srch2Home is not set.\n";
        configSuccess = false;
        return;
    }

    configAttribute = configDoc.child("config").child("indexConfig").child("indexType");
    if (configAttribute && configAttribute.text()) {
        string it = string(configAttribute.text().get());
        if (this->isValidIndexType(it)) {
            this->indexType = configAttribute.text().as_int();
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

    this->supportSwapInEditDistance = true; // by default it is true
    configAttribute = configDoc.child("config").child("indexConfig").child("supportSwapInEditDistance");
    if (configAttribute && configAttribute.text()) {
        string qtmt = configAttribute.text().get();
        if (this->isValidBool(qtmt)) {
            this->supportSwapInEditDistance = configAttribute.text().as_bool();
        } else {
            parseError << "The provided supportSwapInEditDistance flag is not valid";
            configSuccess = false;
            return;
        }
    }

    this->enablePositionIndex = false; // by default it is false
    configAttribute = configDoc.child("config").child("indexConfig").child("enablePositionIndex");
    if (configAttribute && configAttribute.text()) {
        string configValue = configAttribute.text().get();
        if (this->isValidBooleanValue(configValue)) {
            this->enablePositionIndex = configAttribute.text().as_bool();
        } else {
            parseError << "enablePositionIndex should be either 0 or 1.\n";
            configSuccess = false;
            return;
        }
        Logger::info("turning on attribute based search because position index is enabled");
        this->supportAttributeBasedSearch = true;
    }

    // uniqueKey is required
    configAttribute = configDoc.child("schema").child("uniqueKey");
    if (configAttribute && configAttribute.text()) {
        this->primaryKey = string(configAttribute.text().get());
    } else {
        parseError << "uniqueKey (primary key) is not set.\n";
        configSuccess = false;
        return;
    }


    /*
     * <schema> in config.xml file
     */
    /*
     * <field>  in config.xml file
     */
    bool hasLatitude = false;
    bool hasLongitude = false;
    vector<string> searchableFieldsVector;
    vector<string> searchableFieldTypesVector;
    vector<bool> searchableFieldIndexsVector;
    vector<bool> searchableAttributesRequiredFlagVector;
    vector<string> searchableAttributesDefaultVector;

    vector<string> nonSearchableFieldsVector;
    vector<srch2::instantsearch::FilterType> nonSearchableFieldTypesVector;
    vector<bool> nonSearchableAttributesRequiredFlagVector;
    vector<string> nonSearchableAttributesDefaultVector;

    this->isPrimSearchable = 0;

    configAttribute = configDoc.child("schema").child("fields");
    if (configAttribute) {
        for (xml_node field = configAttribute.first_child(); field; field = field.next_sibling()) {
            if (string(field.name()).compare("field") == 0) {

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
                if(string(field.attribute("indexed").value()).compare("") != 0){
                    tempUse = string(field.attribute("indexed").value());
                    if(isValidBool(tempUse)){
                        if(field.attribute("indexed").as_bool()){ // indexed = true
                            isSearchable = true;
                            isRefining = true;
                        }else{ // indexed = false
                            if(string(field.attribute("searchable").value()).compare("") != 0){
                                tempUse = string(field.attribute("searchable").value());
                                if(isValidBool(tempUse)){
                                    if(field.attribute("searchable").as_bool()){
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

                            if(string(field.attribute("refining").value()).compare("") != 0){
                                tempUse = string(field.attribute("refining").value());
                                if(isValidBool(tempUse)){
                                    if(field.attribute("refining").as_bool()){
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
                    if(string(field.attribute("searchable").value()).compare("") != 0){
                        tempUse = string(field.attribute("searchable").value());
                        if(isValidBool(tempUse)){
                            if(field.attribute("searchable").as_bool()){
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

                    if(string(field.attribute("refining").value()).compare("") != 0){
                        tempUse = string(field.attribute("refining").value());
                        if(isValidBool(tempUse)){
                            if(field.attribute("refining").as_bool()){
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
                if(string(field.attribute("name").value()).compare(this->primaryKey) == 0){
                    if(isSearchable){
                        this->isPrimSearchable = 1;
                    }

                    if(isRefining){
                        nonSearchableFieldsVector.push_back(this->primaryKey);
                        nonSearchableFieldTypesVector.push_back(srch2::instantsearch::ATTRIBUTE_TYPE_TEXT);
                        nonSearchableAttributesDefaultVector.push_back("");
                        nonSearchableAttributesRequiredFlagVector.push_back(true);
                    }
                    continue;
                }
                // Checking if the values are empty or not
                if (string(field.attribute("name").value()).compare("") != 0
                        && string(field.attribute("type").value()).compare("") != 0) {
                    if(isSearchable){ // it is a searchable field
                        searchableFieldsVector.push_back(string(field.attribute("name").value()));
                        searchableFieldIndexsVector.push_back(true);

                        // Checking the validity of field type
                        tempUse = string(field.attribute("type").value());
                        if (this->isValidFieldType(tempUse , true)) {
                            searchableFieldTypesVector.push_back(tempUse);
                        } else {
                            parseError << "Config File Error: " << tempUse << " is an unknown field type.\n";
                            configSuccess = false;
                            return;
                        }

                        if (string(field.attribute("default").value()).compare("") != 0){
                            searchableAttributesDefaultVector.push_back(string(field.attribute("default").value()));
                        }else{
                            searchableAttributesDefaultVector.push_back("");
                        }

                        tempUse = string(field.attribute("required").value());
                        if (string(field.attribute("required").value()).compare("") != 0 && isValidBool(tempUse)){
                            searchableAttributesRequiredFlagVector.push_back(field.attribute("required").as_bool());
                        }else{
                            searchableAttributesRequiredFlagVector.push_back(false);
                        }
                    }

                    if(isRefining){ // it is a refining field
                        nonSearchableFieldsVector.push_back(string(field.attribute("name").value()));
                        searchableFieldIndexsVector.push_back(false);

                        // Checking the validity of field type
                        tempUse = string(field.attribute("type").value());
                        if (this->isValidFieldType(tempUse , false)) {
                            nonSearchableFieldTypesVector.push_back(parseFieldType(tempUse));
                        } else {
                            parseError << "Config File Error: " << tempUse << " is an unknown field type for refining fields.\n";
                            configSuccess = false;
                            return;
                        }

                        // Check the validity of field default value based on it's type
                        if (string(field.attribute("default").value()).compare("") != 0){
                            tempUse = string(field.attribute("default").value());
                            if(isValidFieldDefaultValue(tempUse , nonSearchableFieldTypesVector.at(nonSearchableFieldTypesVector.size()-1))){

                                if(nonSearchableFieldTypesVector.at(nonSearchableFieldTypesVector.size()-1) == srch2::instantsearch::ATTRIBUTE_TYPE_TIME){
                                    long timeValue = srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(tempUse);
                                    std::stringstream buffer;
                                    buffer << timeValue;
                                    tempUse = buffer.str();
                                }
                            }else{
                                parseError << "Config File Error: " << tempUse << " is not compatible with the type used for this field.\n";
                                tempUse = "";
                            }
                            nonSearchableAttributesDefaultVector.push_back(tempUse);
                        }else{
                            nonSearchableAttributesDefaultVector.push_back("");
                        }

                        tempUse = string(field.attribute("required").value());
                        if (string(field.attribute("required").value()).compare("") != 0 && isValidBool(tempUse)){
                            nonSearchableAttributesRequiredFlagVector.push_back(field.attribute("required").as_bool());
                        }else{
                            nonSearchableAttributesRequiredFlagVector.push_back(false);
                        }
                    }

                    // Checks for geo types. location_latitude and location_longitude are geo types
                    if (string(field.attribute("type").value()).compare("location_latitude") == 0) {
                        hasLatitude = true;
                        this->fieldLatitude = string(field.attribute("name").value());
                    }
                    if (string(field.attribute("type").value()).compare("location_longitude") == 0) {
                        hasLongitude = true;
                        this->fieldLongitude = string(field.attribute("name").value());
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
    if (searchableFieldsVector.size() == 0) {
        parseError << "No searchable fields are provided.\n";
        configSuccess = false;
        return;
    }

    if(nonSearchableFieldsVector.size() != 0){
        for (unsigned iter = 0; iter < nonSearchableFieldsVector.size(); iter++) {

            /// map<string, pair< srch2::instantsearch::FilterType, pair<string, bool> > >
            nonSearchableAttributesInfo[nonSearchableFieldsVector[iter]] =
                    pair<srch2::instantsearch::FilterType,
                            pair<string, bool> >(nonSearchableFieldTypesVector[iter],
                            pair<string, bool>(
                                    nonSearchableAttributesDefaultVector[iter],
                                    nonSearchableAttributesRequiredFlagVector[iter]));
        }
    }





    /*
     * <schema> in config.xml file
     */
    /*
     * <facetEnabled>  in config.xml file
     */
    this->facetEnabled = false; // by default it is false
    configAttribute = configDoc.child("schema").child("facetEnabled");
    if (configAttribute && configAttribute.text()) {
        string qtmt = configAttribute.text().get();
        if (this->isValidBool(qtmt)) {
            this->facetEnabled = configAttribute.text().as_bool();
        } else {
            parseError << "The facetEnabled that is provided is not valid";
            configSuccess = false;
            return;
        }
    }

    /*
     * <schema> in config.xml file
     */
    /*
     * <facetFields>  in config.xml file
     */

    if(this->facetEnabled){
        configAttribute = configDoc.child("schema").child("facetFields");
        if (configAttribute) {
            for (xml_node field = configAttribute.first_child(); field; field = field.next_sibling()) {
                if (string(field.name()).compare("facetField") == 0) {
                    if (string(field.attribute("name").value()).compare("") != 0
                            && string(field.attribute("facetType").value()).compare("") != 0){
                        // insert the name of the facet
                        this->facetAttributes.push_back(string(field.attribute("name").value()));
                        // insert the type of the facet
                        tempUse = string(field.attribute("facetType").value());
                        int facetType = parseFacetType(tempUse);
                        if(facetType == 0){ // categorical
                            this->facetTypes.push_back(facetType);
                            // insert place holders for start,end and gap
                            this->facetStarts.push_back("");
                            this->facetEnds.push_back("");
                            this->facetGaps.push_back("");
                        }else if(facetType == 1){ // range
                            this->facetTypes.push_back(facetType);
                            // insert start
                            string startTextValue = string(field.attribute("facetStart").value());
                            string facetAttributeName = string(field.attribute("name").value());
                            srch2::instantsearch::FilterType facetAttributeType ;
                            if(nonSearchableAttributesInfo.find(facetAttributeName) != nonSearchableAttributesInfo.end()){
                                facetAttributeType = nonSearchableAttributesInfo.find(facetAttributeName)->second.first;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                facetEnabled = false;
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
                                    facetEnabled = false;
                                    break;
                                }
                            }
                            this->facetStarts.push_back(startTextValue);

                            // insert end
                            string endTextValue = string(field.attribute("facetEnd").value());
                            if(nonSearchableAttributesInfo.find(facetAttributeName) != nonSearchableAttributesInfo.end()){
                                facetAttributeType = nonSearchableAttributesInfo.find(facetAttributeName)->second.first;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                facetEnabled = false;
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
                                    facetEnabled = false;
                                    break;
                                }
                            }
                            this->facetEnds.push_back(endTextValue);

                            // insert gap
                            string gapTextValue = string(field.attribute("facetGap").value());
                            if(nonSearchableAttributesInfo.find(facetAttributeName) != nonSearchableAttributesInfo.end()){
                                facetAttributeType = nonSearchableAttributesInfo.find(facetAttributeName)->second.first;
                            }else{
                                parseError << "Facet attribute is not declared as a non-searchable attribute. Facet disabled.\n";
                                facetEnabled = false;
                                break;
                            }
                            if(facetAttributeType == srch2is::ATTRIBUTE_TYPE_TIME){
                                if(!srch2is::DateAndTimeHandler::verifyDateTimeString(gapTextValue , srch2is::DateTimeTypeDurationOfTime) ){
                                    parseError << "Facet attribute end value is in wrong format.Facet disabled.\n";
                                    facetEnabled = false;
                                    break;
                                }
                            }
                            this->facetGaps.push_back(gapTextValue);
                        }else{
                            parseError << "Facet type is not recognized. Facet disabled.";
                            this->facetEnabled = false;
                            break;
                        }

                    }
                }
            }
        }
    }

    if(!facetEnabled){
        this->facetAttributes.clear();
        this->facetTypes.clear();
        this->facetStarts.clear();
        this->facetEnds.clear();
        this->facetGaps.clear();
    }

    if (this->indexType == 1) {
        // If index type is 1, it means it is geo. So both latitude and longitude should be provided.
        if (!(hasLatitude && hasLongitude)) {
            parseError << "Both Geo related attributes should set together. Currently only one of them is set.\n";
            configSuccess = false;
            return;
        }
        this->searchType = 2; // GEO search
    } else if (this->indexType == 0){
        this->searchType = 0;
        this->fieldLongitude = "IGNORE"; // IN URL parser these fields are being checked with "IGNORE". We should get rid of them.
        this->fieldLatitude = "IGNORE"; // IN URL parser these fields are being checked with "IGNORE". We should get rid of them.
        configAttribute = configDoc.child("config").child("query").child("searcherType");
        if (configAttribute && configAttribute.text()) {
            string st = configAttribute.text().get();
            if (this->isValidSearcherType(st)) {
                this->searchType = configAttribute.text().as_int();
            } else {
                parseError << "The Searcher Type only can get 0 or 1";
                configSuccess = false;
                return;
            }
        }
    }


    // Analyzer flags : Everything is disabled by default.
    this->stemmerFlag = false;
    this->stemmerFile = "";
    this->stopFilterFilePath = "";
    this->synonymFilterFilePath = "";
    this->synonymKeepOrigFlag = false;

    configAttribute = configDoc.child("schema").child("types");
    if (configAttribute) {        // Checks if <schema><types> exists or not
        for (xml_node fieldType = configAttribute.first_child(); fieldType; fieldType = fieldType.next_sibling()) { // Going on the children
            if ((string(fieldType.name()).compare("fieldType") == 0)) { // Finds the fieldTypes
                if (string(fieldType.attribute("name").value()).compare("text_en") == 0) {
                    // Checking if the values are empty or not
                    xml_node configAttributeTemp = fieldType.child("analyzer"); // looks for analyzer
                    for (xml_node field = configAttributeTemp.first_child(); field; field = field.next_sibling()) {
                        if (string(field.name()).compare("filter") == 0) {
                            if (string(field.attribute("name").value()).compare("PorterStemFilter") == 0) { // STEMMER FILTER
                                if (string(field.attribute("dictionary").value()).compare("") != 0) { // the dictionary for porter stemmer is set.
                                    this->stemmerFlag = true;
                                    this->stemmerFile = this->srch2Home + string(field.attribute("dictionary").value());
                                }
                            } else if (string(field.attribute("name").value()).compare("StopFilter") == 0) { // STOP FILTER
                                if (string(field.attribute("words").value()).compare("") != 0) { // the words file for stop filter is set.
                                    this->stopFilterFilePath = this->srch2Home
                                            + string(field.attribute("words").value());
                                }
                            } /*else if (string(field.attribute("name").value()).compare("SynonymFilter") == 0) {
                                if (string(field.attribute("synonyms").value()).compare("") != 0) { // the dictionary file for synonyms is set
                                    this->synonymFilterFilePath = this->srch2Home
                                            + string(field.attribute("synonyms").value());
                                    // checks the validity of boolean provided for 'expand'
                                    tempUse = string(field.attribute("expand").value());
                                    if (this->isValidBool(tempUse)) {
                                        this->synonymKeepOrigFlag = field.attribute("expand").as_bool();
                                    } else {
                                        parseError << "Config File Error: can not convert from '" << tempUse
                                                << "' to boolean\n";
                                        configSuccess = false;
                                        return;
                                    }
                              }
                            }*/
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

    /*
     * <Config> in config.xml file
     */
    // licenseFile is a required field
    configAttribute = configDoc.child("config").child("licenseFile");
    if (configAttribute && configAttribute.text()) { // checks if config/licenseFile exists and have any text value or not
        this->licenseKeyFile = this->srch2Home + string(configAttribute.text().get());
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
        this->indexPath = this->srch2Home + string(configAttribute.text().get());
    } else {
        parseError
                << "Path of index file is not set. You should set it as <dataDir>path/to/index/file</dataDir> in the config file.\n";
        configSuccess = false;
        return;
    }

    configAttribute = configDoc.child("config").child("dataSourceType");
    if (configAttribute && configAttribute.text()) {
        int datasourceValue = configAttribute.text().as_int(DATA_SOURCE_JSON_FILE);
        switch(datasourceValue) {
        case 0:
        	this->dataSourceType = DATA_SOURCE_NOT_SPECIFIED;
        	break;
        case 1:
        	this->dataSourceType = DATA_SOURCE_JSON_FILE;
        	break;
        case 2:
        	this->dataSourceType = DATA_SOURCE_MONGO_DB;
        	break;
        default:
        	// if user forgets to specify this option, we will assume data soruce is
        	// JSON file
        	this->dataSourceType = DATA_SOURCE_JSON_FILE;
        	break;
        }
    } else {
    	this->dataSourceType = DATA_SOURCE_JSON_FILE;
    }
    if (this->dataSourceType == DATA_SOURCE_JSON_FILE) {
    	// dataFile is a required field only if JSON file is specified as data source.
    	configAttribute = configDoc.child("config").child("dataFile");
    	if (configAttribute && configAttribute.text()) { // checks if the config/dataFile has any text in it or not
    		this->filePath = this->srch2Home + string(configAttribute.text().get());
    	}else {
    		parseError
    		<< "Path to the data file is not set. You should set it as <dataFile>path/to/data/file</dataFile> in the config file.\n";
    		configSuccess = false;
    		return;
    	}
    }

    // <config>
    //   <indexconfig>
    //        <fieldBoost>
    configAttribute = configDoc.child("config").child("indexConfig").child("fieldBoost");
    map<string, unsigned> boostsMap;
    // spliting the field boost input and put them in boostsMap
    if (configAttribute && configAttribute.text()) {
        string boostString = string(configAttribute.text().get());
        boost::algorithm::trim(boostString);
        this->splitBoostFieldValues(boostString, boostsMap);
    }

    // filling the searchableAttributesInfo map
    for (int i = 0; i < searchableFieldsVector.size(); i++) {
        if (boostsMap.find(searchableFieldsVector[i]) == boostsMap.end()) {
            searchableAttributesInfo[searchableFieldsVector[i]] =
                    pair<bool, pair<string, pair<unsigned, unsigned> > >(
                            searchableAttributesRequiredFlagVector[i],
                            pair<string, pair<unsigned, unsigned> >(
                                    searchableAttributesDefaultVector[i],
                                    pair<unsigned, unsigned>(0,1)));
        } else {
            searchableAttributesInfo[searchableFieldsVector[i]] =
                    pair<bool, pair<string, pair<unsigned, unsigned> > >(
                            searchableAttributesRequiredFlagVector[i],
                            pair<string, pair<unsigned, unsigned> >(
                                    searchableAttributesDefaultVector[i],
                                    pair<unsigned, unsigned>(0,boostsMap[searchableFieldsVector[i]])));
        }
    }

    // checks the validity of the boost fields in boostsMap
    if (!this->isValidBoostFields(boostsMap)) {
        configSuccess = false;
        parseError << "Fields that are provided in the boostField do not match with the defined fields.";
        return;
    }
    // checks the validity of the boost values in boostsMap
    if (!this->isValidBoostFieldValues(boostsMap)) {
        configSuccess = false;
        parseError << "Boost values that are provided in the boostField are not in the range [1 to 100].";
        return;
    }

    // give each searchable attribute an id based on the order in the info map
    // should be consistent with the id in the schema
    unsigned idIter = 0;

    map<string, pair<bool, pair<string, pair<unsigned,unsigned> > > >::iterator searchableAttributeIter = searchableAttributesInfo.begin();
    for(; searchableAttributeIter != searchableAttributesInfo.end(); searchableAttributeIter++){
        searchableAttributeIter->second.second.second.first = idIter;
        idIter ++;
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

    // queryTermFuzzyPenalty is an optional field
    this->queryTermSimilarityBoost = 1; // By default it is 1
    configAttribute = configDoc.child("config").child("query").child("queryTermFuzzyPenalty");
        string qtsb = configAttribute.text().get();
        if (this->isValidQueryTermSimilarityBoost(qtsb)) {
            this->queryTermSimilarityBoost = configAttribute.text().as_float();
        } else {
            configSuccess = false;
            parseError << "The expression provided for queryTermFuzzyPenalty is not a valid.";
            return;
        }
    }

    // queryTermSimilarityThreshold is an optional field
    //By default it is 0.5.
    this->queryTermSimilarityThreshold = 0.5;
    configAttribute = configDoc.child("config").child("query").child("queryTermSimilarityThreshold");
    if (configAttribute && configAttribute.text()) {
        string qtsb = configAttribute.text().get();
        if (this->isValidQueryTermSimilarityThreshold(qtsb)) {
            this->queryTermSimilarityThreshold = configAttribute.text().as_float();
            if(this->queryTermSimilarityThreshold < 0 || this->queryTermSimilarityThreshold > 1 ){
                this->queryTermSimilarityThreshold = 0.5;
                parseError << "The value provided for queryTermSimilarityThreshold is not in [0,1].";
            }
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermSimilarityThreshold is not a valid.";
            return;
        }
    }

    // queryTermSimilarityThreshold is an optional field
    //By default it is 0.5.
    this->queryTermSimilarityThreshold = 0.5;
    configAttribute = configDoc.child("config").child("query").child("queryTermSimilarityThreshold");
    if (configAttribute && configAttribute.text()) {
        string qtsb = configAttribute.text().get();
        if (this->isValidQueryTermSimilarityThreshold(qtsb)) {
            this->queryTermSimilarityThreshold = configAttribute.text().as_float();
            if(this->queryTermSimilarityThreshold < 0 || this->queryTermSimilarityThreshold > 1 ){
                this->queryTermSimilarityThreshold = 0.5;
                parseError << "The value provided for queryTermSimilarityThreshold is not in [0,1].";
            }
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermSimilarityThreshold is not a valid.";
            return;
        }
    }

    // queryTermSimilarityThreshold is an optional field
    //By default it is 0.5.
    this->queryTermSimilarityThreshold = 0.5;
    configAttribute = configDoc.child("config").child("query").child("queryTermSimilarityThreshold");
    if (configAttribute && configAttribute.text()) {
        string qtsb = configAttribute.text().get();
        if (this->isValidQueryTermSimilarityThreshold(qtsb)) {
            this->queryTermSimilarityThreshold = configAttribute.text().as_float();
            if(this->queryTermSimilarityThreshold < 0 || this->queryTermSimilarityThreshold > 1 ){
                this->queryTermSimilarityThreshold = 0.5;
                parseError << "The value provided for queryTermSimilarityThreshold is not in [0,1].";
            }
        } else {
            configSuccess = false;
            parseError << "The value provided for queryTermSimilarityThreshold is not a valid.";
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
            parseError << "The expression provided for queryTermLengthBoost is not a valid.";
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

    // cacheSize is an optional field
    this->cacheSizeInBytes = 50 * 1048576;
    configAttribute = configDoc.child("config").child("query").child("cacheSize");
    if (configAttribute && configAttribute.text()) {
        string cs = configAttribute.text().get();
        if (this->isValidCacheSize(cs)) {
            this->cacheSizeInBytes = configAttribute.text().as_uint();
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
    this->numberOfThreads = 1; // by default it is 1
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
    if (this->enablePositionIndex == false) {
        this->supportAttributeBasedSearch = false; // by default it is false
        configAttribute = configDoc.child("config").child("query").child("fieldBasedSearch");
        if (configAttribute && configAttribute.text()) {
            string configValue = configAttribute.text().get();
            if (this->isValidBooleanValue(configValue)) {
                this->supportAttributeBasedSearch = configAttribute.text().as_bool();
            } else {
                parseError << "fieldBasedSearch is not set correctly.\n";
                configSuccess = false;
                return;
            }
        }
    } else {
        // attribute based search is enabled if positional index is enabled
        this->supportAttributeBasedSearch = true;
    }

    // queryTermMatchType is an optional field
    this->exactFuzzy = false; // by default it is false
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
            parseError << "The provided responseFormat is not valid";
            configSuccess = false;
            return;
        }
    }

    // responseContent is an optional field
    this->searchResponseFormat = (ResponseType)0; // by default it is 0
    configAttribute = configDoc.child("config").child("query").child("queryResponseWriter").child("responseContent");
    if (configAttribute) {
        string type = configAttribute.attribute("type").value();
        if (this->isValidResponseContentType(type)) {
            this->searchResponseFormat = (ResponseType)configAttribute.attribute("type").as_int();
        } else {
            parseError << "The type provided for responseContent is not valid";
            configSuccess = false;
            return;
        }

        if (this->searchResponseFormat == 2) {
            if (configAttribute.text()) {
                this->splitString(string(configAttribute.text().get()), ",", this->attributesToReturn);
            } else {
                parseError << "For specified response content type, return fields should be provided.";
                configSuccess = false;
                return;
            }
        }

    }



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

    // logLevel is required
    this->loglevel = Logger::SRCH2_LOG_INFO;
    configAttribute = configDoc.child("config").child("updatehandler").child("updateLog").child("logLevel");
    bool llflag = true;
    if (configAttribute && configAttribute.text()) {
        string ll = configAttribute.text().get();
        if (this->isValidLogLevel(ll)) {
            this->loglevel = static_cast<Logger::LogLevel>(configAttribute.text().as_int());
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
    configAttribute = configDoc.child("config").child("updatehandler").child("updateLog").child("accessLogFile");
    if (configAttribute && configAttribute.text()) {
        this->httpServerAccessLogFile = this->srch2Home + string(configAttribute.text().get());
    } else {
        parseError << "httpServerAccessLogFile is not set.\n";
        configSuccess = false;
        return;
    }

    /*
     * query: END
     */

    if (this->supportAttributeBasedSearch && this->searchableAttributesInfo.size() > 31) {
        parseError
                << "To support attribute-based search, the number of searchable attributes cannot be bigger than 31.\n";
        configSuccess = false;
        return;
    }


    this->allowedRecordTokenizerCharacters = "";
    this->ordering = 0;
    this->trieBootstrapDictFile = "";
    this->attributeToSort = 0;

    if (this->dataSourceType == DATA_SOURCE_MONGO_DB) {
    	configAttribute = configDoc.child("config").child("mongodb").child("host");
    	if (configAttribute && configAttribute.text()) {
    		this->mongoHost = string(configAttribute.text().get());
    	}else {
    		parseError << "mongo host is not set.\n";
    		configSuccess = false;
    		return;
    	}
    	configAttribute = configDoc.child("config").child("mongodb").child("port");
    	if (configAttribute && configAttribute.text()) {
    		this->mongoPort = string(configAttribute.text().get());
    	}else {
    		this->mongoPort = ""; // use default port
    	}
    	configAttribute = configDoc.child("config").child("mongodb").child("db");
    	if (configAttribute && configAttribute.text()) {
    		this->mongoDbName = string(configAttribute.text().get());
    	}else {
    		parseError << "mongo data base name is not set.\n";
    		configSuccess = false;
    		return;
    	}
    	configAttribute = configDoc.child("config").child("mongodb").child("collection");
    	if (configAttribute && configAttribute.text()) {
    		this->mongoCollection = string(configAttribute.text().get());
    	}else {
    		parseError << "mongo collection name is not set.\n";
    		configSuccess = false;
    		return;
    	}
    	configAttribute = configDoc.child("config").child("mongodb").child("listenerWaitTime");
    	if (configAttribute && configAttribute.text()) {
    		this->mongoListenerWaitTime = configAttribute.text().as_uint(1);
    	}else {
    		this->mongoListenerWaitTime = 1;
    	}
    }
}

void ConfigManager::_setDefaultSearchableAttributeBoosts(const vector<string> &searchableAttributesVector) {
    for (unsigned iter = 0; iter < searchableAttributesVector.size(); iter++) {
        searchableAttributesInfo[searchableAttributesVector[iter]] =
                pair<bool, pair<string, pair<unsigned, unsigned> > >(false,
                        pair<string, pair<unsigned, unsigned> >("" , pair<unsigned, unsigned>(iter, 1) ) );
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

bool ConfigManager::getSupportSwapInEditDistance() const {
    return supportSwapInEditDistance;
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
const map<string, pair<bool, pair<string, pair<unsigned, unsigned> > > > * ConfigManager::getSearchableAttributes() const {
    return &searchableAttributesInfo;
}

const map<string, pair<srch2::instantsearch::FilterType, pair<string, bool> > > * ConfigManager::getNonSearchableAttributes() const {
    return &nonSearchableAttributesInfo;
}

const vector<string> * ConfigManager::getAttributesToReturnName() const {
    return &attributesToReturn;
}

bool ConfigManager::isFacetEnabled() const {
    return facetEnabled;
}
const vector<string> * ConfigManager::getFacetAttributes() const {
    return &facetAttributes;
}
const vector<int> * ConfigManager::getFacetTypes() const {
    return &facetTypes;
}
const vector<string> * ConfigManager::getFacetStarts() const {
    return &facetStarts;
}
const vector<string> * ConfigManager::getFacetEnds() const {
    return &facetEnds;
}

const vector<string> * ConfigManager::getFacetGaps() const {
    return &facetGaps;
}


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

float ConfigManager::getQueryTermSimilarityThreshold() const {
    return queryTermSimilarityThreshold;
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

ResponseType ConfigManager::getSearchResponseFormat() const {
    return searchResponseFormat;
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
        if ((fieldType.compare("text") == 0) || (fieldType.compare("location_latitude") == 0)
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

bool ConfigManager::isValidFieldDefaultValue(string& defaultValue, srch2::instantsearch::FilterType fieldType){
    return validateValueWithType(fieldType , defaultValue);
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
bool ConfigManager::isValidBoostFields(map<string, unsigned>& boostFields) {
    map<string, unsigned>::iterator iter;
    for (iter = boostFields.begin(); iter != boostFields.end(); ++iter) {
        if (this->searchableAttributesInfo.count(iter->first) > 0) {
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

bool ConfigManager::isValidQueryTermSimilarityBoost(string& queryTermSimilarityBoost) {
    return this->isFloat(queryTermSimilarityBoost);
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

bool ConfigManager::isValidQueryTermMatchType(string& queryTermMatchType) {
    if (queryTermMatchType.compare("0") == 0 || queryTermMatchType.compare("1") == 0) {
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


bool ConfigManager::isPositionIndexEnabled() const{
    return this->enablePositionIndex;
}

}
}
