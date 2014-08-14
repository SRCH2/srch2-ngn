/*
 * ConfigManager_Test.cpp
 *
 *  Created on: Jul 17, 2014
 *      Author: prateek
 */

/**
 * This test case tests schema section of the config file. Checks if it works for various combination and number of searchable, refining, and indexed fields.
 */

#include "server/util/xmlParser/pugixml.hpp"
#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>
#include "util/Logger.h"
#include "ConfigManager.h"
#include "HTTPRequestHandler.h"
#include "Srch2Server.h"

using namespace std;
using namespace srch2::instantsearch;

using namespace std;
namespace srch2is = srch2::instantsearch;
namespace srch2http = srch2::httpwrapper;

using srch2http::ConfigManager;
using namespace srch2::util;
using namespace pugi;
using srch2http::SearchableAttributeInfoContainer;
using srch2http::RefiningAttributeInfoContainer;


int main(int argc, char* argv[])
{
	string configFile1(string(getenv("srch2_config_file")) + "/conf-SRI-I.xml");
	string configFile2(string(getenv("srch2_config_file")) + "/conf-SI-RI.xml");
	string configFile3(string(getenv("srch2_config_file")) + "/conf-invalid-IR.xml");
	string configFile4(string(getenv("srch2_config_file")) + "/conf-SRI-false.xml");
	string configFile5(string(getenv("srch2_config_file")) + "/conf-SI-present-R-missing.xml");
	string configFile6(string(getenv("srch2_config_file")) + "/conf-inconsistent-attribute.xml");
	string configFile7(string(getenv("srch2_config_file")) + "/conf-unique-searchable.xml");
	string configFile8(string(getenv("srch2_config_file")) + "/conf-unique-refining.xml");
	string configFile9(string(getenv("srch2_config_file")) + "/conf-invalid-boostField.xml");
    string configFile10(string(getenv("srch2_config_file")) + "/conf-invalidRecordBoostField.xml");

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

//	ASSERT(serverConf1->loadConfigFile() == true);
//	ASSERT(serverConf2->loadConfigFile() == true);
//	ASSERT(serverConf3->loadConfigFile() == true);
//	ASSERT(serverConf6->loadConfigFile() == true);
//	ASSERT(serverConf5->loadConfigFile() == true);
//	ASSERT(serverConf7->loadConfigFile() == true);
//	ASSERT(serverConf8->loadConfigFile() == false);
    ASSERT(serverConf9->loadConfigFile() == false);
    ASSERT(serverConf10->loadConfigFile() == false);

	ConfigManager::CoreInfoMap_t::iterator it;

	for(it = serverConf7->coreInfoIterateBegin(); it != serverConf7->coreInfoIterateEnd(); it++) {

		map<string, SearchableAttributeInfoContainer> ::const_iterator it2 = it->second->getSearchableAttributes()->begin();
		ASSERT(it2->first == "uniqueSearchableField");
	}

	for(it = serverConf5->coreInfoIterateBegin(); it != serverConf5->coreInfoIterateEnd(); it++) {

		map<string, RefiningAttributeInfoContainer> ::const_iterator it2 = it->second->getRefiningAttributes()->begin();
		ASSERT(it2->first == "id");
		it2++;
		ASSERT(it2->first == "refining1");
		it2++;
		ASSERT(it2->first == "refining2");
		it2++;
		ASSERT(it2->first == "refining3");
	}

	for(it = serverConf5->coreInfoIterateBegin(); it != serverConf5->coreInfoIterateEnd(); it++) {

		map<string, SearchableAttributeInfoContainer> ::const_iterator it2 = it->second->getSearchableAttributes()->begin();
		ASSERT(it2->first == "refining1");
		it2++;
		ASSERT(it2->first == "refining2");
		it2++;
	}

	for(it = serverConf6->coreInfoIterateBegin(); it != serverConf6->coreInfoIterateEnd(); it++) {

		map<string, SearchableAttributeInfoContainer> ::const_iterator it2 = it->second->getSearchableAttributes()->begin();
		ASSERT(it2->first == "director");
		it2++;
		ASSERT(it2->first == "inconsistentField");
		it2++;
		ASSERT(it2->first == "inconsistentField2");
		it2++;
	}
}



