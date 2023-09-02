#include "executor.h"

#include <stddef.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "checks.h"
#include "path.h"
#include "mem_utils.h"
#include "builtin.h"
#include "math_utils.h"
#include "self_pipe.h"

#define READ_PORT 0
#define WRITE_PORT 1

typedef struct IO {
	int in;
	int out;
} IO;

static IO io_new() {
	return (IO) { 0 };
}

static IO stdio_save() {
	return (IO) {
		dup(STDIN_FILENO),
		dup(STDOUT_FILENO)
	};
}

inline static int io_restore(IO* stdio) {
	//fprintf(stderr, "RESTORE: %d -> %d\n", stdio->in, STDIN_FILENO);
	errno_return(dup2(stdio->in, STDIN_FILENO), -1, "Unable to duplicate STDIN");
	//fprintf(stderr, "CLOSED: %d\n", STDIN_FILENO);
	//fprintf(stderr, "RESTORE: %d -> %d\n", stdio->out, STDOUT_FILENO);
	errno_return(dup2(stdio->out, STDOUT_FILENO), -1, "Unable to duplicate STDOUT");
	//fprintf(stderr, "CLOSED: %d\n", STDOUT_FILENO);
	errno_return(close(stdio->in), -1, "Unable to close STDIN");
	//fprintf(stderr, "CLOSED: %d\n", stdio->in);
	errno_return(close(stdio->out), -1, "Unable to close STDOUT");
	//fprintf(stderr, "CLOSED: %d\n", stdio->out);
	return 0;
}

inline static int redirect(int fd, int std) {
	errno_return(dup2(fd, std), -1, "Unable to redirect %d -> %d", fd, std,);
	//fprintf(stderr, "REDIRECT: %d -> %d\n", fd, std);
	errno_return(close(fd), -1, "Unable to close %d", fd,);
	//fprintf(stderr, "CLOSED: %d\n", fd);
	return 0;
}

static int exec_child(Command* command, int selfPipe[2]) {
	close(selfPipe[READ_PORT]);
	char* resolved = path_resolve(command->command);
	if (resolved == NULL) {
		self_pipe_send(selfPipe, ENOENT);
		exit(0);
	}
	checked_free(command->command);
	command->command = command->args[0] = resolved;
	execv(command->command, command->args);
	self_pipe_send(selfPipe, errno);
	exit(0);
}

static int configure_output(bool isLast, IO* stdio, IO* fileio, char* outfile) {
	if (isLast) {
		if (outfile != NULL) {
			fileio->out = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
		} else {
			fileio->out = dup(stdio->out);
		}
		return 0;
	}
	// Not last command (piped)
	// Create a pipe
	int pipes[2];
	errno_return(pipe(pipes), -1, "Unable to construct pipe to connect commands");
	fileio->out = pipes[WRITE_PORT];
	fileio->in = pipes[READ_PORT];
	return 0;
}

static int execute_command_line(CommandLine* line) {
	INSTANCE_NULL_CHECK_RETURN("CommandLine", line, 1);
	if (line->pipeCount == 1) {
		// TODO: These builtins should not be treated any differently from regular executables.
		//       Refactor these to be invoked as child procs which can access stdin/stdout within
		//       a chain of pipes.
		Command* command = line->pipes[0];
		size_t argCount = DEC_FLOOR(DEC_FLOOR(command->argCount));
		int ret = builtin_execv(
			command->command,
			argCount == 0 ? NULL : &command->args[1],
			argCount
		);
		if (ret != -1) {
			return ret;
		}
	}

	// Command structure
	char* infile = NULL; // NOTE: Always null, we only support outfiles currently
	char* outfile = NULL;
	if (line->modifiersCount > 0) {
		outfile = line->ioModifiers[0]->target;
	}

	// Save stdin/stdout
	IO stdio = stdio_save();
	//fprintf(stderr, "SAVED: %d, %d\n", stdio.in, stdio.out);
	IO fileio = io_new();

	// Set initial input
	if (infile != NULL) {
		fileio.in = open(infile, O_RDONLY);
	} else {
		fileio.in = dup(stdio.in);
	}

	int ret;
	int err = 0;
	for (int i = 0; i < line->pipeCount; i++) {
		// Redirect input
		transparent_return(redirect(fileio.in, STDIN_FILENO));

		// Setup output
		transparent_return(configure_output(
			i == line->pipeCount -1,
			&stdio, &fileio,
			outfile
		));
		
		// Redirect output
		transparent_return(redirect(fileio.out, STDOUT_FILENO));

		int selfPipe[2];
		ret_return(self_pipe_new(selfPipe), != 0, "Unable to create selfPipe");
		ret = fork();
		if (ret == 0) {
			// Create child process
			exec_child(line->pipes[i], selfPipe);
		}
		if (self_pipe_poll(selfPipe, &err)) {
			ERROR(err, "%s", line->pipes[i]->command);
			close(selfPipe[READ_PORT]);
			return err;
		}
		ret_return(self_pipe_free(selfPipe), != 0, "Unable to free selfPipe");
	}

	if (!line->bgOp) {
		// Wait for commands
		while (wait(NULL) >= 0);
	}

	// Restore in/out defaults
	transparent_return(io_restore(&stdio));

	return 0;
}

int execute(CommandTable* table) {
	INSTANCE_NULL_CHECK_RETURN("CommandTable", table, 0);
	int ret;
	for (int i = 0; i < table->lineCount; i++) {
		if ((ret = execute_command_line(table->lines[i]))) {
			return ret;
		}
	}
	return 0;
}
