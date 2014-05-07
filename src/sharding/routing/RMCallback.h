#ifndef __RM_CALLBACKS_H__
#define __RM_CALLBACKS_H__

#include "transport/PendingMessages.h"

namespace srch2 {
namespace httpwrapper {

/*
 * This struct is a wrapper around aggregator callbacks, this objects is used in TM for
 * resolving receiving messages.
 */
template <typename RequestType, typename ResponseType>
class RMCallback : Callback {
public:
	ResultAggregatorAndPrint<RequestType, ResponseType>& aggregrate;

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

	/*
	 * Corresponding to ResultAggregatorAndPrint::finalize(ResultsAggregatorAndPrintMetadata metadata)
	 */
	void finalize();

	virtual ~RMCallback(){
		finalize();
	};

private:
	std::vector<ResponseType*> responsesToBeDeletedAfterFinalize;
  ResultsAggregatorAndPrintMetadata meta;
};


template <typename RequestType, typename ResponseType> inline
RMCallback<RequestType, ResponseType>::RMCallback(
		ResultAggregatorAndPrint<RequestType, ResponseType>& a) : aggregrate(a) {
	//TODO decide on the place to call prePocessing
	preProcessing();
}

template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::preProcessing() {
  aggregrate.preProcessing(meta);
}

template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::timeout(void*) {
	//TODO : timeout is not implemented yet.
}


template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::callback(Message* msg) {
	// deserialize the message into the response type
	// example : msg deserializes into SerializableSearchResults
	ResponseType& response = ResponseType::deserialize(msg->buffer);

	// use aggregator callback and pass the deserialized msg
	aggregrate.callBack(&response);

	// store the pointer to this response for deallocation in destructor
	responsesToBeDeletedAfterFinalize.push_back(&response);
}

template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::callbackAll(std::vector<Message*>& msgs) {

	typedef std::vector<Message*> Messages;
	typedef std::vector<ResponseType*> Responses;

	// deserialize all messages into response objects
	for(Messages::iterator msg = msgs.begin(); msg != msgs.end(); ++msg) {
		responsesToBeDeletedAfterFinalize.push_back(&ResponseType::deserialize((*msg)->buffer));
	}

	// call aggregator callback
	aggregrate.callBack(responsesToBeDeletedAfterFinalize);
}


template <typename RequestType, typename ResponseType> inline
void RMCallback<RequestType, ResponseType>::finalize() {

	typedef std::vector<ResponseType*> Responses;
	// Finalize must call finalize of aggregator and 
  // delete the response objects after
  aggregrate.finalize(meta);
	for(typename Responses::iterator response = 
      responsesToBeDeletedAfterFinalize.begin();
			response != responsesToBeDeletedAfterFinalize.end(); ++response) {
		delete *response;
	}
}

}}
#endif /* __RM_CALLBACKS_H__ */
