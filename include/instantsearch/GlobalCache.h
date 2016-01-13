
#ifndef __GLOBALCACHE_H__
#define __GLOBALCACHE_H__

namespace srch2
{
namespace instantsearch
{
// TODO: Add cache module and move to internal
class GlobalCache
{
public:
    static GlobalCache* create(unsigned long byteSizeOfCache, unsigned noOfCacheEntries);
    virtual ~GlobalCache() { };

    virtual int clear() = 0;
};

}}

#endif //__GLOBALCACHE_H__
