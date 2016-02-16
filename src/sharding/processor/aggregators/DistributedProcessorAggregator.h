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
#ifndef __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_
#define __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_


#include "sharding/processor/aggregators/ResponseAggregator.h"
#include <instantsearch/Record.h>
#include "wrapper/ParsedParameterContainer.h"
#include "util/CustomizableJsonWriter.h"
#include "util/Logger.h"

#include "core/highlighter/Highlighter.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "server/HTTPJsonResponse.h"


namespace srch2is = srch2::instantsearch;
using namespace std;

using namespace srch2is;

namespace srch2 {
namespace httpwrapper {

template <class Request, class Response>
class DistributedProcessorAggregator : public ResponseAggregatorInterface<Request, Response>{
public:


	DistributedProcessorAggregator(boost::shared_ptr<const ClusterResourceMetadata_Readview> clusterReadview,
			unsigned coreId):ResponseAggregatorInterface<Request,Response>(clusterReadview){
		this->coreId = coreId;
	}

	unsigned getCoreId(){
		return this->coreId;
	}

    virtual ~DistributedProcessorAggregator(){
    	for(unsigned i = 0 ; i < requestObjs.size() ; ++i){
    		if(requestObjs.at(i) != NULL){
    			delete requestObjs.at(i);
    		}
    	}
    };

    void addRequestObj(Request * requestObj){
    	this->requestObjs.push_back(requestObj);
    }
    vector<Request *> getRequestObjs(){
    	return this->requestObjs;
    }

private:
    unsigned coreId;
    vector<Request *> requestObjs;
};

}
}


#endif // __SHARDING_PROCESSOR_DISTRIBUTED_PROCESSOR_AGGREGATOR_H_
