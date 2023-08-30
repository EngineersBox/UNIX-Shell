#include "executor.h"

#include "checks.h"
#include "path.h"
#include "memutils.h"

#include <stddef.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define READ_PORT 0
#define WRITE_PORT 1

int exec_child(Command* command) {
	fprintf(stderr, "Executing: %s %s %s\n", command->command, command->args[0], command->args[1]);
	char* resolved = resolve(command->command);
	if (resolved == NULL) {
		ERROR(EINVAL, "No such file or directory: %s\n", command->command);
		return 0;
	}
	checked_free(command->command);
	command->command = resolved;
	command->args[0] = resolved;
	fprintf(stderr, "Resolved: %s %s %s\n", command->command, command->args[0], command->args[1]);
	execv(command->command, command->args);
	perror("execvp");
	exit(0);
}

int execute_command_line(CommandLine* line) {
	INSTANCE_NULL_CHECK_RETURN("CommandLine", line, 0);
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

		ret = fork();
		if (ret == 0) {
			// Create child process
			exec_child(line->pipes[i]);
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

	return 1;
}

int execute(CommandTable* table) {
	INSTANCE_NULL_CHECK_RETURN("CommandTable", table, 0);
	int ret;
	for (int i = 0; i < table->lineCount; i++) {
		if ((ret = execute_command_line(table->lines[i])) != 1) {
			return ret;
		}
	}
	return 1;
}
