#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>

#include "src/sharding/configuration/ConfigManager.h"
using srch2http::ConfigManager;
using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

void testConfigurationParser(char* configFile)
{
	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	Cluster* c = configManager->getCluster();
	ASSERT(c->getClusterName() == "SRCH2 Cluster");
	vector<Node>* nodesFromCluster = c->getNodes();
	ASSERT(nodesFromCluster->size() == 2);

	for(int i = 0; i < nodesFromCluster->size(); i++){
		if(i == 0){
			ASSERT(nodesFromCluster->at(i).getIpAddress() == "192.168.1.54");
			ASSERT(nodesFromCluster->at(i).getPortNumber() == 8089);
			ASSERT(nodesFromCluster->at(i).thisIsMe == true);
			ASSERT(nodesFromCluster->at(i).getName() == "queen");
			ASSERT(nodesFromCluster->at(i).isMaster() == true);
			ASSERT(nodesFromCluster->at(i).isData() == true);
			ASSERT(nodesFromCluster->at(i).getHomeDir() == "myHome");
			ASSERT(nodesFromCluster->at(i).getDataDir() == "Default");
		}

		if(i == 1){
			ASSERT(nodesFromCluster->at(i).thisIsMe == false);
			ASSERT(nodesFromCluster->at(i).getIpAddress() == "192.168.1.55");
			ASSERT(nodesFromCluster->at(i).getName() == "frozen");
			ASSERT(nodesFromCluster->at(i).getPortNumber() == 8088);
		}
	}
	ASSERT(configManager->getSrch2Home() == "./multicore//");
}

void testConfigurationParser1(char* configFile)
{
	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	Cluster* c = configManager->getCluster();
	ASSERT(c->getClusterName() == "MyCluster");
	vector<Node>* nodesFromCluster = c->getNodes();
	ASSERT(nodesFromCluster->size() == 2);

	for(int i = 0; i < nodesFromCluster->size(); i++){
		if(i == 0){
			ASSERT(nodesFromCluster->at(i).getIpAddress() == "192.168.1.54");
			ASSERT(nodesFromCluster->at(i).getPortNumber() == 8087);
			ASSERT(nodesFromCluster->at(i).getName() == "avatar");
			ASSERT(nodesFromCluster->at(i).getHomeDir() == "myHome");
			ASSERT(nodesFromCluster->at(i).getDataDir() == "Default");
			ASSERT(nodesFromCluster->at(i).isMaster() == true);
			ASSERT(nodesFromCluster->at(i).isData() == true);
			ASSERT(nodesFromCluster->at(i).thisIsMe == true);
		}

		if(i == 1){
			ASSERT(nodesFromCluster->at(i).getIpAddress() == "192.168.1.55");
			ASSERT(nodesFromCluster->at(i).getName() == "frozen");
			ASSERT(nodesFromCluster->at(i).getPortNumber() == 8088);
			ASSERT(nodesFromCluster->at(i).thisIsMe == false);
		}
	}
	ASSERT(configManager->getSrch2Home() == "./multicore//");
}

void testShard(){

	Shard* s1 = new Shard(1,1,1,1);
	ASSERT(s1->getShardId().toString() == "C1_R1_1");

	Shard* s2 = new Shard(2,1,2,1);
	ASSERT(s2->getShardId().toString() == "C1_R2_1");

	Shard* s3 = new Shard(3,2,2,0);
	ASSERT(s3->getShardId().toString() == "C2_P2");
}

void testCore(char* configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
    bool configSuccess = true;
        const CoreInfo_t* c1 = configManager->getCoreInfo("core1");
        const CoreInfo_t* c2 = configManager->getCoreInfo("core2");
        const CoreInfo_t* c3 = configManager->getCoreInfo("core3");

        cout << c1->numberOfPrimaryShards;

        ASSERT(c1->numberOfPrimaryShards == 5);
        ASSERT(c1->numberOfReplicas == 1);

        ASSERT(c2->numberOfPrimaryShards == 3);
        ASSERT(c2->numberOfReplicas == 1);

        ASSERT(c3->numberOfPrimaryShards == 1);
        ASSERT(c3->numberOfReplicas == 1);

	//ASSERT(c1->getName() == "");
}


int main() {
    testShard();
    Logger::setLogLevel(Logger::SRCH2_LOG_DEBUG);
    testConfigurationParser(getenv("ConfigManagerFilePath"));
    testConfigurationParser1(getenv("ConfigManagerFilePath1"));
   // cout<<"Hello";
    testCore(getenv("ConfigManagerFilePath"));
    testCore(getenv("ConfigManagerFilePath1"));   //Primary Shard tag and Replica Shard tag is missing

}



