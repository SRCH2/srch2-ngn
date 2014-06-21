#include "GetInfoAggregatorAndPrint.h"
#include "sharding/routing/PendingMessages.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {


GetInfoResponseAggregator::GetInfoResponseAggregator(ConfigManager * configurationManager, evhttp_request *req,
		boost::shared_ptr<const Cluster> clusterReadview, unsigned coreId):
		ResponseAggregator<GetInfoCommand,GetInfoCommandResults>(clusterReadview, coreId){
    this->configurationManager = configurationManager;
    this->req = req;

    this->readCount = 0;
    this->writeCount = 0;
    this->numberOfDocumentsInIndex  = 0;
    this->docCount = 0;
}

/*
 * This function is always called by RoutingManager as the first call back function
 */
void GetInfoResponseAggregator::preProcess(ResponseAggregatorMetadata metadata){

}
/*
 * This function is called by RoutingManager if a timeout happens, The call to
 * this function must be between preProcessing(...) and callBack()
 */
void GetInfoResponseAggregator::processTimeout(PendingMessage<GetInfoCommand,
        GetInfoCommandResults> * message,
        ResponseAggregatorMetadata metadata){
    if(message == NULL){
        return;
    }
    boost::unique_lock< boost::shared_mutex > lock(_access);
    messages << "{\"shard getInfo\":\"failed\",\"reason\":\"Corresponging shard ("<<
                    message->getNodeId()<<") timedout.\"}";
}


/*
 * The main function responsible of aggregating status (success or failure) results
 */
void GetInfoResponseAggregator::callBack(vector<PendingMessage<GetInfoCommand,
        GetInfoCommandResults> * > messages){
    boost::unique_lock< boost::shared_mutex > lock(_access);
    for(vector<PendingMessage<GetInfoCommand,
            GetInfoCommandResults> * >::iterator messageItr = messages.begin();
            messageItr != messages.end() ; ++messageItr){
        if(*messageItr == NULL || (*messageItr)->getResponseObject() == NULL){
            continue;
        }
        this->readCount += (*messageItr)->getResponseObject()->getReadCount();
        this->writeCount += (*messageItr)->getResponseObject()->getWriteCount();
        this->numberOfDocumentsInIndex += (*messageItr)->getResponseObject()->getNumberOfDocumentsInIndex();
        this->lastMergeTimeStrings.push_back((*messageItr)->getResponseObject()->getLastMergeTimeString());
        this->docCount += (*messageItr)->getResponseObject()->getDocCount();
        this->versionInfoStrings.push_back((*messageItr)->getResponseObject()->getVersionInfo());
    }
}

/*
 * The last call back function called by RoutingManager in all cases.
 * Example of call back call order for search :
 * 1. preProcessing()
 * 2. timeoutProcessing() [only if some shard times out]
 * 3. aggregateSearchResults()
 * 4. finalize()
 */
void GetInfoResponseAggregator::finalize(ResponseAggregatorMetadata metadata){

    //TODO : this print should be checked to make sure it prints correct json format
    std::stringstream str;
    str << "\"engine_status\":{";
    str << "\"search_requests\":\"" << this->readCount << "\", ";
    str << "\"write_requests\":\"" <<  this->writeCount << "\", ";
    str << "\"docs_in_index\":\"" << this->numberOfDocumentsInIndex << "\", ";
    str << "\"shard_status\":[";
    for(unsigned i=0; i < lastMergeTimeStrings.size() ; ++i){
        str << "\"shard_status_" << i << "\":{"; //TODO : we should use better information at this place
        str << "\"last_merge\":\"" << this->lastMergeTimeStrings.at(i) << "\", ";
        str << "\"version\":\"" << this->versionInfoStrings.at(i) << "\"";
        str << "}";
        if(i < lastMergeTimeStrings.size()-1){
            str << ", ";
        }
    }
    str << "], \"doc_count\":\"" << this->docCount << "\"}";
    if(messages.str().compare("") != 0){ // there is actually a message to show
        str << ",\"messages\":[" << messages.str() << "]";
    }
    Logger::info("%s", messages.str().c_str());

    bmhelper_evhttp_send_reply2(req, HTTP_OK, "OK",
            "{\"log\":["
                    + str.str() + "]}\n");
}



}
}

