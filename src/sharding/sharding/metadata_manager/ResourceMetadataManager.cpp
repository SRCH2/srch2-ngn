#include "ResourceMetadataManager.h"
#include "MetadataInitializer.h"
#include "core/util/Assert.h"
#include <iostream>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

void ClusterNodes_Writeview::getArrivedNodes(vector<NodeId> & arrivedNodes, bool returnThisNode) const{
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin();
			nodeItr != nodes.end(); ++nodeItr){
		if(! returnThisNode && nodeItr->first == currentNodeId){
			continue;
		}
		if(nodeItr->second.first == ShardingNodeStateArrived && nodeItr->second.second != NULL){
            arrivedNodes.push_back(nodeItr->first);
		}
	}
}

void ClusterNodes_Writeview::getAllNodes(std::vector<const Node *> & localCopy) const{
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin();
			nodeItr != nodes.end(); ++nodeItr){
		localCopy.push_back(nodeItr->second.second);
	}
}

unsigned ClusterNodes_Writeview::getNumberOfAliveNodes() const{
	unsigned count = 0;
	for(map<NodeId, std::pair<ShardingNodeState, Node *> >::const_iterator nodeItr = nodes.begin();
			nodeItr != nodes.end(); ++nodeItr){
		if(nodeItr->second.first != ShardingNodeStateFailed){
			count++;
		}
	}
	return count;
}

bool ClusterNodes_Writeview::isNodeAlive(const NodeId & nodeId) const{
	if(nodes.find(nodeId) == nodes.end()){
		return false;
	}
	return nodes.find(nodeId)->second.first != ShardingNodeStateFailed;
}

void ClusterNodes_Writeview::addNode(const Node & node){

    if(nodes.find(node.getId()) == nodes.end()){ // new node.
		Logger::sharding(Logger::Warning, "DiscoveryCallBack | ClusterInfoReplyMessage adding node %s.", node.toStringShort().c_str());
    	nodes[node.getId()] = std::make_pair(ShardingNodeStateNotArrived, new Node(node));
	}else{
		if(nodes[node.getId()].second != NULL){
			Logger::sharding(Logger::Warning, "DiscoveryCallBack | ClusterInfoReplyMessage adding node %s although node %s exists.",
			        node.toStringShort().c_str(), nodes[node.getId()].second->toStringShort().c_str());
			delete nodes[node.getId()].second;
		}
        nodes[node.getId()].second = new Node(node);
	}
}

void ClusterNodes_Writeview::setNodeState(NodeId nodeId, ShardingNodeState state){

    if(nodes.find(nodeId) == nodes.end()){ // new node.
        nodes[nodeId] = std::make_pair(state, (Node*)NULL);
    }else{
        nodes[nodeId].first = state;
    }
}

bool ClusterNodes_Writeview::getNodeState(const NodeId & nodeId, ShardingNodeState & state){
    if(nodes.find(nodeId) == nodes.end()){ // not found
        return false;
    }else{
        state = nodes[nodeId].first;
        return true;
    }
}

ResourceMetadataManager::ResourceMetadataManager(){
	// initialize writeview
	pthread_spin_init(&m_spinlock, 0);
	pthread_spin_init(&m_spinlock_writeview, 0);
	boost::unique_lock<boost::shared_mutex> xLock(writeviewMutex);
	writeview = NULL;
    pthread_spin_lock(&m_spinlock_writeview);
    writeviewVersionId = 0;
    pthread_spin_unlock(&m_spinlock_writeview);
}

ResourceMetadataManager::~ResourceMetadataManager(){
	boost::unique_lock<boost::shared_mutex> xLock(writeviewMutex);
	if(writeview != NULL){
		delete writeview;
	    pthread_spin_lock(&m_spinlock_writeview);
	    writeviewVersionId = 0;
	    pthread_spin_unlock(&m_spinlock_writeview);
	}
}

