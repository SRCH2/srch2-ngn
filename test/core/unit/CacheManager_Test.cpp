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
