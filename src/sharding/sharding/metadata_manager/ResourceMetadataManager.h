/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
	uint32_t applyAndCommit(MetadataChange * metadataChange);
	uint32_t getClusterWriteviewVersionID();
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
    uint32_t writeviewVersionId;
    mutable pthread_spinlock_t m_spinlock_writeview;
	boost::shared_mutex writeviewMutex;
	boost::shared_mutex nodesMutex;
    boost::shared_ptr< const ClusterResourceMetadata_Readview > metadata_readView;
    mutable pthread_spinlock_t m_spinlock;

	void commitClusterMetadata(ClusterResourceMetadata_Readview * newReadview);
	uint32_t applyAndCommit(MetadataChange * shardingChange, Cluster_Writeview * writeview);
};


}
}

#endif //__SHARDING_SHARDING_RESOURCE_METADATA_MANAGER_H__
