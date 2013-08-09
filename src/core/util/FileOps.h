#ifndef __INSTANTSEARCH__UTIL_FILEOPS_H__
#define __INSTANTSEARCH__UTIL_FILEOPS_H__
#include <string>
using std::string;
namespace srch2 {
namespace util {

string getDir(string file);
bool checkDirExistence(const char *dirName);
int createDir(const char *pathName);

}
}

#endif
