//$Id$

#include "Evaluate.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "util/Logger.h"

namespace srch2{
namespace util{

int getRAMUsageValue(){
	FILE* file = fopen("/proc/self/status", "r");
	int result = -1;
	if (file == NULL) {
		Logger::error("File %s open failed", "/proc/self/status");
		return result;
	}
	char line[128];
	while (fgets(line, 128, file) != NULL) {
		if (strncmp(line, "VmRSS:", 6) == 0) {
            // parseLine
            char* pline = line;
            int i = strlen(pline);
        	while (*pline < '0' || *pline > '9'){
	        	pline++;
            }
            pline[i - 3] = '\0';
            i = atoi(pline);
            result = i;
			break;
		}
	}
	fclose(file);
	return result;
}


}}

