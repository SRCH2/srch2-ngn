#include "RequestMessageHandler.h"
#include "processor/serializables/SerializableSearchCommandInput.h"
#include "processor/serializables/SerializableSearchCommandInput.h"
#include "processor/serializables/SerializableSearchResults.h"
#include "processor/serializables/SerializableInsertUpdateCommandInput.h"
#include "processor/serializables/SerializableDeleteCommandInput.h"
#include "processor/serializables/SerializableCommandStatus.h"
#include "processor/serializables/SerializableSerializeCommandInput.h"
#include "processor/serializables/SerializableResetLogCommandInput.h"
#include "processor/serializables/SerializableCommitCommandInput.h"
#include "processor/serializables/SerializableGetInfoCommandInput.h"
#include "processor/serializables/SerializableGetInfoResults.h"
#include "transport/Message.h"
#include "util/Version.h"
#include "RoutingManager.h"
#include "RoutingUtil.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

namespace srch2 {
namespace httpwrapper {


// DP requests that should go to DP Internal reach here ...
/*
 * We deserialize the msg in this functions and give the target object and the request object to
 * the overload of resolveMessage to pass to DP Internal.
 */
bool RequestMessageHandler::resolveMessage(Message * msg, NodeId node){

	if(msg == NULL){
		ASSERT(false);
		return false;
	}

	ASSERT(msg->isDPRequest());
	char * buffer = Message::getBodyPointerFromMessagePointer(msg);
	// 1. First read the target object
	NodeTargetShardInfo target;
	buffer = target.deserialize(buffer);
	boost::shared_ptr<const Cluster> clusterReadview;
   	configManager.getClusterReadView(clusterReadview);
	// 2. Second, based on the type of this message, deserialize Request objects
   	bool resultFlag = true;
	switch(msg->getType()){
    case SearchCommandMessageType: // -> for LogicalPlan object
    	SearchCommand * searchCommand = SearchCommand::deserialize(buffer);
        resultFlag = resolveMessage(searchCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
        delete searchCommand;
        break;
    case InsertUpdateCommandMessageType: // -> for Record object (used for insert and update)
    	InsertUpdateCommand* insertUpdateCommand =
    			InsertUpdateCommand::deserialize(buffer,clusterReadview->getCore(target.getCoreId())->getSchema());
    	resultFlag = resolveMessage(insertUpdateCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
    	delete insertUpdateCommand;
    	break;
    case DeleteCommandMessageType: // -> for DeleteCommandInput object (used for delete)
    	DeleteCommand* deleteCommand = DeleteCommand::deserialize(buffer);
    	resultFlag = resolveMessage(deleteCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
    	delete deleteCommand;
    	break;
    case SerializeCommandMessageType: // -> for SerializeCommandInput object
    	SerializeCommand* serializeCommand = SerializeCommand::deserialize(buffer);
    	resultFlag = resolveMessage(serializeCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
    	delete serializeCommand;
    	break;
    case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
    	GetInfoCommand* getInfoCommand = GetInfoCommand::deserialize(buffer);
        resultFlag = resolveMessage(getInfoCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
    	delete serializeCommand;
    	break;
    case CommitCommandMessageType: // -> for CommitCommandInput object
    	CommitCommand* commitCommand = CommitCommand::deserialize(buffer);
        resultFlag = resolveMessage(commitCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
        delete commitCommand;
        break;
    case ResetLogCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
    	ResetLogCommand* resetLogCommand = ResetLogCommand::deserialize(buffer);
        resultFlag = resolveMessage(resetLogCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
        delete resetLogCommand;
        break;
    case MergeCommandMessageType: // -> for ResetLogCommandInput (used for resetting log)
    	MergeCommand* mergeCommand = MergeCommand::deserialize(buffer);
        resultFlag = resolveMessage(mergeCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
        delete mergeCommand;
        break;
    default:
        ASSERT(false);
        return false;
    }
	return resultFlag;
}


void RequestMessageHandler::deleteResponseRequestObjectBasedOnType(ShardingMessageType type, void * responseObject){
    switch (type) {
    case SearchResultsMessageType: // -> for LogicalPlan object
        delete (SearchCommandResults*)responseObject;
        return;
    case StatusMessageType: // -> for Record object (used for insert and update)
        delete (CommandStatus*)responseObject;
        return;
    case GetInfoResultsMessageType: // -> for DeleteCommandInput object (used for delete)
    	delete (GetInfoCommandResults*)responseObject;
    	return;
    case SearchCommandMessageType:
        delete (SearchCommand*)responseObject;
        return;
    case InsertUpdateCommandMessageType:
    	delete (InsertUpdateCommand *)responseObject;
    	return;
    case DeleteCommandMessageType:
    	delete (DeleteCommand *)responseObject;
    	return;
    case SerializeCommandMessageType:
    	delete (SerializeCommand *)responseObject;
    	return;
    case GetInfoCommandMessageType:
    	delete (GetInfoCommand *)responseObject;
    	return;
    case CommitCommandMessageType:
    	delete (CommitCommand *)responseObject;
    	return;
    case ResetLogCommandMessageType:
    	delete (ResetLogCommand *)responseObject;
    	return;
    default:
        ASSERT(false);
        return;
    }
}

}
}

