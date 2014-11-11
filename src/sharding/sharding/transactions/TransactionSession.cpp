#include "TransactionSession.h"
#include "server/HTTPJsonResponse.h"

namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

TransactionSession::TransactionSession(){
	response = NULL;
}
TransactionSession::~TransactionSession(){
	if(response != NULL){
		delete response;
	}
}

}
}
