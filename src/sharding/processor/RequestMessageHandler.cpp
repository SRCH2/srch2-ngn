#include "RequestMessageHandler.h"

#include "transport/Message.h"
#include "util/Version.h"

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
	void * buffer = (void*) Message::getBodyPointerFromMessagePointer(msg);
	// 1. First read the target object
	NodeTargetShardInfo target;
	buffer = target.deserialize(buffer);
	boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
   	ShardManager::getShardManager()->getMetadataManager()->getClusterReadView(clusterReadview);
	// 2. Second, based on the type of this message, deserialize Request objects
   	bool resultFlag = true;
	switch(msg->getType()){
    case SearchCommandMessageType: // -> for LogicalPlan object
    {
    	SearchCommand * searchCommand = SearchCommand::deserialize(buffer, clusterReadview->getCore(target.getCoreId())->getSchema());
        resultFlag = resolveMessage(searchCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
        delete searchCommand;
        break;
    }
    case GetInfoCommandMessageType: // -> for GetInfoCommandInput object (used for getInfo)
    {
    	GetInfoCommand* getInfoCommand = GetInfoCommand::deserialize(buffer);
        resultFlag = resolveMessage(getInfoCommand, node, msg->getMessageId(), target, msg->getType(), clusterReadview);
    	delete getInfoCommand;
    	break;
    }
    default:
    {
        ASSERT(false);
        return false;
    }
    }
	return resultFlag;
}


void RequestMessageHandler::deleteResponseRequestObjectBasedOnType(ShardingMessageType type, void * responseObject){
    switch (type) {
    case SearchResultsMessageType: // -> for LogicalPlan object
        delete (SearchCommandResults*)responseObject;
        return;
    case GetInfoResultsMessageType: // -> for DeleteCommandInput object (used for delete)
    	delete (GetInfoCommandResults*)responseObject;
    	return;
    case SearchCommandMessageType:
        delete (SearchCommand*)responseObject;
        return;
    case GetInfoCommandMessageType:
    	delete (GetInfoCommand *)responseObject;
    	return;
    default:
        ASSERT(false);
        return;
    }
}

}
}

