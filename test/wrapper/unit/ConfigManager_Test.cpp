/*
 * ConfigManager_Test.cpp
 *
 *  Created on: Jul 17, 2014
 *      Author: prateek
 */

/**
 * This test case tests schema section of the config file. Checks if it works for various combination and number of searchable, refining, and indexed fields.
 * It checks for invalid boost, invalid record boost field and all possible combinations for character offset/positionalIndex and field based search.
 *
 * The conf-logging.xml file has log tag outside core, it enables testing of global log tag.
 */

#include "server/util/xmlParser/pugixml.hpp"
#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>
#include "util/Logger.h"
#include "sharding/configuration/ConfigManager.h"
#include "HTTPRequestHandler.h"
#include "Srch2Server.h"

using namespace std;
using namespace srch2::instantsearch;

using namespace std;
namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using srch2http::ConfigManager;
using srch2http::CoreInfo_t;
using namespace srch2::util;
using namespace pugi;
using srch2http::SearchableAttributeInfoContainer;
using srch2http::RefiningAttributeInfoContainer;


int main(int argc, char* argv[])
{
	const char * config_dir = getenv("srch2_config_file");
	ASSERT(config_dir != NULL);
    string configFile1(string(config_dir) + "/conf-SRI-I.xml");
    string configFile2(string(config_dir) + "/conf-SI-RI.xml");
    string configFile3(string(config_dir) + "/conf-invalid-IR.xml");
    string configFile4(string(config_dir) + "/conf-SRI-false.xml");
    string configFile5(string(config_dir) + "/conf-SI-present-R-missing.xml");
    string configFile6(string(config_dir) + "/conf-inconsistent-attribute.xml");
    string configFile7(string(config_dir) + "/conf-unique-searchable.xml");
    string configFile8(string(config_dir) + "/conf-unique-refining.xml");
    string configFile9(string(config_dir) + "/conf-invalid-boostField.xml");
    string configFile10(string(config_dir) + "/conf-invalidRecordBoostField.xml");
    string configFile11(string(config_dir) + "/conf-fieldBasedSearch.xml");
    string configFile12(string(config_dir) + "/conf-fieldBasedSearch-2.xml");
    string configFile13(string(config_dir) + "/conf-fieldBasedSearch-3.xml");
    string configFile14(string(config_dir) + "/conf-fieldBasedSearch-4.xml");
    string configFile15(string(config_dir) + "/conf-responseContent.xml");
    string configFile16(string(config_dir) + "/conf-singleCore.xml");
    string configFile17(string(config_dir) + "/conf-responseContent.xml");
//    string configFile18(string(config_dir) + "/conf-sqlLite.xml");//TODO disabled for shardign for now

    ConfigManager *serverConf1 = new ConfigManager(configFile1);
    ConfigManager *serverConf2 = new ConfigManager(configFile2);
    ConfigManager *serverConf3 = new ConfigManager(configFile3);
    ConfigManager *serverConf4 = new ConfigManager(configFile4);
    ConfigManager *serverConf5 = new ConfigManager(configFile5);
    ConfigManager *serverConf6 = new ConfigManager(configFile6);
    ConfigManager *serverConf7 = new ConfigManager(configFile7);
    ConfigManager *serverConf8 = new ConfigManager(configFile8);
    ConfigManager *serverConf9 = new ConfigManager(configFile9);
    ConfigManager *serverConf10 = new ConfigManager(configFile10);
    ConfigManager *serverConf11 = new ConfigManager(configFile11);
    ConfigManager *serverConf12 = new ConfigManager(configFile12);
    ConfigManager *serverConf13 = new ConfigManager(configFile13);
    ConfigManager *serverConf14 = new ConfigManager(configFile14);
    ConfigManager *serverConf15 = new ConfigManager(configFile15);
    ConfigManager *serverConf16 = new ConfigManager(configFile16);
    ConfigManager *serverConf17 = new ConfigManager(configFile17);
//    ConfigManager *serverConf18 = new ConfigManager(configFile18); //TODO disabled for shardign for now


    ASSERT(serverConf1->loadConfigFile() == true);
    ASSERT(serverConf2->loadConfigFile() == true);
    ASSERT(serverConf3->loadConfigFile() == true);
    ASSERT(serverConf6->loadConfigFile() == true);
    ASSERT(serverConf5->loadConfigFile() == true);
    ASSERT(serverConf7->loadConfigFile() == true);
    ASSERT(serverConf8->loadConfigFile() == false);
    ASSERT(serverConf9->loadConfigFile() == false);
    ASSERT(serverConf10->loadConfigFile() == false);
    ASSERT(serverConf11->loadConfigFile() == true);
    ASSERT(serverConf12->loadConfigFile() == true);
    ASSERT(serverConf13->loadConfigFile() == true);
    ASSERT(serverConf14->loadConfigFile() == true);
    ASSERT(serverConf15->loadConfigFile() == true);
    ASSERT(serverConf16->loadConfigFile() == true);
    ASSERT(serverConf17->loadConfigFile() == true);

    //This config file is single core with no core tags and dataFile at top, but it
    //has dataDir at the top level.
//    ASSERT(serverConf18->loadConfigFile() == true); //TODO disabled for shardign for now

    //This checks if the log file path and log level are correctly set in the config file where log
    //tag has been moved out of core
    ASSERT(serverConf15->getHTTPServerAccessLogFile() == "./multicore/srch2-log.txt");
    ASSERT(serverConf15->getHTTPServerLogLevel() == 3);

    const std::string &expr_string = "invalid Expression";
    RankerExpression* rank = new RankerExpression(expr_string);

    std::vector<CoreInfo_t *>::iterator it;

    for(it = serverConf14->coreInfoIterateBegin(); it != serverConf14->coreInfoIterateEnd(); it++) {

        ASSERT((*it)->getSupportAttributeBasedSearch() == 1);
    }

    for(it = serverConf13->coreInfoIterateBegin(); it != serverConf13->coreInfoIterateEnd(); it++) {
        ASSERT((*it)->getSupportAttributeBasedSearch() == 1);
    }

    for(it = serverConf12->coreInfoIterateBegin(); it != serverConf12->coreInfoIterateEnd(); it++) {
        ASSERT((*it)->getSupportAttributeBasedSearch() == 0);
    }

    for(it = serverConf11->coreInfoIterateBegin(); it != serverConf11->coreInfoIterateEnd(); it++) {
        ASSERT((*it)->getSupportAttributeBasedSearch() == 1);
    }

    for(it = serverConf7->coreInfoIterateBegin(); it != serverConf7->coreInfoIterateEnd(); it++) {

        map<string, SearchableAttributeInfoContainer> ::const_iterator it2 = (*it)->getSearchableAttributes()->begin();
        ASSERT(it2->first == "uniqueSearchableField");
    }

    for(it = serverConf5->coreInfoIterateBegin(); it != serverConf5->coreInfoIterateEnd(); it++) {

        map<string, RefiningAttributeInfoContainer> ::const_iterator it2 = (*it)->getRefiningAttributes()->begin();
        ASSERT(it2->first == "id");
        it2++;
        ASSERT(it2->first == "refining1");
        it2++;
        ASSERT(it2->first == "refining2");
        it2++;
        ASSERT(it2->first == "refining3");
    }

    for(it = serverConf5->coreInfoIterateBegin(); it != serverConf5->coreInfoIterateEnd(); it++) {

        map<string, SearchableAttributeInfoContainer> ::const_iterator it2 = (*it)->getSearchableAttributes()->begin();
        ASSERT(it2->first == "refining1");
        it2++;
        ASSERT(it2->first == "refining2");
        it2++;
    }

    for(it = serverConf6->coreInfoIterateBegin(); it != serverConf6->coreInfoIterateEnd(); it++) {

        map<string, SearchableAttributeInfoContainer> ::const_iterator it2 = (*it)->getSearchableAttributes()->begin();
        ASSERT(it2->first == "director");
        it2++;
        ASSERT(it2->first == "inconsistentField");
        it2++;
        ASSERT(it2->first == "inconsistentField2");
        it2++;
    }
    return 0;
}



