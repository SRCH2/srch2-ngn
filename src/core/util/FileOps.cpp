#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include "FileOps.h"
#include "Logger.h"

namespace srch2 {
namespace util {

// Find the longest path of a file
string getFilePath(string fullPathFileName)
{
    int found = fullPathFileName.rfind("/");
    if(found != -1)
        return fullPathFileName.substr(0, found);
    else
        return "";
}

bool checkDirExistence(const char *dirName)
{
	return access(dirName, F_OK) == 0;
}

int createDir(const char *pathName)
{
    char dirName[PATH_MAX];

    if (pathName[0] == '\000') {
        Logger::error("Null directory path - cannot create");
        return -1;
    }
    strncpy(dirName, pathName, PATH_MAX-1);
    unsigned len = strlen(dirName);
    if (dirName[len-1]!='/')
        strcat(dirName,   "/");
    len = strlen(dirName);
    for (int i=1; i<len; i++){
		if (dirName[i]=='/'){
			dirName[i] = 0;
			if (!checkDirExistence(dirName)){
				if(mkdir(dirName, 0755) == -1){
					Logger::error("mkdir %s fail.", dirName);
					return -1;
				}
			}
			dirName[i] = '/';
		}
	}

    return 0;
}

}
}
