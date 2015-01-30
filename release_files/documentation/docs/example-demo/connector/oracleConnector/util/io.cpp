#include "io.h"

#include <sstream>
#include <stdlib.h>

bool checkDirExisted(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (st.st_mode & S_IFDIR) {
            return true;
        }
    }
    return false;
}

bool checkFileExisted(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return true;
    }
    return false;
}

std::vector<std::string> &splitString(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}
