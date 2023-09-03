/* PARSER TYPE: LR(1) [Left to right, Rightmost derivation, Single token lookahead]
 * PARSER GRAMMAR:
 * =================================================
 * GOAL: CommandList;
 *
 * Args: <STRING>*;
 *
 * Command: <STRING> Args;
 *
 * PipeList:
 *		| <PIPE> Command PipeList
 *		| Command;
 *
 * IoModifier:
 *		| <GREATER> <STRING>;
 *
 * IoModifiers: IoModifier?;
 *
 * BackgroundOp: <AMPERSAND>?;
 *
 * CommandLine: PipeList IoModifiers BackgroundOp;
 *
 * CommandList: CommandLine*;
 * =================================================
 */

#include "parser.h"

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include "error.h"
#include "checks.h"
#include "lexer.h"
#include "structure.h"
#include "visibility.h"

Parser parser_default() {
	return (Parser) {
		.arg_list_base_size = DEFAULT_ARG_LIST_SIZE,
		.pipes_list_base_size = DEFAULT_PIPES_LIST_SIZE,
		.command_list_base_size = DEFAULT_COMMAND_LIST_SIZE,
	};
}

#define HANDLED_REALLOC(target, bump)\
	size += (bump);\
	if (((target) = realloc((target), size * sizeof(*target))) == NULL) {\
		ERROR(ENOMEM, "Unable to resize " #target " to size %d", size);\
		return NULL;\
	}

VISIBILITY_PRIVATE Args parse_args(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);	
	if (lexer_current_symbol(lexer) != STRING) {
		*count = 0;
		ERROR(EINVAL, "Expected a string, got %s", token_names[lexer_current_symbol(lexer)]);
		return NULL;
	}
	size_t size = _this->arg_list_base_size;
	Args args = calloc(size, sizeof(*args));
	verrno_return(args, NULL, "Unable to allocate Args of size %d", size);
	Token symbol;
	size_t index = 1;
	 while (lexer_next_symbol(lexer)) {	
		symbol = lexer_current_symbol(lexer);
		if (symbol != STRING) {
			break;
		} else if (index >= size - 1) {
			// Resize the Args if we have more than the space allocated currently allows for
			HANDLED_REALLOC(args, _this->arg_list_base_size);
		}
		verrno_return(
			args[index++] = strdup(lexer_current_string(lexer)),
			NULL, "Unable to duplicate argument string"
		);
	}
	if (index >= size - 1) {
		HANDLED_REALLOC(args, _this->arg_list_base_size);
	}
	args[index++] = NULL;
	*count = index;
	return args;
}

// -1: Failure, 0: Continue, 1: Terminate
VISIBILITY_PRIVATE int parse_command_and_args(Parser* _this, Lexer* lexer, Command** commandAndArgs) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, -1);
	Token prefix = lexer_current_symbol(lexer);
	char* command;
	if (prefix != STRING && prefix != PIPE) {
		ERROR(EINVAL, "Expected a pipe or subcommand, got %s", token_names[prefix]);
		return -1;
	} else if (prefix == PIPE && !lexer_next_symbol(lexer)) {
		ERROR(EINVAL, "Unable to parse command following pipe");
		return -1;
	} else if ((command = strdup(lexer_current_string(lexer))) == NULL) {
		ERROR(ENOMEM, "unable to duplicate command string");
		return -1;
	}
	size_t argCount;
	Args args = parse_args(_this, lexer, &argCount);
	args[0] = strdup(command);
	*commandAndArgs = command_new(
		command,
		args,
		argCount
	);
	return lexer_current_symbol(lexer) != PIPE;
} 

VISIBILITY_PRIVATE PipeList parse_pipe_list(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	size_t size = _this->pipes_list_base_size;
	PipeList pipes = calloc(size, sizeof(*pipes));
	verrno_return(pipes, NULL, "Unable to allocate PipeList of size %d", size);
	size_t index = 0;
	do {
		Command* commandAndArgs;
		int res = parse_command_and_args(_this, lexer, &commandAndArgs);
		if (res == -1) {
			return NULL;
		} else if (index >= size - 1) {
			// Resize the PipeList if we have more than the space allocated currently allows for
			HANDLED_REALLOC(pipes, _this->pipes_list_base_size);
		}
		pipes[index++] = commandAndArgs;
		if (res == 1) {
			break;
		}
	} while (true);
	*count = index;
	return pipes;
}

