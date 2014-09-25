#include "io.h"

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
