#ifndef _CORE_UTIL_VERSION_H_
#define _CORE_UTIL_VERSION_H_

#define CURRENT_VERSION "3.1.0"

/**
 *  Helper class for version system. 
 */

class Version{
    public:
        static const string getCurrentVersion() {
            return CURRENT_VERSION;
        }
}        

#endif 
