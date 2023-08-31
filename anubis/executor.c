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

int exec_child(Command* command, int selfPipe[2]) {
	close(selfPipe[READ_PORT]);
	fprintf(stderr, "Executing: %s %s %s\n", command->command, command->args[0], command->args[1]);
	char* resolved = resolve(command->command);
	if (resolved == NULL) {
		fprintf(stderr, "Failed to resolve in path: %s\n", command->command);
		self_pipe_send(selfPipe, ENOENT);
		exit(0);
	}
	checked_free(command->command);
	command->command = resolved;
	command->args[0] = resolved;
	fprintf(stderr, "Resolved: %s %s %s\n", command->command, command->args[0], command->args[1]);
	execv(command->command, command->args);
	self_pipe_send(selfPipe, errno);
	exit(0);
}

int execute_command_line(CommandLine* line) {
	INSTANCE_NULL_CHECK_RETURN("CommandLine", line, 1);
	if (line->pipeCount == 1) {
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
	int tmpin = dup(STDIN_FILENO);
	int tmpout = dup(STDOUT_FILENO);

	// Set initial input
	int fdin;
	if (infile != NULL) {
		fdin = open(infile, O_RDONLY);
	} else {
		fdin = dup(tmpin);
	}

	int ret;
	int fdout;
	int err = 0;
	for (int i = 0; i < line->pipeCount; i++) {
		// Redirect input
		dup2(fdin, STDIN_FILENO);
		close(fdin);

		// Setup output
		if (i == line->pipeCount - 1) {
			if (outfile != NULL) {
				fdout = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
			} else {
				fdout = dup(tmpout);
			}
		} else {
			// Not last command (piped)
			// Create a pipe
			int pipes_[2];
			pipe(pipes_);
			fdout = pipes_[WRITE_PORT];
			fdin = pipes_[READ_PORT];
		}
		
		// Redirect output
		dup2(fdout, STDOUT_FILENO);
		close(fdout);

		// Self pipe for error comms trick: https://lkml.org/lkml/2006/7/10/300
		int selfPipe[2];
		ret = self_pipe_new(selfPipe);
		if (ret) {
			ERROR(ret, "Could not create selfPipe: %s\n", strerror(ret));
			return ret;
		}
		ret = fork();
		if (ret == 0) {
			// Create child process
			exec_child(line->pipes[i], selfPipe);
		}
		if (self_pipe_poll(selfPipe, &err)) {
			ERROR(err, "%s: %s\n", line->pipes[i]->command, strerror(err));
			close(selfPipe[READ_PORT]);
			return err;
		}
		ret = self_pipe_free(selfPipe);
		if (ret) {
			ERROR(ret, "Unable to free selfPipe: %s\n", strerror(ret));
			return ret;
		}
	}

	// Restore in/out defaults
	dup2(tmpin, STDIN_FILENO);
	dup2(tmpout, STDOUT_FILENO);
	close(tmpin);
	close(tmpout);

	if (!line->bgOp) {
		// Wait for last command
		waitpid(ret, NULL, 0);
	}

	return 0;
}

int execute(CommandTable* table) {
	INSTANCE_NULL_CHECK_RETURN("CommandTable", table, 0);
	int ret;
	for (int i = 0; i < table->lineCount; i++) {
		if ((ret = execute_command_line(table->lines[i])) != 0) {
			return ret;
		}
	}
	return 1;
}
