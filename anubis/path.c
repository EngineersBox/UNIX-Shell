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
#include "visibility.h"

#define PATH_DELIMITER '/'

#define INITIAL_PATH_LEN 4
static char* initialPath = "/bin";

char* path;
static size_t pathLen;

LINKAGE_PUBLIC int path_init() {
	path = calloc(INITIAL_PATH_LEN + 1, sizeof(*path));
	INSTANCE_NULL_CHECK_RETURN("path", path, 1);
	path[INITIAL_PATH_LEN] = '\0';
	strncpy(path, initialPath, INITIAL_PATH_LEN);
	pathLen = INITIAL_PATH_LEN;
	return 0;
}

LINKAGE_PUBLIC void path_clear() {
	if (pathLen < 1) {
		return;
	}
	memset(path, '\0', INITIAL_PATH_LEN);
	pathLen = 0;
}

LINKAGE_PUBLIC int path_add(char** paths, size_t count) {
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

LINKAGE_PUBLIC void path_free() {
	checked_free(path);
}

LINKAGE_PRIVATE char* join_paths(const char* dir, const char* file) {
	int dirLen = strlen(dir);
	int pathLen = dirLen + strlen(file) + 2;
	char* newPath = calloc(pathLen, sizeof(*newPath));
	verrno_return(newPath, NULL, "Unable to allocate buffer to join paths");
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

#define NON_EXEC_TARGET(entry) (\
	(entry) != DT_REG\
	&& (entry) != DT_LNK\
	&& (entry) != DT_CHR\
	&& (entry) != DT_UNKNOWN\
	)

LINKAGE_PRIVATE int directory_filter(const struct dirent* dir) {
	if (!dir || NON_EXEC_TARGET(dir->d_type) || strcmp(dir->d_name, filename)) {
		return 0;
	}
	if (dir->d_type == DT_LNK || dir->d_type == DT_UNKNOWN) {
		struct stat sstat;
		return stat(filename, &sstat) && (S_ISREG(sstat.st_mode) || S_ISCHR(sstat.st_mode));
	}
	return 1;
}

LINKAGE_PRIVATE char* resolve_within_directory(char* searchable, char* dirPath) {
	struct dirent** dirEntries;
	filename = searchable;
	int n = scandir(dirPath, &dirEntries, directory_filter, alphasort);
	filename = NULL;
	bool found = n > 0;
	if (found) {
		checked_array_free(dirEntries, n, free);
	}
	free(dirEntries);
	if (!found) {
		return NULL;
	}
	return join_paths(dirPath, searchable);
}

LINKAGE_PRIVATE bool is_path(char* searchable) {
	char* _searchable = searchable;
	while (true) {
		if (_searchable == NULL) {
			return false;
		}
		char* ptr = strchr(_searchable, PATH_DELIMITER);
		if (ptr == NULL) {
			// No slashes in string
			return false;
		} else if ((uintptr_t) ptr == (uintptr_t) _searchable) {
			// Slash found at the start of the string
			return true;
		} else if (*(ptr - 1) == '\\') {
			// Search over string beyond this escaped slash
			_searchable = ptr + 1;
			continue;
		}
		break;
	}
	return true;
}

LINKAGE_PUBLIC char* path_resolve(char* executable) {
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
				// Directory has an entry that matches the given executable name
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
