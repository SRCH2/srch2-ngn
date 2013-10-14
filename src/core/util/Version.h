#ifndef __CORE_UTIL_VERSION_H__
#define __CORE_UTIL_VERSION_H__

#define ENGINE_VERSION "4.0.0"
#define INDEX_VERSION 1 // Increment this every time we make some changes to indexes
#include <string>
/**
 *  Helper class for version system. 
 */

class Version{
    public:
        static const std::string getCurrentVersion() {
            return ENGINE_VERSION;
        }
};        

#endif 
