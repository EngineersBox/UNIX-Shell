#include "builtin.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "error.h"
#include "path.h"

// NOTE: This is in alphabetical order to allow for binary search over commands
BuiltIn built_in_commands[] = {
	{"cd", builtin_cd},
	{"exit", builtin_exit},
	{"path", builtin_path},
	{NULL, NULL}
}; // TODO: Allow abitrary registration of commands

size_t built_in_commands_size = 3;

int builtin_cd(char** args, size_t argCount) {
	if (argCount == 0 || argCount > 1) {
		return EINVAL;
	}
	if (chdir(args[0])) {
		return errno;
	}
	return 0;
}

int builtin_exit(char** args, size_t argCount) {
	if (argCount > 0) {
		return EINVAL;
	}
	exit(0);
}

int builtin_path(char** args, size_t argCount) {
	if (argCount == 0) {
		path_clear();
		return 0;
	}
	int ret = path_add(args, argCount);
	if (ret != 0) {
		return errno;
	}
	return 0;
}

static BuiltIn* builtin_binary_search(char* command) {
	int lower = 0;
	int mid;
	int upper = built_in_commands_size - 1;
	int ret;
	while (lower <= upper) {
		mid = (lower + upper) / 2;
		BuiltIn* builtIn = &built_in_commands[mid];
		ret = strcmp(builtIn->name, command);
		if (ret == 0) {
			return builtIn;
		} else if (ret > 0) {
			upper = mid - 1;
		} else {
			lower = mid + 1;
		}
	}
	return NULL;
}

int builtin_execv(char* command, char** args, size_t argCount) {
	BuiltIn* cmd = builtin_binary_search(command);
	if (cmd != NULL) {
		return cmd->command(args, argCount);
	}
	return -1;
}
