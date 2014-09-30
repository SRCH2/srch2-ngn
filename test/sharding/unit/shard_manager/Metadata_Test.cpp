#include "MetadataManagerTestHelper.h"

using namespace std;
using namespace srch2::instantsearch;
using namespace srch2::httpwrapper;

int main(){

	string confPath(getenv("ConfigManagerFilePath"));
	ConfigManager * serverConf = new ConfigManager(confPath);
	ResourceMetadataManager * metadataManager = new ResourceMetadataManager();
	ShardManager * shardManager = ShardManager::createShardManager(serverConf, metadataManager);

	NodeStatusAfterRestart * nodeState = new NodeStatusAfterRestart[3];
	testFreshClusterInit(serverConf, metadataManager);
	nodeState[0] = NodeStatus_Ready;

	testNode1FirstArrival(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);

	testNode1LoadBalancing(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[1] = NodeStatus_Ready;
	testNode2FirstArrival(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);

	restart(confPath, serverConf, metadataManager);
	nodeState[1] = NodeStatus_Pending;
	validateRestart(serverConf, metadataManager, nodeState);

	// start adding nodes and and reclaim the shards.
	// first node comes back to reclaim its shards
	testNode1Reclaim(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[1] = NodeStatus_Ready;

	// second node comes again
	testNode2FirstArrival(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);

	// move 4 last shards to the second arrived node
	testNode2LoadBalancing(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;

	// now the first arrived node fails
	testNode1Failure(metadataManager);
	nodeState[1] = NodeStatus_Unassigned;


	// save and kill
	restart(confPath, serverConf, metadataManager);
	nodeState[2] = NodeStatus_Pending;
	validateRestart(serverConf, metadataManager, nodeState);

	restart(confPath, serverConf, metadataManager);
	validateRestart(serverConf, metadataManager, nodeState);

	// second node comes again
	testNode2Reclaim(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;


	restart(confPath, serverConf, metadataManager);
	nodeState[2] = NodeStatus_Pending;
	validateRestart(serverConf, metadataManager, nodeState);

	// second node comes again
	testNode2Reclaim(metadataManager->getClusterWriteview()->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;

	delete serverConf;

//	metadataManager->getClusterWriteview()->print();
	cout << "Metadata unit tests: Passed" << endl;
}
