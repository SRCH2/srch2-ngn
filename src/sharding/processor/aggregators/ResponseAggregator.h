#ifndef __SHARDING_RESPONSE_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_RESPONSE_AGGREGATOR_AND_PRINT_H_


#include "sharding/configuration/ConfigManager.h"

namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class Request, class Response>
class PendingMessage;

/*
 * This struct is the channel to send information to aggregators from
 * pending message handlers.
 */
struct ResponseAggregatorMetadata{

};

template <class Request, class Response>
class ResponseAggregatorInterface {
public:


	ResponseAggregatorInterface(boost::shared_ptr<const Cluster> clusterReadview){
		this->clusterReadview = clusterReadview;
		clusterWriteview = NULL;
	}

	ResponseAggregatorInterface(Cluster * clusterWriteview){
		this->clusterWriteview = clusterWriteview;
	}

	boost::shared_ptr<const Cluster> getClusterReadview(){
		return this->clusterReadview;
	}
	/*
     * This function is always called by Pending Message Framework as the first call back function
     */
    virtual void preProcess(ResponseAggregatorMetadata metadata) = 0 ;
    /*
     * This function is called by Pending Message Framework if a timeout happens, The call to
     * this function must be between preProcessing(...) and callBack()
     */
    virtual void processTimeout(PendingMessage<Request, Response> * message,ResponseAggregatorMetadata metadata) = 0 ;

    /*
     * The callBack function used by Pending Message Framework
     */
    virtual void callBack(PendingMessage<Request, Response> * message) = 0;
    virtual void callBack(vector<PendingMessage<Request, Response> * > messages) = 0;

    /*
     * The last call back function called by Pending Message Framework in all cases.
     * Example of call back call order for search :
     * 1. preProcessing()
     * 2. timeoutProcessing() [only if some shard times out]
     * 3. aggregateSearchResults()
     * 4. finalize()
     */
    virtual void finalize(ResponseAggregatorMetadata metadata) = 0;


    virtual ~ResponseAggregatorInterface(){};

private:
    boost::shared_ptr<const Cluster> clusterReadview;
    Cluster * clusterWriteview;
};

}
}


#endif // __SHARDING_RESPONSE_AGGREGATOR_AND_PRINT_H_
