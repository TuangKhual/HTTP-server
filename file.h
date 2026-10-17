#pragma once

#include "stringFuncs.h"
#include <linux/limits.h>
#include <sys/stat.h>
#include <stdbool.h>

typedef struct {
    bool exists;
    size_t size;
} fileData;

fileData getFileData(stringView filename){ // handles opening directories and other types not just using fopen()
    char buffer[PATH_MAX];
    fileData data;
    struct stat st;

    data.exists = false;
    if (filename.len > PATH_MAX) {
        return data;
    }
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, filename.data, filename.len);
    if (stat(buffer, &st) < 0) {
        return data;
    }
    data.size = st.st_size;
    data.exists = true;
    return data;
}