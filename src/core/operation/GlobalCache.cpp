
#include "CacheManager.h"

namespace srch2
{
namespace instantsearch
{

GlobalCache* GlobalCache::create(unsigned long byteSizeOfCache, unsigned noOfCacheEntries)
{
    return dynamic_cast<GlobalCache*>( new CacheManager(byteSizeOfCache) );
}

}}
