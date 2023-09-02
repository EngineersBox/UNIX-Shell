#ifndef ANUBIS_BUILTIN_H
#define ANUBIS_BUILTIN_H

#include <stddef.h>

typedef int (*BuiltinCmd)(char**, size_t);

typedef struct BuiltIn {
	const char *name;
	BuiltinCmd command;
} BuiltIn;

extern BuiltIn built_in_commands[];
extern size_t built_in_commands_size;

int builtin_cd(char** args, size_t argCount);
int builtin_exit(char** args, size_t argCount);
int builtin_path(char** args, size_t argCount);

// -1: No matching command, Otherwise: command found and executed with return value
int builtin_execv(char* command, char** args, size_t argCount);

#endif // ANUBIS_BUILTIN_H
