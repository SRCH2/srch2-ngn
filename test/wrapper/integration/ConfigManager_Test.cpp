#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>

#include "src/server/ConfigManager.h"
using srch2http::ConfigManager;
using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void test(char* configFile)
{
	ConfigManager *configManager = new ConfigManager(configFile);

	configManager->loadConfigFile();

	Cluster c = configManager->getCluster();

	ASSERT(c.getClusterName() == "SRCH2 Cluster");

	vector<Node>* nodesFromCluster = c.getNodes();

	ASSERT(nodesFromCluster->size() == 2);

	for(int i = 0; i < nodesFromCluster->size(); i++){
		cout<< nodesFromCluster->at(i).getIpAddress()<<"\n"<<flush;
		if(i == 0){
		ASSERT(nodesFromCluster->at(i).getIpAddress() == "192.168.1.54");
		ASSERT(nodesFromCluster->at(i).getName() == " avatar ");
		ASSERT(nodesFromCluster->at(i).getHomeDir() == ".");
		ASSERT(nodesFromCluster->at(i).getDataDir() == "Default");
		ASSERT(nodesFromCluster->at(i).isMaster() == true);
		ASSERT(nodesFromCluster->at(i).isData() == true);
		}

		if(i == 1){
		ASSERT(nodesFromCluster->at(i).getIpAddress() == "192.168.1.55");
		ASSERT(nodesFromCluster->at(i).getName() == " frozen ");
		}

	}
	ASSERT(configManager->getSrch2Home() == "./multicore//");

}

int main() {
    test(getenv("ConfigManagerFilePath"));
}
