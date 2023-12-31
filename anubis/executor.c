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
#include "visibility.h"

#define READ_PORT 0
#define WRITE_PORT 1
#define FAIL_COND -1

typedef struct IO {
	int in;
	int out;
} IO;

__attribute__((hot))
LINKAGE_PRIVATE IO io_new() {
	return (IO) { 0 };
}

__attribute__((hot))
LINKAGE_PRIVATE IO stdio_save() {
	return (IO) {
		dup(STDIN_FILENO),
		dup(STDOUT_FILENO)
	};
}

__attribute__((hot))
LINKAGE_PRIVATE int io_restore(IO* stdio) {
	errno_return(dup2(stdio->in, STDIN_FILENO), FAIL_COND, "Unable to duplicate STDIN");
	errno_return(dup2(stdio->out, STDOUT_FILENO), FAIL_COND, "Unable to duplicate STDOUT");
	errno_return(close(stdio->in), FAIL_COND, "Unable to close STDIN");
	errno_return(close(stdio->out), FAIL_COND, "Unable to close STDOUT");
	return 0;
}

__attribute__((hot))
LINKAGE_PRIVATE int redirect(int fd, int std) {
	errno_return(dup2(fd, std), FAIL_COND, "Unable to redirect %d -> %d", fd, std);
	errno_return(close(fd), FAIL_COND, "Unable to close %d", fd);
	return 0;
}

__attribute__((hot, noreturn))
LINKAGE_PRIVATE int exec_child(Command* command, int selfPipe[2]) {
	if (close(selfPipe[READ_PORT])) {
		ERROR(errno, "Unabe to close self pipe read port from child");
		exit(errno);
		__builtin_unreachable();
	}
	char* resolved = path_resolve(command->command);
	if (resolved == NULL) {
		self_pipe_send(selfPipe, ENOENT);
		exit(0);
		__builtin_unreachable();
	}
	checked_free(command->command);
	command->command = resolved;
	execv(command->command, command->args);
	self_pipe_send(selfPipe, errno);
	exit(0);
	__builtin_unreachable();
}

__attribute__((hot))
LINKAGE_PRIVATE int configure_input(char* infile, IO* stdio, IO* fileio) {
	if (infile != NULL) {
		if ((fileio->in = open(infile, O_RDONLY)) == FAIL_COND) {
			return errno;
		}
	} else if ((fileio->in = dup(stdio->in)) == FAIL_COND) {
		return errno;
	}
	return 0;
}

__attribute__((hot))
LINKAGE_PRIVATE int configure_output(bool isLast, IO* stdio, IO* fileio, char* outfile) {
	if (!isLast) {
		// Not last command (piped)
		// Create a pipe
		int pipes[2];
		errno_return(pipe(pipes), -1, "Unable to construct pipe to connect commands");
		/* NOTE: In theory you could do zero-copy between processes to avoid buffered IPC that
		 *       is the standard pipe(...) way. Something like this: first create 2 common files
		 *       to use between processes and then mmap() them into memory progressizely. First
		 *       one is mapped for the previous process then when the next process starts it maps
		 *       its receiver file into mem (these ping-pong between sequential processes). On
		 *       the writing side (previous proc) map it over STDOUT and vmsplice(...) it through tee
		 *       to a pipe, then on the reading proc splice(...) it into the receiving memory area.
		 *       Here, vmsplice(...) should be configured with SPLICE_F_GIFT to allow for pages to
		 *       be released back to the kernel and then ownership transfered to the reading proc.
		 *       Using tee means that page references are copied between the procs without needing to
		 *       remap addresses once the recieveing proc has got a hold of the addresses for the page(s).
		 *       Downside of this is that since we are gifting pages, we can technically write an
		 *       infinite number and consume all memory space since the kernel will only GC the yielded
		 *       pages once the reading proc has released them. So the reading proc would need to
		 *       consume and release the pages at the same rate or quicker than the writing proc.
		 *       Given that we are saving on data copy by doing zero-copy through page ownship transfer
		 *       through the kernel, this could very well be less efficient for small data sizes.
		 *       Also the page cache might take a beating if pages are not released fast enough since
		 *       it would be filled with pages that are held by the kernel but not used by the reading
		 *       proc yet, or yielded for GC. Definitely some issues to consider before a full
		 *       implementation is possible.
		 */
		fileio->out = pipes[WRITE_PORT];
		fileio->in = pipes[READ_PORT];
	} else if (outfile != NULL) {
		if ((fileio->out = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) == FAIL_COND) {
			return errno;
		}
	} else if ((fileio->out = dup(stdio->out)) == FAIL_COND) {
		return errno;	
	}
	return 0;
}

__attribute__((hot))
LINKAGE_PRIVATE int invoke_builtin_checked(Command* command) {
	size_t argCount = DEC_FLOOR(DEC_FLOOR(command->argCount));
	return builtin_execv(
		command->command,
		argCount == 0 ? NULL : &command->args[1],
		argCount
	);
}

__attribute__((hot))
LINKAGE_PRIVATE int execute_command_line(CommandLine* line) {
	INSTANCE_NULL_CHECK_RETURN("CommandLine", line, 1);
	// Command structure
	char* infile = NULL; // NOTE: Always null, we only support outfiles currently
	char* outfile = checked_invocation(strdup, line->ioModifiers->outTrunc);
	// Save stdin/stdout
	IO stdio = stdio_save();
	IO fileio = io_new();
	// Setup input
	int ret;
	transparent_return(configure_input(infile, &stdio, &fileio));
	int err = 0;
	for (int i = 0; i < line->pipeCount; i++) {
		// Redirect input
		transparent_return(redirect(fileio.in, STDIN_FILENO));
		// Setup output
		transparent_return(configure_output(
			i == line->pipeCount - 1,
			&stdio, &fileio,
			outfile
		));
		// Redirect output
		transparent_return(redirect(fileio.out, STDOUT_FILENO));
		// Invoke builtins conditonally (allows for piped usage)
		Command* command = line->pipes[i];
		err = invoke_builtin_checked(command);
		if (err == 0) {
			continue;
		} else if (err != FAIL_COND) {
			ERROR(err, "%s", command->command);
			break;
		}
		int selfPipe[2];
		ret_return(self_pipe_new(selfPipe), != 0, "Unable to create selfPipe");
		if ((ret = fork()) == 0) {
			// Create child process
			exec_child(command, selfPipe);
		} else if (ret == FAIL_COND) {
			err = errno;
			ERROR(err, "Failed child fork");
			break;
		}
		// Await error byte or close-on-exec
		if (self_pipe_poll(selfPipe, &err)) {
			ERROR(err, "%s", command->command);
			if (close(selfPipe[READ_PORT])) {
				ERROR(errno, "Unable to close self pipe read port");
				// Allow fall through on double failure to capture original error not secondary close failure
			}
			break;
		}
		// Free resources
		ret_return(self_pipe_free(selfPipe), != 0, "Unable to free selfPipe");
		// Reset error for default success
		err = 0;
	}
	// Restore in/out defaults
	transparent_return(io_restore(&stdio));
	if (!line->bgOp) {
		// Wait for commands if last command in foreground
		while (wait(NULL) >= 0);
	}
	return err;
}

__attribute__((hot))
LINKAGE_PUBLIC int execute(CommandTable* table) {
	INSTANCE_NULL_CHECK_RETURN("CommandTable", table, 0);
	int ret;
	for (int i = 0; i < table->lineCount; i++) {
		transparent_return(execute_command_line(table->lines[i]));
	}
	return 0;
}
