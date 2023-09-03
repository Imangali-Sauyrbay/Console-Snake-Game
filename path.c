#include <libgen.h>
#include <stdio.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#include "path.h"

char currentDir[PATH_MAX];

char* getCurrentDir() {
    char file_path[PATH_MAX];

    ssize_t len = readlink("/proc/self/exe", file_path, sizeof(file_path) - 1);
    
    if (len == -1) {
        perror("Error getting the path of the executable file");
        return NULL;
    }

    file_path[len] = '\0';

    return dirname(file_path);
}

void relToAbsPath(const char* relPath, char* absPath, size_t absPathSize) {
    snprintf(absPath, absPathSize, "%s/%s", currentDir, relPath);
}