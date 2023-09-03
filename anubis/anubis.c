#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"
#include "structure.h"
#include "path.h"
#include "mem_utils.h"
#include "checks.h"

#define INTERACTIVE 1
#define BATCH 2

static bool initialised = false;
static Parser parser;
static CommandTable* table = NULL;
static Lexer* lexer = NULL;
static char* line = NULL;

static void exit_handler(void) {
	command_table_free(table);
	lexer_free(lexer);
	path_free();
	checked_free(line);
	while (wait(NULL) >= 0);
}

int shell_core(char* _line) {
	if (!initialised) {
		parser = parser_default();
		initialised = true;
	}
	lexer_free(lexer);
	lexer = lexer_new(_line);
	command_table_free(table);
	table = parse(&parser, lexer);
	if (table == NULL) {
		return 1;
	}
	int ret;
	//command_table_dump(table);
	if ((ret = execute(table))) {
		return ret;
	}
	return 0;
}

int next_line(char** _line, size_t* len, FILE* stream) {
	if (stream == stdin) {
		fprintf(stdout, "anubis> ");
	}
	return getline(_line, len, stream);
}

int shell_stream(int mode, char* filename) {
	FILE* stream = stdin;
	if (mode == BATCH && (stream = fopen(filename, "r")) == NULL) {
		ERROR(errno, "Unable to open file to stream");
		return 1;
	}
	checked_free(line);
	line = NULL;
	size_t len = 0;
	ssize_t count = 0;
	while ((count = next_line(&line, &len, stream)) > 0) {
		if (line != NULL && len > 0) {
			shell_core(line);
		}
	}
	if (mode == BATCH && fclose(stream)) {
		ERROR(errno, "Unable to close stream");
		return 1;
	}
	return 0;
}

int main(int argc, char** argv) {
	int ret;
	transparent_return(atexit(exit_handler));
	if (argc > 2) {
		ERROR(EINVAL, "usage: anubis [script]");
		return 1;
	}
	path_init();
	transparent_return(shell_stream(
		argc,
		argc == BATCH ? argv[1] : NULL
	));
}
