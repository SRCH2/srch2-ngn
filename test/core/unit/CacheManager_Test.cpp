
#include "operation/CacheBase.h"

#include <instantsearch/GlobalCache.h>
#include <assert.h>
#include "util/Assert.h"

using namespace std;
using namespace srch2::util;
using namespace srch2::instantsearch;
namespace srch2is = srch2::instantsearch;

class CachedStruct{
public:
	int a;
	CachedStruct(int a){
		this->a = a;
	}
	unsigned getNumberOfBytes(){
		return 12;
	}
};

void test1(srch2is::CacheContainer<CachedStruct> * cacheContainer){

	for(unsigned i=0; i< 2000; i++){
		boost::shared_ptr<CachedStruct> ai ;
		ai.reset(new CachedStruct(i));

                ostringstream convert; // convert the unsigned to a string
                convert << i;
		string key = convert.str();
		cacheContainer->put(key , ai);

		boost::shared_ptr<CachedStruct> aiHit ;
		ASSERT(cacheContainer->get(key , aiHit));
		ASSERT(cacheContainer->checkCacheConsistency());
		ASSERT(aiHit->a == ai->a);
	}

}


int main(int argc, char *argv[])
{

	srch2is::CacheContainer<CachedStruct> * cacheContainer = new srch2is::CacheContainer<CachedStruct>(200);

	test1(cacheContainer);

    cout << "CacheContainer Unit Test: Passed\n";
}
