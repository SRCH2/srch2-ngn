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

	boost::unique_lock<boost::shared_mutex> xLock;
	Cluster_Writeview * writeview = metadataManager->getClusterWriteview_write(xLock);
	testNode1FirstArrival(writeview->currentNodeId, metadataManager);

	testNode1LoadBalancing(writeview->currentNodeId, metadataManager);
	nodeState[1] = NodeStatus_Ready;
	testNode2FirstArrival(writeview->currentNodeId, metadataManager);

	restart(confPath, serverConf, metadataManager);
	nodeState[1] = NodeStatus_Pending;
	validateRestart(serverConf, metadataManager, nodeState);

	// start adding nodes and and reclaim the shards.
	// first node comes back to reclaim its shards
	testNode1Reclaim(writeview->currentNodeId, metadataManager);
	nodeState[1] = NodeStatus_Ready;

	// second node comes again
	testNode2FirstArrival(writeview->currentNodeId, metadataManager);

	// move 4 last shards to the second arrived node
	testNode2LoadBalancing(writeview->currentNodeId, metadataManager);
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
	testNode2Reclaim(writeview->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;


	restart(confPath, serverConf, metadataManager);
	nodeState[2] = NodeStatus_Pending;
	validateRestart(serverConf, metadataManager, nodeState);

	// second node comes again
	testNode2Reclaim(writeview->currentNodeId, metadataManager);
	nodeState[2] = NodeStatus_Ready;

	delete serverConf;

//	metadataManager->getClusterWriteview()->print();
	cout << "Metadata unit tests: Passed" << endl;
}
