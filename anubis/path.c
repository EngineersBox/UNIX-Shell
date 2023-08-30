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

#define PATH_ENV_DELIMITER ":"
#define PATH_DELIMITER '/'

static char* get_default_path (void) {
	size_t len = confstr (_CS_PATH, NULL, 0);
	char* buffer = (char*) malloc (len);
	if (confstr (_CS_PATH, buffer, len + 1) == 0){
		free (buffer);
		return NULL;
	}
	return buffer;
}

static char* join_paths(const char* dir, const char* file) {
	int dirLen = strlen(dir);
	int pathLen = dirLen + strlen(file) + 2;
	char* path = calloc(pathLen, sizeof(*path));
	if (path == NULL) {
		ERROR(errno, "Unable to allocate buffer to join paths\n");
		return NULL;
	}
	strcpy(path, dir);
	if (dir[dirLen - 1] != PATH_DELIMITER) {
		char delim[2] = "\0";
		delim[0] = PATH_DELIMITER;
		strcat(path, delim);
	}
	// Will never have a slash in it, due to early return condition for resolution
	strcat(path, file);
	return path;
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
	while (n--) {
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

char* resolve(char* executable) {
	// TODO: If executable has a slash in it, just return the executable
	if (is_path(executable)) {
		return strdup(executable);
	}
	char* path = getenv("PATH");
	if (path == NULL) {
		path = get_default_path();
	}
	char* searchable = strtok(path, PATH_ENV_DELIMITER);
	while (searchable != NULL) {
		struct stat	sstat;
		if (stat(searchable, &sstat) != 0) {
			goto next; // Skip if we can't resolve the path
		} else if (S_ISDIR(sstat.st_mode)) {
			printf("Directory: %s\n", searchable);
			char* resolved = resolve_within_directory(executable, searchable);
			if (resolved != NULL) {
				return resolved;
			}
			goto next;
		}
		char* slash = strrchr(searchable, PATH_DELIMITER);
		if (slash && strcmp(slash, searchable)) {
			return searchable; // File name at searchable path matches given executable name
		}
next:
		searchable = strtok(NULL, PATH_ENV_DELIMITER);
	}
	return NULL;
}
