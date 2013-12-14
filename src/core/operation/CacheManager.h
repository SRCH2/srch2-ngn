// $Id: Cache.h 3490 2013-06-25 00:57:57Z jamshid.esmaelnezhad $
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

#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include <instantsearch/GlobalCache.h>
#include "operation/ActiveNode.h"

#include "CacheBase.h"

#include <boost/shared_ptr.hpp>

using namespace std;

namespace srch2
{
namespace instantsearch
{


class ActiveNodesCache { // TODO : remove busy bit from PrefixActiveNodeSet
public:
	ActiveNodesCache(unsigned long byteSizeOfCache = 134217728){
		this->cacheContainer = new CacheContainer<PrefixActiveNodeSet>(byteSizeOfCache);
	}
	int findLongestPrefixActiveNodes(Term *term, boost::shared_ptr<PrefixActiveNodeSet> &in);
	int setPrefixActiveNodeSet(boost::shared_ptr<PrefixActiveNodeSet> &prefixActiveNodeSet);
private:
	CacheContainer<PrefixActiveNodeSet> * cacheContainer;

};


/*
 * This class is the cache manager. The CacheManager is the holder of different kinds of Cache, e.g.
 * ActiveNodesCache.
 */
class CacheManager : public GlobalCache
{
public:
    CacheManager(unsigned long byteSizeOfCache = 134217728){
    	aCache = new ActiveNodesCache(byteSizeOfCache);
    }
    virtual ~CacheManager(){
    	delete aCache;
    }

    ActiveNodesCache * getActiveNodesCache();

private:
    ActiveNodesCache * aCache;

};



}
}

#endif /* __CACHEMANAGER_H__ */
