#ifndef __SHARDING_SHARDING_TRNASACTION_TRANSACTION_SESSION_H__
#define __SHARDING_SHARDING_TRNASACTION_TRANSACTION_SESSION_H__


#include "core/util/Assert.h"
#include "core/util/Logger.h"
#include "include/instantsearch/Constants.h"
#include "../metadata_manager/Cluster.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

class JsonResponseHandler;

class TransactionSession {
public:
	TransactionSession(){
		response = NULL;
	}
	~TransactionSession(){
		if(response != NULL){
			delete response;
		}
	}
//	void setResponse(JsonResponseHandler * response){
//		this->response = response;
//	}

	JsonResponseHandler * response;

private:
};

}
}

#endif // __SHARDING_SHARDING_TRNASACTION_TRANSACTION_SESSION_H__
