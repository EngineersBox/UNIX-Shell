#include "builtin.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "error.h"
#include "path.h"

BuiltIn built_in_commands[] = {
	{"cd", builtin_cd},
	{"exit", builtin_exit},
	{"path", builtin_path},
	{NULL, NULL}
}; // TODO: Allow abitrary registration of commands

int builtin_cd(char** args, size_t argCount) {
	if (argCount == 0 || argCount > 1) {
		ERROR(EINVAL, "command 'cd' expects 1 command, got %d", argCount);
		return EINVAL;
	}
	if (chdir(args[0]) != 0) {
		ERROR(errno, "cd:");
		return errno;
	}
	return 0;
}

int builtin_exit(char** args, size_t argCount) {
	if (argCount > 0) {
		ERROR(EINVAL, "command 'exit' expects no arguments, got %d", argCount);
		return EINVAL;
	}
	// TODO: Invoke a shutdown hook to clean up resources from main method
	exit(0);
}

int builtin_path(char** args, size_t argCount) {
	if (argCount == 0) {
		path_clear();
		return 0;
	}
	int ret = path_add(args, argCount);
	if (ret != 0) {
		ERROR(errno, "path:");
		return errno;
	}
	return 0;
}

int builtin_execv(char* command, char** args, size_t argCount) {
	BuiltIn* cmd = built_in_commands;
	do {
		if (strcmp(cmd->name, command) == 0) {
			return cmd->command(args, argCount);
		}
	} while ((++cmd)->name != NULL);
	return -1;
}
