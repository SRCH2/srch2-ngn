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
#ifndef __SHARDING_SHARDING_METADATA_REPORT_H__
#define __SHARDING_SHARDING_METADATA_REPORT_H__

#include "Notification.h"
#include "../metadata_manager/Cluster_Writeview.h"
#include "core/util/SerializationHelper.h"
#include "sharding/transport/Message.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

// This notification is used to read the metadata of a node
class MetadataReport: public ShardingNotification{
public:
	MetadataReport(Cluster_Writeview * writeview){
		this->writeview = writeview;
		this->shouldDeleteWriteview = true;
	}
	MetadataReport(){
	    this->shouldDeleteWriteview = false;// because it's going to be allocated in deserialize
	    this->writeview = NULL;
	};
	~MetadataReport(){
		if(this->writeview != NULL && shouldDeleteWriteview){
			delete this->writeview;
		}
	};

	bool resolveNotification(SP(ShardingNotification) _notif);

	ShardingMessageType messageType() const;
	void * serializeBody(void * buffer) const;
	unsigned getNumberOfBytesBody() const;
	void * deserializeBody(void * buffer) ;
    Cluster_Writeview * getWriteview() const;

    bool operator==(const MetadataReport & report);

private:
    // must not be deleted in this class.
    Cluster_Writeview * writeview;
    bool shouldDeleteWriteview;

public:
    ///////////// Sub classes :
    class REQUEST : public ShardingNotification{
    public:
        ShardingMessageType messageType() const;
		bool resolveNotification(SP(ShardingNotification) _notif);
		bool hasResponse() const {
				return true;
		}
    };
};

}
}


#endif // __SHARDING_SHARDING_METADATA_REPORT_H__
