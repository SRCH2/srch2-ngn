#ifndef __INSTANTSEARCH_UTIL_FILEOPS_H__
#define __INSTANTSEARCH_UTIL_FILEOPS_H__
#include <string>
using std::string;
namespace srch2 {
namespace util {

string getFilePath(string fullPathFileName);
bool checkDirExistence(const char *dirName);
int createDir(const char *pathName);

}
}

#endif
