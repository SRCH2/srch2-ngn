#ifndef __SHARDING_SHARDING_RESOURCE_METADATA_MANAGER_H__
#define __SHARDING_SHARDING_RESOURCE_METADATA_MANAGER_H__


#include "Cluster.h"
#include "Cluster_Writeview.h"

#include "../notifications/Notification.h"
#include "../notifications/CommitNotification.h"
#include "../notifications/MetadataReport.h"
#include "../ShardManager.h"
#include "../../configuration/ConfigManager.h"
#include "../../configuration/ShardingConstants.h"

#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {
/*
 * Thread concurrency conflicts :
 * Mutexes are :
 *      writeviewMutex
 *      nodesMutex
 * resource    |    write operation  |   read operation
 * writeview   |	WX,NX				 |	 WS,NS
 * nodes       |	W TODO				 |	 S
 *
 */

class ClusterNodes_Writeview{
public:
	// assumes nodesMutex X lock is acquited before passing it to constructor.
	ClusterNodes_Writeview(boost::shared_mutex & nodesMutex,
			map<NodeId, std::pair<ShardingNodeState, Node *> > & nodes,
			const NodeId currentNodeId, const bool xLockedByDefault):
				nodesMutex(nodesMutex),nodes(nodes),
				currentNodeId(currentNodeId), xLockedByDefault(xLockedByDefault){
		// assumes caller aquired S lock and then passes mutex to here ...
	}

	~ClusterNodes_Writeview(){
		if(xLockedByDefault){
			unlock();
		}else{
			unlock_shared();
		}
	}
	void lock(){
		nodesMutex.lock();
	}
	void unlock(){
		nodesMutex.unlock();
	}

	void lock_shared(){
		nodesMutex.lock_shared();
	}
	void unlock_shared(){
		nodesMutex.unlock_shared();
	}

	void getArrivedNodes(vector<NodeId> & allNodes, bool returnThisNode = false) const;
	void getAllNodes(std::vector<const Node *> & localCopy) const;
	unsigned getNumberOfAliveNodes() const;
	bool isNodeAlive(const NodeId & nodeId) const;
	void addNode(const Node & node);
	void setNodeState(NodeId nodeId, ShardingNodeState state);
	unsigned getTotalNumberOfNodes() const{return nodes.size();};
	map<NodeId, std::pair<ShardingNodeState, Node *> > & getNodes(){
		return nodes;
	}

private:
	map<NodeId, std::pair<ShardingNodeState, Node *> > & nodes;
	const NodeId currentNodeId;
	const bool xLockedByDefault;
	boost::shared_mutex & nodesMutex;
};

class ResourceMetadataManager{
	friend class MetadataInitializer;
public:

	ResourceMetadataManager();
	~ResourceMetadataManager();
	void resolve(SP(CommitNotification) commitNotification);


	void saveMetadata(ConfigManager * confManager);
	void resolve(SP(MetadataReport::REQUEST) readRequest);
	void resolve(SP(NodeFailureNotification) nodeFailureNotif);


	void commitClusterMetadata(const bool shouldLock = true);
	void getClusterReadView(boost::shared_ptr<const ClusterResourceMetadata_Readview> & clusterReadview) const;
	/*
	 * if shouldLock is false it assumes we already have both writeview and nodes X locked.
	 * otherwise it acquires both before operation
	 */
	void setWriteview(Cluster_Writeview * newWriteview, const bool shouldLock = true);
	unsigned applyAndCommit(MetadataChange * metadataChange);
	Cluster_Writeview * getClusterWriteview_write(boost::unique_lock<boost::mutex> & xLock) const;
//	const Cluster_Writeview * getClusterWriteview_read(boost::shared_lock<boost::shared_mutex> & sLock) const;
	SP(ClusterNodes_Writeview) getClusterNodesWriteview_write();
	SP(ClusterNodes_Writeview) getClusterNodesWriteview_read();
	boost::shared_mutex & getNodesMutex() const{
		return nodesMutex;
	}



	// sLock on writeview and nodes must be acquired before calling this method
	void print() const;

private:
	Cluster_Writeview * writeview;
	boost::shared_mutex writeviewMutex;
	boost::shared_mutex nodesMutex;
    boost::shared_ptr< const ClusterResourceMetadata_Readview > metadata_readView;
    mutable pthread_spinlock_t m_spinlock;

	void commitClusterMetadata(ClusterResourceMetadata_Readview * newReadview);
	unsigned applyAndCommit(MetadataChange * shardingChange, Cluster_Writeview * writeview);
};


}
}

#endif //__SHARDING_SHARDING_RESOURCE_METADATA_MANAGER_H__
