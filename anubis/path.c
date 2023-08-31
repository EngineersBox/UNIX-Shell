#include "path.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>

#include "error.h"
#include "checks.h"
#include "mem_utils.h"

#define PATH_DELIMITER '/'

#define INITIAL_PATH_LEN 4
static char* initialPath = "/bin";

char* path;
static size_t pathLen;

int path_init() {
	path = calloc(INITIAL_PATH_LEN + 1, sizeof(*path));
	INSTANCE_NULL_CHECK_RETURN("path", path, 1);
	path[INITIAL_PATH_LEN] = '\0';
	strncpy(path, initialPath, INITIAL_PATH_LEN);
	pathLen = INITIAL_PATH_LEN;
	return 0;
}

void path_clear() {
	memset(path, '\0', INITIAL_PATH_LEN);
	pathLen = 0;
}

int path_add(char** paths, size_t count) {
	for (size_t i = 0; i < count; i++) {
		char* newPath = paths[i];
		size_t newPathLen = strlen(newPath);
		size_t offset = pathLen > 0;
		path = realloc(path, (pathLen + newPathLen + 1 + offset) * sizeof(*path));
		INSTANCE_NULL_CHECK_RETURN("path", path, errno);
		strncpy(&path[pathLen + offset], newPath, newPathLen);
		if (pathLen) {
			path[pathLen] = PATH_ENV_DELIMITER[0];
		}
		path[pathLen + newPathLen + offset] = '\0';
		pathLen += newPathLen + offset;
	}
	return 0;
}

void path_free() {
	checked_free(path);
}

static char* join_paths(const char* dir, const char* file) {
	int dirLen = strlen(dir);
	int pathLen = dirLen + strlen(file) + 2;
	char* newPath = calloc(pathLen, sizeof(*newPath));
	if (newPath == NULL) {
		ERROR(errno, "Unable to allocate buffer to join paths\n");
		return NULL;
	}
	strcpy(newPath, dir);
	if (dir[dirLen - 1] != PATH_DELIMITER) {
		char delim[2] = "\0";
		delim[0] = PATH_DELIMITER;
		strcat(newPath, delim);
	}
	// Will never have a slash in it, due to early return condition for resolution via is_path(char*)
	strcat(newPath, file);
	return newPath;
}

static char* filename = NULL;

static int directory_filter(const struct dirent* dir) {
	if (!dir) {
		return 0;
	} else if (dir->d_type != DT_REG) {
		return 0;
	} else if (strcmp(dir->d_name, filename) == 0) {
		return 1;
	}
	return 0;
}

static char* resolve_within_directory(char* searchable, char* dirPath) {
	struct dirent** dirEntries;
	filename = searchable;
	int n = scandir(dirPath, &dirEntries, directory_filter, alphasort);
	filename = NULL;
	bool found = n > 0;
	while (found && n--) {
		free(dirEntries[n]);
	}
	free(dirEntries);
	if (!found) {
		return NULL;
	}
	return join_paths(dirPath, searchable);
}

static bool is_path(char* searchable) {
	if (searchable == NULL) {
		return false;
	}
	char* ptr = strchr(searchable, PATH_DELIMITER);
	if (ptr == NULL) {
		return false;
	} else if ((uintptr_t) ptr == (uintptr_t) searchable) {
		return true;
	} else if (*(ptr - 1) == '\\') {
		return is_path(ptr + 1);
	}
	return true;
}

char* path_resolve(char* executable) {
	if (is_path(executable)) {
		return strdup(executable);
	}
	char* searchable = strtok(path, PATH_ENV_DELIMITER);
	while (searchable != NULL) {
		struct stat	sstat;
		if (stat(searchable, &sstat) != 0) {
			// Skip if we can't resolve the path
			goto next;
		} else if (S_ISDIR(sstat.st_mode)) {
			char* resolved = resolve_within_directory(executable, searchable);
			if (resolved != NULL && access(resolved, F_OK | X_OK) == 0) {
				return resolved;
			}
			goto next;
		}
		char* slash = strrchr(searchable, PATH_DELIMITER);
		if (slash && strcmp(slash, searchable) && access(searchable, F_OK | X_OK) == 0) {
			// File name at searchable path matches given executable name
			return searchable;
		}
next:
		// Tokenise from previously found offset
		searchable = strtok(NULL, PATH_ENV_DELIMITER);
	}
	return NULL;
}
