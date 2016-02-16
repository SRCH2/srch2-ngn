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
#ifndef __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__
#define __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__

#include "Notification.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"


namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class LoadBalancingReport : public ShardingNotification{
public:
	LoadBalancingReport(){
		this->load = 0;
	};
	LoadBalancingReport(double load){
		this->load = load;
	};

	bool resolveNotification(SP(ShardingNotification) _notif);

    ShardingMessageType messageType() const;
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer);
    double getLoad() const;

private:
	// this load might not be consistent with other loads
	// but this is the most up to date view of that node.
	double load;


public:
	//////// Sub classes
	class REQUEST : public ShardingNotification{
	public:
	    ShardingMessageType messageType() const{
	    	return ShardingLoadBalancingReportRequestMessageType;
	    }
		bool resolveNotification(SP(ShardingNotification) _notif);
	    bool hasResponse() const {
				return true;
		}
	};
};

}
}


#endif // __SHARDING_SHARDING_LOAD_BALANCING_REPORT_H__
