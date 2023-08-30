#include "structure.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h> 
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h>

#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"

int executor_test_main(int argc, char** argv) {
	char* string = "/bin/echo \"yeah nah\"|rev | rev|/bin/wc -c> test.txt & /bin/echo \"wait now\" > test2.txt & /bin/ls -la";
	fprintf(stderr, "Executing: %s\n", string);
	Lexer lexer = lexer_new(string);
	Parser parser = parser_default();
	CommandTable* table = parse(&parser, &lexer);
	command_table_dump(table);
	execute(table);
	command_table_free(table);
	lexer_free(&lexer);
	return 0;
}

int parser_test_main(int argc, char** argv) {
	char* string = "cmd1 | ./cmd2 > output1 & /test/other\\ cmd3 > ouput2 & cmd4";
	printf("Parsing: %s\n", string);
	Lexer lexer = lexer_new(string);
	Parser parser = parser_default();
	CommandTable* table = parse(&parser, &lexer);
	command_table_dump(table);
	command_table_free(table);
	lexer_free(&lexer);
	return 0;
}

int lexer_test_main(int argc, char** argv) {
	char* string = "cmd1 | ./cmd2 > output1 & /test/other\\ cmd3 > ouput2 & cmd4";
	printf("Tokenising: %s\n", string);
	Lexer tok = lexer_new(string);
	while (lexer_next_symbol(&tok)) {
		Token symbol = lexer_current_symbol(&tok);
		switch (symbol) {
			case AMPERSAND:
			case PIPE:
			case GREATER:
			case EOI:
				printf("[SYMBOL: %s]\n", token_names[symbol]);
				break;
			case STRING:
				printf("[SYMBOL: %s] %s\n", token_names[symbol], lexer_current_string(&tok));
				break;
		}
	}
    return 0;
}

// TODO: implement your shell!

int main(int argc, char** argv) {
	// 1. Check if args exist for batch exec
	// 2. If they do then pipe batch file lines as input
	// 3. Otherwise poll stdin for lines as input
	// 4. Tokenise line
	// 5. Parse line
	// 6. Construct execution structures
	// 7. Execute commands and resolve paths on-demand
	executor_test_main(0, NULL);
	return 0;
}
