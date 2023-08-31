#ifndef ANUBIS_PATH_H
#define ANUBIS_PATH_H

#include <stddef.h>

#define PATH_ENV_DELIMITER ":"

extern char* path;

int path_init();
void path_clear();
int path_add(char** paths, size_t count);
void path_free();

char* resolve(char* executable);

#endif // ANUBIS_PATH_H
