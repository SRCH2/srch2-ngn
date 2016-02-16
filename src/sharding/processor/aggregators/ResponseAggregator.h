/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SHARDING_RESPONSE_AGGREGATOR_AND_PRINT_H_
#define __SHARDING_RESPONSE_AGGREGATOR_AND_PRINT_H_


#include "sharding/configuration/ConfigManager.h"
#include "sharding/sharding/metadata_manager/Cluster.h"

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


	ResponseAggregatorInterface(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview){
		this->clusterReadview = clusterReadview;
		clusterWriteview = NULL;
	}

	ResponseAggregatorInterface(ClusterResourceMetadata_Readview * clusterWriteview){
		this->clusterWriteview = clusterWriteview;
	}

	boost::shared_ptr<const ClusterResourceMetadata_Readview> getClusterReadview(){
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
    boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview;
    ClusterResourceMetadata_Readview * clusterWriteview;
};

}
}


#endif // __SHARDING_RESPONSE_AGGREGATOR_AND_PRINT_H_
