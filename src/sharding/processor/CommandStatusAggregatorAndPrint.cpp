#include "CommandStatusAggregatorAndPrint.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class RequestWithStatusResponse>
CommandStatusAggregatorAndPrint<RequestWithStatusResponse>::CommandStatusAggregatorAndPrint(ConfigManager * configurationManager, evhttp_request *req){
	this->configurationManager = configurationManager;
	this->req = req;
}

template <class RequestWithStatusResponse>
void CommandStatusAggregatorAndPrint<RequestWithStatusResponse>::setMessages(std::stringstream & log_str){
	this->messages << log_str.str();
}

template <class RequestWithStatusResponse>
std::stringstream & CommandStatusAggregatorAndPrint<RequestWithStatusResponse>::CommandStatusAggregatorAndPrint::getMessages(){
	return this->messages;
}

/*
 * This function is always called by RoutingManager as the first call back function
 */
template <class RequestWithStatusResponse>
void CommandStatusAggregatorAndPrint<RequestWithStatusResponse>::preProcessing(ResultsAggregatorAndPrintMetadata metadata){

}

/*
 * The main function responsible of aggregating status (success or failure) results
 */
template <class RequestWithStatusResponse>
void CommandStatusAggregatorAndPrint<RequestWithStatusResponse>::callBack(SerializableCommandStatus * responseObject){

	boost::unique_lock< boost::shared_mutex > lock(_access);
	messages << responseObject->getMessage();

}

template <class RequestWithStatusResponse>
void CommandStatusAggregatorAndPrint<RequestWithStatusResponse>::callBack(vector<SerializableCommandStatus *> responseObjects){

	boost::unique_lock< boost::shared_mutex > lock(_access);
	for(vector<SerializableCommandStatus *>::iterator responseItr = responseObjects.begin(); responseItr != responseObjects.end(); ++responseItr){
		messages << (*responseItr)->getMessage();
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
template <class RequestWithStatusResponse>
void CommandStatusAggregatorAndPrint<RequestWithStatusResponse>::finalize(ResultsAggregatorAndPrintMetadata metadata){
	Logger::info("%s", messages.str().c_str());

	bmhelper_evhttp_send_reply(req, HTTP_OK, "OK",
			"{\"message\":\"The batch was processed successfully\",\"log\":["
			+ messages.str() + "]}\n");
}



}
}

