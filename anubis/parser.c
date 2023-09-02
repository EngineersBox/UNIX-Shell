/* PARSER GRAMMAR:
 * =================================================
 * GOAL: CommandList;
 *
 * ArgList: <STRING>*;
 *
 * CommandAndArgs: <STRING> ArgList;
 *
 * PipeList:
 *		| <PIPE> CommandAndArgs PipeList
 *		| CommandAndArgs;
 *
 * IoModifier: <GREATER> <STRING>;
 *
 * IoModifierList:
 *		| IoModifier IoModifierList
 *		| IoModifier;
 *
 * BackgroundOp: <AMPERSAND>?;
 *
 * CommandLine: PipeList IoModifierList BackgroundOp;
 *
 * CommandList: CommandLine+;
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

#define VISIBILITY_PUBLIC
#define VISIBILITY_PRIVATE static

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
		ERROR(ENOMEM, "Unable t resize" #target " to size %d", size);\
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
	if (args == NULL) {
		ERROR(ENOMEM, "Unable to allocate ArgList of size %d", size);
		return NULL;
	}
	Token symbol;
	size_t index = 1;
	 while (lexer_next_symbol(lexer)) {	
		symbol = lexer_current_symbol(lexer);
		if (symbol != STRING) {
			break;
		} else if (index >= size - 1) {
			HANDLED_REALLOC(args, _this->arg_list_base_size);
		}
		if ((args[index++] = strdup(lexer_current_string(lexer))) == NULL) {
			ERROR(ENOMEM, "Unable to duplicate argument string");
			return NULL;
		}
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
	if (pipes == NULL) {
		ERROR(ENOMEM, "Unable to allocate PipeList of size %d", size);
		return NULL;
	}
	size_t index = 0;
	do {
		Command* commandAndArgs;
		int res = parse_command_and_args(_this, lexer, &commandAndArgs);
		if (res == -1) {
			return NULL;
		} else if (index >= size - 1) {
			HANDLED_REALLOC(pipes, _this->pipes_list_base_size);
		}
		pipes[index++] = commandAndArgs;
		if (res == 1) {
			break;
		}
	} while (true);//while (lexer_next_symbol(lexer));
	*count = index;
	return pipes;
}

VISIBILITY_PRIVATE IoModifiers* parse_io_modifiers(Parser* _this, Lexer* lexer) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	IoModifiers* ioModifiers = io_modifiers_new(NULL);
	if (ioModifiers == NULL) {
		return NULL;
	}
	Token symbol;
	size_t index;
	do {
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
			case GREATER:
				if (ioModifiers->in != NULL) {
					ERROR(EINVAL, "Multiple output redirection is not supported");
					return NULL;
				} else if ((ioModifiers->in = strdup(lexer_current_string(lexer))) == NULL) {
					ERROR(ENOMEM, "Unable to duplicate modifier target string");
					return NULL;
				}
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
	if (table == NULL) return NULL;
	size_t size = _this->command_list_base_size;
	table->lines = calloc(size, sizeof(*(table->lines)));
	if (table == NULL) {
		ERROR(ENOMEM, "Unable to allocate command list of size %d", size);
		return NULL;
	}
	size_t index = 0;
	while (lexer_next_symbol(lexer) && lexer_current_symbol(lexer) != EOI) {
		CommandLine* cmdLine = parse_command_line(_this, lexer);
		if (cmdLine == NULL) {
			return NULL;
		} else if (index >= size - 1) {
			HANDLED_REALLOC(table->lines, _this->command_list_base_size);
		}
		table->lines[index++] = cmdLine;
	}
	table->lineCount = index;
	return table;
}
