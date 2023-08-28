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
#include "tokeniser.h"

// TODO: implement your shell!

int main(int argc, char** argv) {
	// 1. Check if args exist for batch exec
	// 2. If they do then pipe batch file lines as input
	// 3. Otherwise poll stdin for lines as input
	// 4. Tokenise line
	// 5. Parse line
	// 6. Construct execution structures
	// 7. Execute commands and resolve paths on-demand
	return 0;
}

int tokeniser_test_main(int argc, char** argv) {
	char* string = "cmd1 | ./cmd2 > output1 & /test/other\\ cmd3 > ouput2 & cmd4";
	printf("Tokenising: %s\n", string);
	Tokeniser tok = tokeniser_new(string);
	while (tokeniser_next_symbol(&tok)) {
		Token symbol = tokeniser_current_symbol(&tok);
		switch (symbol) {
			case AMPERSAND:
			case PIPE:
			case GREATER:
			case EOI:
				printf("[SYMBOL: %s]\n", token_names[symbol]);
				break;
			case STRING:
				printf("[SYMBOL: %s] %s\n", token_names[symbol], tokeniser_current_string(&tok));
				break;
		}
	}
    return 0;
}

