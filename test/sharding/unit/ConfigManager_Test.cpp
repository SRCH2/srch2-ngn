#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>

#include "src/sharding/configuration/ConfigManager.h"
using srch2http::ConfigManager;

using namespace std;
using namespace srch2::instantsearch;

void test(char* configFile)
{
	ConfigManager *configManager = new ConfigManager(configFile);

	configManager->loadConfigFile();

	ASSERT(configManager->getSrch2Home() == "./multicore//");




}

int main() {
    test(getenv("ConfigManagerFilePath"));
}
