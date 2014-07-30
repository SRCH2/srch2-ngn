#include "transport/TransportManager.h"
#include "transport/CallbackHandler.h"
#include "synchronization/SynchronizerManager.h"

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
			syncManger.config.getClusterWriteView()->addNewNode_Writeview(node);
			syncManger.config.commitClusterMetadata();
			Logger::console("[%d, %d, %d]", syncManger.config.getClusterWriteView()->getTotalNumberOfNodes()
					,syncManger.masterNodeId, syncManger.currentNodeId);
			break;

		}
		case ClusterInfoRequestMessageType:
		{
			char * messageBody = msg->getMessageBody();
			unsigned replyNodeId = *(unsigned *) messageBody;

			string serializedCluster = syncManger.config.getClusterWriteView()->serializeClusterNodes();
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
				syncManger.config.getClusterWriteView()->addNewNode_Writeview(node);
				body += nodeSerializedSize;
			}
			__sync_val_compare_and_swap(&syncManger.configUpdatesDone, false, true);
			break;
		}
		default:
			ASSERT(false);
		}
	}

	DiscoveryCallBack(SyncManager& sm) : syncManger(sm) {}
private:
	SyncManager& syncManger;
};

}}
