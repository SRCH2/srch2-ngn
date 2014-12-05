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

class ResourceMetadataManager{
	friend class MetadataInitializer;
public:

	ResourceMetadataManager();
	~ResourceMetadataManager();
	void resolve(SP(CommitNotification) commitNotification);


	void saveMetadata(ConfigManager * confManager);
	void resolve(SP(MetadataReport::REQUEST) readRequest);
	void resolve(SP(NodeFailureNotification) nodeFailureNotif, const bool shouldLock = true);


	void commitClusterMetadata(const bool shouldLock = true, const bool shouldLockMutexOfLockManager = true);
	void getClusterReadView(boost::shared_ptr<const ClusterResourceMetadata_Readview> & clusterReadview) const;
	/*
	 * if shouldLock is false it assumes we already have both writeview and nodes X locked.
	 * otherwise it acquires both before operation
	 */
	void setWriteview(Cluster_Writeview * newWriteview, const bool shouldLock = true);
	unsigned applyAndCommit(MetadataChange * metadataChange);
	unsigned getClusterWriteviewVersionID();
	Cluster_Writeview * getClusterWriteview_write(boost::unique_lock<boost::shared_mutex> & xLock);
	Cluster_Writeview * getClusterWriteview_nolock();
	const Cluster_Writeview * getClusterWriteview_read(boost::shared_lock<boost::shared_mutex> & sLock);

	SP(ClusterNodes_Writeview) getClusterNodesWriteview_write();
	SP(const ClusterNodes_Writeview) getClusterNodesWriteview_read();

	// sLock on writeview and nodes must be acquired before calling this method
	void print(JsonResponseHandler * response = NULL) const;

private:
	// NOTE : changing this pointer (like assigning it to a new object or deleting
	// its object) requires writeview X lock and nodes S lock.
	Cluster_Writeview * writeview;
    unsigned writeviewVersionId;
    mutable pthread_spinlock_t m_spinlock_writeview;
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
