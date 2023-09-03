#include "self_pipe.h"

#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "visibility.h"

#define READ_PORT 0
#define WRITE_PORT 1

LINKAGE_PUBLIC int self_pipe_new(int selfPipe[2]) {
	if (pipe(selfPipe)) {
		return errno;
	} if (fcntl(selfPipe[WRITE_PORT], F_SETFD, fcntl(selfPipe[WRITE_PORT], F_GETFD) | FD_CLOEXEC)) {
		return errno;
	}
	return 0;
}

LINKAGE_PUBLIC int self_pipe_send(int selfPipe[2], int signal) {
	return write(selfPipe[WRITE_PORT], &signal, sizeof(int));
}

LINKAGE_PUBLIC int self_pipe_poll(int selfPipe[2], int* error) {
	int count = -1;
	close(selfPipe[WRITE_PORT]);
	while ((count = read(selfPipe[READ_PORT], error, sizeof(errno))) == -1) {
		if (errno != EAGAIN && errno != EINTR) {
			break;
		}
	}
	return count;
}

LINKAGE_PUBLIC int self_pipe_free(int selfPipe[2]) {
	return close(selfPipe[READ_PORT]);
}
