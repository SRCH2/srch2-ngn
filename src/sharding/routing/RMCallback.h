#ifndef __SHARDING_RM_CALLBACKS_H__
#define __SHARDING_RM_CALLBACKS_H__

#include "sharding/transport/PendingMessages.h"
#include "sharding/routing/RoutingUtil.h"

namespace srch2 {
namespace httpwrapper {

/*
 * This struct is a wrapper around aggregator callbacks, this objects is used in TM for
 * resolving receiving messages.
 */
template <typename RequestType, typename ResponseType>
class RMCallback : public Callback {
public:
	RMCallback(ResultAggregatorAndPrint<RequestType, ResponseType>&);

	/*
	 * Corresponding to ResultAggregatorAndPrint::preProcessing(ResultsAggregatorAndPrintMetadata metadata)
	 */
	void preProcessing();

	/*
	 * Corresponding to ResultAggregatorAndPrint::timeoutProcessing
	 */
	void timeout(void*);

	/*
	 * Corresponding to ResultAggregatorAndPrint::callBack(Response * responseObject)
	 */
	void callback(Message*);

	/*
	 * Corresponding to ResultAggregatorAndPrint::callBack(vector<Response *> responseObjects)
	 */
	void callbackAll(std::vector<Message*>&);

	virtual ~RMCallback(){
		// Finalize must call finalize of aggregator and
		// delete the response objects after
		aggregrate.finalize(meta);

	};

private:
	ResultAggregatorAndPrint<RequestType, ResponseType>& aggregrate;
	ResultsAggregatorAndPrintMetadata meta;
	bool hasCalledPreProcess;
	mutable boost::shared_mutex _access;
};


template <typename RequestType, typename ResponseType> inline
RMCallback<RequestType, ResponseType>::RMCallback(
		ResultAggregatorAndPrint<RequestType, ResponseType>& a) : aggregrate(a) {
	hasCalledPreProcess = false;
}

template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::preProcessing() {
	aggregrate.preProcessing(meta);
}

template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::timeout(void*) {
	{
		boost::unique_lock< boost::shared_mutex > lock(_access);
		if(hasCalledPreProcess == false){
			preProcessing();
			hasCalledPreProcess = true;
		}
	}
	//TODO : timeout is not implemented yet.
}


template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::callback(Message* responseMessage) {
	{
		boost::unique_lock< boost::shared_mutex > lock(_access);
		if(hasCalledPreProcess == false){
			preProcessing();
			hasCalledPreProcess = true;
		}
	}
	// deserialize the message into the response type
	// example : msg deserializes into SerializableSearchResults
	ResponseType * responseObject = NULL;
	if(responseMessage->isLocal() == true){ // for local response we should just get the pointer
		responseObject = decodeInternalMessage<ResponseType>(responseMessage);
	}else{ // for non-local response we should deserialize the message
		responseObject = decodeExternalMessage<ResponseType>(responseMessage);
	}

	// use aggregator callback and pass the deserialized msg
	aggregrate.addResponseObject(responseObject);
	aggregrate.callBack(responseObject);

}

template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::callbackAll(std::vector<Message*>& responseMessages) {

	/*
	 * This function is entered always by only one thread.
	 */


	{
		boost::unique_lock< boost::shared_mutex > lock(_access);
		if(hasCalledPreProcess == false){
			preProcessing();
			hasCalledPreProcess = true;
		}
	}

	std::vector<ResponseType*> responseObjects;
	// deserialize all messages into response objects
	for(std::vector<Message*>::iterator msgItr = responseMessages.begin(); msgItr != responseMessages.end(); ++msgItr) {
		// deserialize the message into the response type
		// example : msg deserializes into SerializableSearchResults
		ResponseType * responseObject = NULL;
		if((*msgItr)->isLocal() == true){ // for local response we should just get the pointer
			responseObject = decodeInternalMessage<ResponseType>(*msgItr);
		}else{ // for non-local response we should deserialize the message
			responseObject = decodeExternalMessage<ResponseType>(*msgItr);
		}
		responseObjects.push_back(responseObject);
	}

	// call aggregator callback
	aggregrate.addResponseObjects(responseObjects);
	aggregrate.callBack(responseObjects);
}



}}
#endif /* __SHARDING_RM_CALLBACKS_H__ */