void ResourceMetadataManager::resolve(SP(CommitNotification) commitNotification){
	if(! commitNotification){
		ASSERT(false);
		return;
	}
    Logger::sharding(Logger::Step, "MetadataManager| Applying the commitNotifcation coming from operation %s.", commitNotification->getSrc().toString().c_str());
	applyAndCommit(commitNotification->getMetadataChange());
	// reply ack
	SP(CommitNotification::ACK) ack = ShardingNotification::create<CommitNotification::ACK>();
	ack->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
	ack->setDest(commitNotification->getSrc());
	ShardingNotification::send(ack);
}

void ResourceMetadataManager::saveMetadata(ConfigManager * confManager){
	boost::shared_lock<boost::shared_mutex> sLock(writeviewMutex);
	if(writeview != NULL){
		const string clusterName = writeview->clusterName;
		MetadataInitializer metadataInitializer(confManager, this);
		metadataInitializer.saveToDisk(clusterName);
	}else{
		ASSERT(false);
	}
}

void ResourceMetadataManager::resolve(SP(MetadataReport::REQUEST) readRequest){
	boost::shared_lock<boost::shared_mutex> sLock(writeviewMutex);
	if(readRequest->getSrc().nodeId == this->writeview->currentNodeId){
		ASSERT(false);
		return;
	}
	SP(MetadataReport) report = SP(MetadataReport)(new MetadataReport(new Cluster_Writeview(*(this->writeview))));
	report->setSrc(NodeOperationId(this->writeview->currentNodeId));
	report->setDest(readRequest->getSrc());
	ShardingNotification::send(report);
}

void ResourceMetadataManager::resolve(SP(NodeFailureNotification) nodeFailureNotif, const bool shouldLock){
	boost::unique_lock<boost::shared_mutex> xLockWriteview;
	boost::unique_lock<boost::shared_mutex> xNodeLockWriteview;
	if(shouldLock){
		xLockWriteview = boost::unique_lock<boost::shared_mutex>(writeviewMutex); // locks writeview
		xNodeLockWriteview = boost::unique_lock<boost::shared_mutex>(nodesMutex); // locks nodes
	}
	// makes changes both to nodes structure and nodeShards structure in writeview
	writeview->removeNode(nodeFailureNotif->getFailedNodeID());
	this->commitClusterMetadata(false);
}

// private method
void ResourceMetadataManager::commitClusterMetadata(ClusterResourceMetadata_Readview * newReadview){
    pthread_spin_lock(&m_spinlock);
    // set readview pointer to the new copy of writeview
    boost::shared_ptr< const ClusterResourceMetadata_Readview > metadata_readViewOld = metadata_readView;
    metadata_readView.reset(newReadview);
    pthread_spin_unlock(&m_spinlock);
    metadata_readViewOld.reset();
}

void ResourceMetadataManager::commitClusterMetadata(const bool shouldLock){
	if(shouldLock){
		writeviewMutex.lock_shared();
		nodesMutex.lock();
	}
	this->writeview->versionId++;
    pthread_spin_lock(&m_spinlock_writeview);
    writeviewVersionId = this->writeview->versionId;
    pthread_spin_unlock(&m_spinlock_writeview);
	ClusterResourceMetadata_Readview * newReadview = this->writeview->getNewReadview();
	if(shouldLock){
		writeviewMutex.unlock_shared();
		nodesMutex.unlock();
	}
	commitClusterMetadata(newReadview);
}

// readview does not go through any locks
void ResourceMetadataManager::getClusterReadView(boost::shared_ptr<const ClusterResourceMetadata_Readview> & clusterReadview) const{
	// We need the lock it to prevent the following two operations from happening at the same time.
	// One reader is doing reader = readview, which is reading the readview.
	// At the same time, we can call commitClusterMetadata(), in which we can have "readview=writeview", which is modifying the read view.
	pthread_spin_lock(&m_spinlock);
	clusterReadview = metadata_readView;
	pthread_spin_unlock(&m_spinlock);
}

/*
 * if shouldLock is false it assumes we already have both writeview and nodes X locked.
 * otherwise it acquires both before operation
 */
