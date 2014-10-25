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

class ResourceMetadataManager{
public:

	ResourceMetadataManager();
	~ResourceMetadataManager();
	void resolve(SP(CommitNotification) commitNotification){
		if(! commitNotification){
			ASSERT(false);
			return;
		}
	    Logger::debug("STEP : Applying the commitNotifcation coming from operation %s ...", commitNotification->getSrc().toString().c_str());
		applyAndCommit(commitNotification->getMetadataChange());
		// reply ack
		SP(CommitNotification::ACK) ack = ShardingNotification::create<CommitNotification::ACK>();
		ack->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
		ack->setDest(commitNotification->getSrc());
		ShardingNotification::send(ack);
	}


	void saveMetadata(ConfigManager * confManager);


	void resolve(SP(MetadataReport::REQUEST) readRequest){
		SP(MetadataReport) report = SP(MetadataReport)(new MetadataReport(this->writeview));
		report->setSrc(NodeOperationId(ShardManager::getCurrentNodeId()));
		report->setDest(readRequest->getSrc());
		ShardingNotification::send(report);
	}

	void resolve(SP(NodeFailureNotification) nodeFailureNotif){
		this->writeview->removeNode(nodeFailureNotif->getFailedNodeID());
		this->commitClusterMetadata();
	}


	void commitClusterMetadata(ClusterResourceMetadata_Readview * newReadview);
	void commitClusterMetadata();
	void getClusterReadView(boost::shared_ptr<const ClusterResourceMetadata_Readview> & clusterReadview) const;
	void setWriteview(Cluster_Writeview * newWriteview);
	unsigned applyAndCommit(MetadataChange * shardingChange, Cluster_Writeview * writeview);
	unsigned applyAndCommit(MetadataChange * metadataChange);
	Cluster_Writeview * getClusterWriteview() const;

	void print();

private:
	Cluster_Writeview * writeview;
    boost::shared_ptr< const ClusterResourceMetadata_Readview > metadata_readView;
    mutable pthread_spinlock_t m_spinlock;
};


}
}

#endif //__SHARDING_SHARDING_RESOURCE_METADATA_MANAGER_H__
