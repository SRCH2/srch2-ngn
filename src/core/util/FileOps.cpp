/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
