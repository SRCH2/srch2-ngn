#ifndef __CORE_UTIL_VERSION_H__
#define __CORE_UTIL_VERSION_H__

#define CURRENT_VERSION "3.1.2"

/**
 *  Helper class for version system. 
 */

class Version{
    public:
        static const string getCurrentVersion() {
            return CURRENT_VERSION;
        }
};        

#endif 
