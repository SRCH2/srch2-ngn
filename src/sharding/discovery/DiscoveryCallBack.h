#include "transport/TransportManager.h"
#include "transport/CallbackHandler.h"
#include "synchronization/SynchronizerManager.h"
#include "sharding/sharding/ShardManager.h"
#include "sharding/sharding/metadata_manager/Cluster_Writeview.h"

namespace srch2 {
namespace httpwrapper {

class DiscoveryCallBack  : public CallBackHandler {
public:
	bool resolveMessage(Message * msg, NodeId node) {
		switch(msg->getType()) {
		case NewNodeNotificationMessageType:
		{
			//unsigned size = msg->getBodySize();
			Node node;
			node.deserialize(msg->getMessageBody());
			// TODO : we should add the new node in the local data structure of discovery manager
			ShardManager::getShardManager()->resolveSMNodeArrival(node);
			syncManger.localNodesCopyMutex.lock();
			syncManger.localNodesCopy.push_back(node);
			Logger::console("[%d, %d, %d]", syncManger.localNodesCopy.size()
					,syncManger.masterNodeId, syncManger.currentNodeId);
			syncManger.localNodesCopyMutex.unlock();
			break;

		}
		case ClusterInfoRequestMessageType:
		{
			char * messageBody = msg->getMessageBody();
			unsigned replyNodeId = *(unsigned *) messageBody;

			string serializedCluster = syncManger.serializeClusterNodes();
			Message * replyMessage = MessageAllocator().allocateMessage(serializedCluster.size());
			replyMessage->setType(ClusterInfoReplyMessageType);
			replyMessage->setDiscoveryMask();
			char * replyMessageBody = replyMessage->getMessageBody();
			memcpy(replyMessageBody, serializedCluster.c_str(), serializedCluster.size());
			syncManger.transport.sendMessage(replyNodeId, replyMessage);
			MessageAllocator().deallocateByMessagePointer(replyMessage);
			break;
		}
		case ClusterInfoReplyMessageType:
		{
			unsigned bodySize = msg->getBodySize();
			char * body = msg->getMessageBody();
			//Logger::console("got cluster info from master");
			unsigned totalNodes = *(unsigned *)body;
			body += sizeof(unsigned);
			Node node;
			for (unsigned i = 0; i < totalNodes; ++i) {
				unsigned nodeSerializedSize = *(unsigned *)body;
				body += sizeof(unsigned);
				node.deserialize(body);
				node.thisIsMe = false;
				syncManger.localNodesCopyMutex.lock();
                bool isPresent = false;
                for(unsigned i = 0 ; i < syncManger.localNodesCopy.size(); ++i){
                    if(syncManger.localNodesCopy.at(i).getId() == node.getId()){
                        isPresent = true;
                        break;
                    }
                }
                if(! isPresent){
                    syncManger.localNodesCopy.push_back(node);
                }
				syncManger.localNodesCopyMutex.unlock();
                if(! isPresent){
					SP(ClusterNodes_Writeview) writeview = ShardManager::getNodesWriteview_write();
					writeview->addNode(node);
					writeview->setNodeState(node.getId(), ShardingNodeStateArrived);
                }

				body += nodeSerializedSize;
			}
			__sync_val_compare_and_swap(&syncManger.configUpdatesDone, false, true);
			break;
		}
		default:
			ASSERT(false);
		}
		return true;
	}

	DiscoveryCallBack(SyncManager& sm) : syncManger(sm) {}
private:
	SyncManager& syncManger;
};

}}
