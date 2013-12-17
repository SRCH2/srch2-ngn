//$Id: Cache_Test.cpp 3456 2013-06-14 02:11:13Z jiaying $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

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
		ts_shared_ptr<CachedStruct> ai ;
		ai.reset(new CachedStruct(i));

		string key = "" + i;
		cacheContainer->put(key , ai);

		ts_shared_ptr<CachedStruct> aiHit ;
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
