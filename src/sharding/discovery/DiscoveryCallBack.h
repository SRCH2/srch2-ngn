/*
 *   Author: Surendra
 */
#ifndef __SHARDING_DICOVERYCALLBACK_H__
#define __SHARDING_DICOVERYCALLBACK_H__

#include "transport/TransportManager.h"
#include "transport/CallbackHandler.h"
#include "synchronization/SynchronizerManager.h"
#include "sharding/sharding/ShardManager.h"
#include "sharding/sharding/metadata_manager/Cluster_Writeview.h"


namespace srch2 {
namespace httpwrapper {

/*
 * Helper function to handle all discovery related callback from TM
 */
class DiscoveryCallBack  : public CallBackHandler {
public:
	bool resolveMessage(Message * msg, NodeId node) {
		switch(msg->getType()) {
		/*
		 *  Handle the node information sent by remote node. Notify ShardManager.
		 */
		case NewNodeNotificationMessageType:
		{
			Node node;
			node.deserialize(msg->getMessageBody());
			// TODO : we should add the new node in the local data structure of discovery manager
			ShardManager::getShardManager()->resolveSMNodeArrival(node);
			syncManager.addNewNodeToLocalCopy(node);
			break;

		}
		/*
		 *  Handle cluster information request by remote node. Serialize the cluster
		 *  information and send it to remote node via TM.
		 */
		case ClusterInfoRequestMessageType:
		{
			char * messageBody = msg->getMessageBody();
			unsigned replyNodeId = *(unsigned *) messageBody;

			string serializedCluster = syncManager.serializeClusterNodes();
			Message * replyMessage = MessageAllocator().allocateMessage(serializedCluster.size());
			replyMessage->setType(ClusterInfoReplyMessageType);
			replyMessage->setDiscoveryMask(); // set the required bit to indicate that this is a discovery message
			char * replyMessageBody = replyMessage->getMessageBody();
			memcpy(replyMessageBody, serializedCluster.c_str(), serializedCluster.size());
			syncManager.transport.sendMessage(replyNodeId, replyMessage);
			MessageAllocator().deallocateByMessagePointer(replyMessage);
			break;
		}
		/*
		 *  Handle cluster information received from by the remote node (master). Deserialize
		 *  cluster information and store in CM.
		 */
		case ClusterInfoReplyMessageType:
		{
			unsigned bodySize = msg->getBodySize();
			char * body = msg->getMessageBody();
			//Logger::console("got cluster info from master");
			// First get(deserialize) the count of nodes in the cluster and then
			// deserialize the node information by iterating over each node.
			unsigned totalNodes = *(unsigned *)body;
			body += sizeof(unsigned);
			Node node;
			for (unsigned i = 0; i < totalNodes; ++i) {
				unsigned nodeSerializedSize = *(unsigned *)body;
				body += sizeof(unsigned);
				node.deserialize(body);
				node.thisIsMe = false;

				// check whether we already have node information in the SM's local copy
				// If node is already present then do not add it.
				syncManager.localNodesCopyMutex.lock();
				bool isPresent = false;
                for(unsigned i = 0 ; i < syncManager.localNodesCopy.size(); ++i){
                    if(syncManager.localNodesCopy.at(i).getId() == node.getId()){
                        isPresent = true;
                        break;
                    }
                }
                if(! isPresent){
                    syncManager.localNodesCopy.push_back(node);
                }
				syncManager.localNodesCopyMutex.unlock();
                if(! isPresent){
                	// Add node information to CM.
					SP(ClusterNodes_Writeview) writeview = ShardManager::getNodesWriteview_write();
					writeview->addNode(node);
					if(node.getId() < syncManager.currentNodeId){
						writeview->setNodeState(node.getId(), ShardingNodeStateArrived);
					}else{
						Logger::sharding(Logger::Warning, "DiscoveryCallback | ClusterInfoReplyMessage : cluster info contains \
								a node with larger nodeId : %s ", node.toStringShort().c_str());
						writeview->setNodeState(node.getId(), ShardingNodeStateNotArrived);
					}
                }

				body += nodeSerializedSize;
			}
			__sync_val_compare_and_swap(&syncManager.configUpdatesDone, false, true);
			break;
		}
		default:
			ASSERT(false);
			break;
		}
		return true;
	}

	DiscoveryCallBack(SyncManager& sm) : syncManager(sm) {}
private:
	SyncManager& syncManager;
};

}}

#endif  //__SHARDING_DICOVERYCALLBACK_H__