// Generified modifier handling
#define CASE_MODIFIER(_symbol, field, name)\
	case _symbol:\
		if ((field) != NULL) {\
			ERROR(EINVAL, "Multiple " name " redirection is not supported");\
			return NULL;\
		} else if (((field) = strdup(lexer_current_string(lexer))) == NULL) {\
			ERROR(ENOMEM, "Unable to duplicate modifier target string");\
			return NULL;\
		}\
		break

VISIBILITY_PRIVATE IoModifiers* parse_io_modifiers(Parser* _this, Lexer* lexer) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	IoModifiers* ioModifiers = io_modifiers_new(NULL);
	verrno_return(ioModifiers, NULL, "Unable to allocate IoModifiers");
	Token symbol;
	size_t index = 0;
	do { // NOTE: Loop is not neccessary for this implementation, but makes it easier for implementing other IO modifiers in future
		symbol = lexer_current_symbol(lexer);
		if (index > 0 && (symbol != EOI && symbol == STRING)) {
			ERROR(EINVAL, "Unexpected string when parsing IoModifiers");
			return NULL;
		} else if (!is_modifier(symbol)) {
			break;
		} else if (!lexer_next_symbol(lexer)) {
			ERROR(EINVAL, "Unable to parse IO modifier target");
			return NULL;
		} else if (lexer_current_symbol(lexer) != STRING) {
			ERROR(EINVAL, "Expected target of IO modifier (string), got %s", token_names[symbol]);
			return NULL;
		}
		switch (symbol) {
			// NOTE: With support for more modifiers, we would add more cases here
			CASE_MODIFIER(GREATER, ioModifiers->outTrunc, "truncated output");
			default: break;
		}
		index++;
	} while (lexer_next_symbol(lexer));
	return ioModifiers;
}

VISIBILITY_PRIVATE BackgroundOp parse_background_op(Parser* _this, Lexer* lexer) {
	Token token = lexer_current_symbol(lexer);
	return token == AMPERSAND;
}

VISIBILITY_PRIVATE CommandLine* parse_command_line(Parser* _this, Lexer* lexer) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, 0);
	size_t pipeCount = 0;
	PipeList pipes = parse_pipe_list(_this, lexer, &pipeCount);
	if (pipes == NULL) {
		return NULL;
	}
	IoModifiers* ioModifiers = parse_io_modifiers(_this, lexer);
	if (ioModifiers == NULL) {
		return NULL;
	}
	BackgroundOp bgOp = parse_background_op(_this, lexer);
	return command_line_new(
		pipes,
		pipeCount,
		ioModifiers,
		bgOp
	);
}

VISIBILITY_PUBLIC CommandTable* parse(Parser* _this, Lexer* lexer) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	CommandTable* table = command_table_new();
	verrno_return(table, NULL, "Unable to allocate CommandTable");
	size_t size = _this->command_list_base_size;
	table->lines = calloc(size, sizeof(*(table->lines)));
	verrno_return(table, NULL, "Unable to allocate command list of size %d", size);
	size_t index = 0;
	Token symbol;
	while (lexer_next_symbol(lexer) && (symbol = lexer_current_symbol(lexer)) != EOI) {
		if (symbol == AMPERSAND) {
			continue;
		}
		CommandLine* cmdLine = parse_command_line(_this, lexer);
		if (cmdLine == NULL) {
			return NULL;
		} else if (index >= size - 1) {
			// Resize the table lines if we have more than the space allocated currently allows for
			HANDLED_REALLOC(table->lines, _this->command_list_base_size);
		}
		table->lines[index++] = cmdLine;
	}
	table->lineCount = index;
	return table;
}
