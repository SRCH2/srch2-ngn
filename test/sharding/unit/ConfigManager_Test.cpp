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

void testConfigurationParser1(char* configFile)
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

void testConfigurationParser2(char* configFile)
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
	CoreInfo_t* c1 = configManager->getCoreInfo("core1");
	CoreInfo_t* c2 = configManager->getCoreInfo("core2");
	CoreInfo_t* c3 = configManager->getCoreInfo("core3");

	ASSERT(c1->getNumberOfPrimaryShards() == 5);
	ASSERT(c1->getNumberOfReplicas() == 1);

	ASSERT(c2->getNumberOfPrimaryShards() == 3);
	ASSERT(c2->getNumberOfReplicas() == 1);

	ASSERT(c3->getNumberOfPrimaryShards() == 1);
	ASSERT(c3->getNumberOfReplicas() == 1);

}

void testVerifyConsistencyPositive(char* configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	ASSERT(configManager->verifyConsistency() == true);

}

void testVerifyConsistencyNegative(char* configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	ASSERT(configManager->verifyConsistency() == false);

}

void testCurrentNodeId(char* configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	ASSERT(configManager->getCurrentNodeId() == 1);
}

void testDiscovery(char* configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	DiscoveryParams discovery = configManager->getDiscovery();

	ASSERT(discovery.getPingInterval() == 2);
	ASSERT(discovery.getPingTimeout() == 8);
	ASSERT(discovery.getRetryCount() == 10);
}
void testDiscoveryInvalid(char* configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	DiscoveryParams discovery = configManager->getDiscovery();

	ASSERT(discovery.getPingInterval() == 1);
	ASSERT(discovery.getPingTimeout() == 1);
	ASSERT(discovery.getRetryCount() == 1);
}

void testCurrentNode(char* configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	Cluster *c = configManager->getCluster();
	const Node * currentNode = c->getCurrentNode();
	ASSERT(currentNode->thisIsMe == true);

}

void testPortsOfNode(char * configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	Cluster *c = configManager->getCluster();
	const Node * currentNode = c->getCurrentNode();
	ASSERT(currentNode->getPort(SearchPort) == 8087);
	ASSERT(currentNode->getPort(SavePort) == 9084);
	ASSERT(currentNode->getPort(UpdatePort) == 9088);
	ASSERT(currentNode->getPort(DocsPort) == 9087);
	ASSERT(currentNode->getPort(ResetLoggerPort) == 9086);
	ASSERT(currentNode->getPort(SuggestPort) == 9089);
	ASSERT(currentNode->getPort(InfoPort) == 9090);
	ASSERT(currentNode->getPort(ResetLoggerPort) == 9086);
}

void testCoreID(char * configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	CoreInfo_t* c1 = configManager->getCoreInfo("core1");
	CoreInfo_t* c2 = configManager->getCoreInfo("core2");
	CoreInfo_t* c3 = configManager->getCoreInfo("core3");
	CoreInfo_t* c4 = configManager->getCoreInfo("core4");

	ASSERT(c1->getCoreId() == 20);
	ASSERT(c2->getCoreId() == 30);
	ASSERT(c3->getCoreId() == 40);
	ASSERT(c4->getCoreId() == 50);
}

void testDefaultCoreId(char * configFile){

	ConfigManager *configManager = new ConfigManager(configFile);
	configManager->loadConfigFile();
	CoreInfo_t* c1 = configManager->getCoreInfo("core1");
	CoreInfo_t* c2 = configManager->getCoreInfo("core2");
	CoreInfo_t* c3 = configManager->getCoreInfo("core3");
	CoreInfo_t* c4 = configManager->getCoreInfo("core4");

	ASSERT(c1->getCoreId() == 0);
	ASSERT(c2->getCoreId() == 1);
	ASSERT(c3->getCoreId() == 2);
	ASSERT(c4->getCoreId() == 3);
}

int main() {
	testShard();
	Logger::setLogLevel(Logger::SRCH2_LOG_DEBUG);
	testConfigurationParser1(getenv("ConfigManagerFilePath1"));
	testConfigurationParser2(getenv("ConfigManagerFilePath2"));
	testCore(getenv("ConfigManagerFilePath1"));
	testCore(getenv("ConfigManagerFilePath2"));   //Primary Shard tag and Replica Shard tag is missing

	testVerifyConsistencyPositive(getenv("ConfigManagerFilePath1"));
	testVerifyConsistencyNegative(getenv("ConfigManagerFilePath3"));

	testCurrentNodeId(getenv("ConfigManagerFilePath1"));

	testDiscovery(getenv("ConfigManagerFilePath1"));
	testDiscoveryInvalid(getenv("ConfigManagerFilePath3"));

	testCurrentNode(getenv("ConfigManagerFilePath1"));

	testPortsOfNode(getenv("ConfigManagerFilePath1"));

	testCoreID(getenv("ConfigManagerFilePath1"));
	testDefaultCoreId(getenv("ConfigManagerFilePath2"));

}



