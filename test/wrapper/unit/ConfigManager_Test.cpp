/*
 * ConfigManager_Test.cpp
 *
 *  Created on: Jul 18, 2014
 *      Author: prateek
 */




/*
 * ConfigManager_Test.cpp
 *
 *  Created on: Jul 17, 2014
 *      Author: prateek
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
	string configFile1(string(getenv("srch2_config_file")) + "/conf-multicore-1.xml");
	string configFile2(string(getenv("srch2_config_file")) + "/conf-multicore-2.xml");
	string configFile3(string(getenv("srch2_config_file")) + "/conf-multicore-3.xml");
	string configFile4(string(getenv("srch2_config_file")) + "/conf-multicore-4.xml");
	string configFile5(string(getenv("srch2_config_file")) + "/conf-multicore-5.xml");
	string configFile6(string(getenv("srch2_config_file")) + "/conf-multicore-6.xml");
	string configFile7(string(getenv("srch2_config_file")) + "/conf-multicore-7.xml");
	string configFile8(string(getenv("srch2_config_file")) + "/conf-multicore-8.xml");

	ConfigManager *serverConf1 = new ConfigManager(configFile1);
	ConfigManager *serverConf2 = new ConfigManager(configFile2);
	ConfigManager *serverConf3 = new ConfigManager(configFile3);
	ConfigManager *serverConf4 = new ConfigManager(configFile4);
	ConfigManager *serverConf5 = new ConfigManager(configFile5);
	ConfigManager *serverConf6 = new ConfigManager(configFile6);
	ConfigManager *serverConf7 = new ConfigManager(configFile7);
	ConfigManager *serverConf8 = new ConfigManager(configFile8);

	cout << serverConf1->loadConfigFile() << "\n";
	cout << serverConf2->loadConfigFile() << "\n";
	cout << serverConf3->loadConfigFile() << "\n";
	cout << serverConf6->loadConfigFile() << "\n";
	cout << serverConf5->loadConfigFile() << "\n";
	cout << serverConf7->loadConfigFile() << "\n";
	cout << serverConf8->loadConfigFile() << "\n";

	ConfigManager::CoreInfoMap_t::iterator it;

	for(it = serverConf8->coreInfoIterateBegin(); it != serverConf8->coreInfoIterateEnd(); it++) {

		map<string, RefiningAttributeInfoContainer> ::const_iterator it2 = it->second->getRefiningAttributes()->begin();
		ASSERT(it2->first == "uniqueRefiningField");
	}

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



