#include "LoadBalancer.h"



namespace srch2is = srch2::instantsearch;
using namespace srch2is;
using namespace std;
namespace srch2 {
namespace httpwrapper {

LoadBalancer * LoadBalancer::singleInstance = NULL;

LoadBalancer * LoadBalancer::createLoadBalancer(){
	if(singleInstance != NULL){
		ASSERT(false);
		return singleInstance;
	}
	singleInstance = new LoadBalancer();
	ASSERT(MetadataManager::getMetadataManager() != NULL);
	ASSERT(LockManager::getLockManager() != NULL);
	ASSERT(NodeInitializer::getNodeInitializer() != NULL);
	return singleInstance;
}
LoadBalancer * LoadBalancer::getLoadBalancer(){
	if(singleInstance == NULL){
		ASSERT(false);
		return NULL;
	}
	return singleInstance;
}

}
}
