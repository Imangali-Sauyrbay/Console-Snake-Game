#ifndef PATH_H
#define PATH_H

#include <stdlib.h>

char* getCurrentDir();
void relToAbsPath(const char* relPath, char* absPath, size_t absPathSize);

#include <linux/limits.h>

extern char currentDir[PATH_MAX];

#endif