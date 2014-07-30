#include "RoutingManager.h"
#include "server/MongodbAdapter.h"

using namespace srch2::httpwrapper;

namespace srch2 {
namespace httpwrapper {


RoutingManager::RoutingManager(ConfigManager&  cm, DPInternalRequestHandler& dpInternal, TransportManager& transportManager)  :
                                    configurationManager(cm),
                                    dpInternal(dpInternal),
                                    transportManager(transportManager),
                                    internalMessageHandler(*this, dpInternal),
                                    replyMessageHandler(transportManager.getMessageAllocator()){

    // share the internal message broker from RM to TM
    transportManager.registerCallbackForInternalMessageHandler(&internalMessageHandler);
    transportManager.registerCallbackForReplyMessageHandler(&replyMessageHandler);
    transportManager.setRoutingManager(this);
}

ConfigManager* RoutingManager::getConfigurationManager() {
    return &this->configurationManager;

}

DPInternalRequestHandler* RoutingManager::getDpInternal() {
    return &this->dpInternal;
}

RequestMessageHandler * RoutingManager::getInternalMessageHandler(){
    return &this->internalMessageHandler;
}

ReplyMessageHandler * RoutingManager::getReplyMessageHandler(){
    return &(this->replyMessageHandler);
}

TransportManager& RoutingManager::getTransportManager(){
    return transportManager;
}

MessageAllocator * RoutingManager::getMessageAllocator() {
    return transportManager.getMessageAllocator();
}

} }
//save indexes in deconstructor
