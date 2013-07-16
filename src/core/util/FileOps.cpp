#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include "FileOps.h"
#include "Logger.h"

namespace srch2 {
namespace util {

bool checkDirExistence(const char *dirName)
{
	return access(dirName, NULL) == 0;
}

int createDir(const char *pathName)
{
    char dirName[PATH_MAX];
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
