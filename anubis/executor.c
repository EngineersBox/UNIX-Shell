#include "executor.h"

#include "checks.h"

#include <stddef.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

// NOTE: When implementing path resolution note the following from `man execvp`, implementation should do the same:
// "If the specified filename includes a slash character, then PATH is ignored, and the file at the specified pathname is executed."

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
			int fdpipe[2];
			if (pipe(fdpipe) == -1) {
				perror("pipe");
			}
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}
		
		// Redirect output
		dup2(fdout, STDOUT_FILENO);
		close(fdout);

		ret = fork();
		if (ret == 0) {
			// Create child process
			fprintf(stderr, "Executing: %s %s %s\n", line->pipes[i]->command, line->pipes[i]->args[0], line->pipes[i]->args[1]);
			fprintf(stderr, "EXEC: %d\n", execvp(line->pipes[i]->command, line->pipes[i]->args));
			perror("execvp");
			exit(0);
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

#define CURRENT 0
#define PREVIOUS 1
#define READ 0
#define WRITE 1

static int pipes_available = 0;

static bool redirect_out(CommandLine* line) {
	if (line->modifiersCount == 0) {
		return true;
	}
	static int output;
	if (output) {
		close(output);
	}
	output = open(line->ioModifiers[0]->target, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	dup2(output, STDOUT_FILENO);
	close(output);
	return true;
}

static bool connect(int pipes[2][2], bool isLast, int index) {
	if (!pipes_available) {
		return true;
	}
	if (isLast || index != 0) {
		dup2(pipes[PREVIOUS][READ], STDIN_FILENO);
	}
	if (index == 0 || !isLast) {
		dup2(pipes[CURRENT][WRITE], STDOUT_FILENO);
	}
	return true;

}

static bool fork_pipe_redirect(CommandLine* line, int pipes[2][2], bool isLast, int index) {
	if (fork() == 0) {
		if (connect(pipes, isLast, index) && redirect_out(line)) {
			return true;
		}
	}
	return false;
}

void alternate(int** pipes) {
	int* current = pipes[CURRENT];
	pipes[CURRENT] = pipes[PREVIOUS];
	pipes[PREVIOUS] = current;
}

int execute_command_line2(CommandLine* line) {
	INSTANCE_NULL_CHECK_RETURN("CommandLine", line, 0);
	pipes_available = line->pipeCount > 1;
	int pipes[2][2];
	if (pipes_available && pipe(pipes[CURRENT]) == -1) {
		perror("pipe");
	} else if (pipes_available && pipe(pipes[PREVIOUS]) == -1) {
		perror("pipe");
	}
	for (int i = 0; i < line->pipeCount; i++) {
		if (fork_pipe_redirect(line, pipes, i == (line->pipeCount - 1), i)) {
			for (int j = 0; j < strlen(line->pipes[i]->command) + 1; j++) {
				fprintf(stderr, "%d\n", line->pipes[i]->command[j]);
			}
			for (int j = 0; j < strlen(line->pipes[i]->args[1]) + 1; j++) {
				fprintf(stderr, "%d\n", line->pipes[i]->args[1][j]);
			}
			execvp(line->pipes[i]->command + 1, line->pipes[i]->args);
			perror("execvp");
			exit(1);
		}
		if (pipes_available) alternate((int**) pipes);
	}
	if (!line->bgOp) {
		while (wait(NULL) >= 0);
	}
	if (pipes_available) {
		close(pipes[PREVIOUS][READ]);
		close(pipes[PREVIOUS][WRITE]);
	}
	return 0;
}

int execute(CommandTable* table) {
	INSTANCE_NULL_CHECK_RETURN("CommandTable", table, 0);
	int ret;
	for (int i = 0; i < table->lineCount; i++) {
		//if (fork() == 0) {
		//	char* args[3] = {"/bin/ls", "-la", NULL};
		//	execvp("/bin/ls", args);
		//}
		if ((ret = execute_command_line(table->lines[i])) != 1) {
			return ret;
		}
	}
	return 1;
}