void ResourceMetadataManager::setWriteview(Cluster_Writeview * newWriteview, const bool shouldLock){
	if(newWriteview == NULL){
		ASSERT(false);
		return;
	}
	if(shouldLock){
		writeviewMutex.lock();
		nodesMutex.lock();
	}// assumes we already have both writeview and nodes X locked.
	if(writeview != NULL){
		delete writeview;
	}
	writeview = newWriteview;
    pthread_spin_lock(&m_spinlock_writeview);
    writeviewVersionId = this->writeview->versionId;
    pthread_spin_unlock(&m_spinlock_writeview);

	if(shouldLock){
		writeviewMutex.unlock();
		nodesMutex.unlock();
	}// assumes we already have both writeview and nodes X locked.
}

unsigned ResourceMetadataManager::applyAndCommit(MetadataChange * metadataChange, Cluster_Writeview * writeview){
	// apply change on writeview and commit
	if(metadataChange == NULL){
		ASSERT(false);
		return 0;
	}
	metadataChange->doChange(writeview);
    Logger::sharding(Logger::Detail, "MetadataManager| Applying the change : %s", metadataChange->toString().c_str() );
    writeview->versionId++;
	this->commitClusterMetadata(writeview->getNewReadview());
	return writeview->versionId - 1;
}

unsigned ResourceMetadataManager::applyAndCommit(MetadataChange * metadataChange){
	this->writeviewMutex.lock();
	unsigned oldVersionId = applyAndCommit(metadataChange, this->writeview);
    pthread_spin_lock(&m_spinlock_writeview);
    writeviewVersionId = this->writeview->versionId;
    pthread_spin_unlock(&m_spinlock_writeview);
	this->writeviewMutex.unlock();
	return oldVersionId;
}

unsigned ResourceMetadataManager::getClusterWriteviewVersionID(){
    unsigned id = 0;
	pthread_spin_lock(&m_spinlock_writeview);
    id = writeviewVersionId;
    pthread_spin_unlock(&m_spinlock_writeview);
    return id;
}

Cluster_Writeview * ResourceMetadataManager::
				getClusterWriteview_write(boost::unique_lock<boost::shared_mutex> & xLock){
	xLock = boost::unique_lock<boost::shared_mutex>(writeviewMutex);
	return writeview;
}

Cluster_Writeview * ResourceMetadataManager::getClusterWriteview_nolock(){
	return writeview;
}

const Cluster_Writeview * ResourceMetadataManager::
				getClusterWriteview_read(boost::shared_lock<boost::shared_mutex> & sLock){
	sLock = boost::shared_lock<boost::shared_mutex>(writeviewMutex);
	return writeview;
}

SP(ClusterNodes_Writeview) ResourceMetadataManager::getClusterNodesWriteview_write(){
	nodesMutex.lock();
	if(writeview == NULL){
		ASSERT(false);
		return SP(ClusterNodes_Writeview)();
	}
	return SP(ClusterNodes_Writeview)
			(new ClusterNodes_Writeview(nodesMutex, writeview->nodes, writeview->currentNodeId, true));
}

SP(const ClusterNodes_Writeview) ResourceMetadataManager::getClusterNodesWriteview_read(){
	nodesMutex.lock_shared();
	return SP(const ClusterNodes_Writeview)
			(new ClusterNodes_Writeview(nodesMutex, writeview->nodes, writeview->currentNodeId, false));
}

void ResourceMetadataManager::print(JsonResponseHandler * response) const{
	if(response != NULL){
		writeview->print(response);
		return;
	}
	if(writeview != NULL){
		cout << "**************************************************************************************************" << endl;
		cout << "Writeview : " << endl;
		cout << "**************************************************************************************************" << endl;
		writeview->print();
	}

//	if(! metadata_readView){
//		cout << "Read view is not initialized yet ..." << endl;
//	}else{
//		cout << "**************************************************************************************************" << endl;
//		cout << "Readview : " << endl;
//		cout << "**************************************************************************************************" << endl;
//		metadata_readView->print();
//	}
}

}
}
