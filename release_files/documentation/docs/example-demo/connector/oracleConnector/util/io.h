#ifndef __CONNECTOR_UTIL_IO__
#define __CONNECTOR_UTIL_IO__
#include <sys/stat.h>
#include <vector>
#include <string>

bool checkDirExisted(const char* path);
bool checkFileExisted(const char* path);
std::vector<std::string> &splitString(const std::string &s, char delim,
        std::vector<std::string> &elems);
#endif
