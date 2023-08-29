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
		.io_modifier_list_base_size = DEFAULT_IO_MODIFIER_LIST_SIZE,
		.command_list_base_size = DEFAULT_COMMAND_LIST_SIZE,
	};
}

#define HANDLED_REALLOC(target, bump)\
	size += (bump);\
	if (((target) = realloc((target), size * sizeof(*target))) == NULL) {\
		ERROR(ENOMEM, "Unable tp resize" #target " to size %d\n", size);\
		return NULL;\
	}

VISIBILITY_PRIVATE Args parse_args(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);	
	if (lexer_current_symbol(lexer) != STRING) {
		*count = 0;
		return NULL;
	}
	size_t size = _this->arg_list_base_size;
	Args args = calloc(size, sizeof(*args));
	if (args == NULL) {
		ERROR(ENOMEM, "Unable to allocate ArgList of size %d\n", size);
		return NULL;
	}
	Token symbol;
	size_t index = 0;
	 while (lexer_next_symbol(lexer)) {	
		symbol = lexer_current_symbol(lexer);
		if (symbol != STRING) {
			lexer_unget_symbol(lexer);
			break;
		} else if (index >= size - 1) {
			HANDLED_REALLOC(args, _this->arg_list_base_size);
		}
		if ((args[index++] = strdup(lexer_current_string(lexer))) == NULL) {
			ERROR(ENOMEM, "Unable to duplicate argument string\n");
			return NULL;
		}
	}
	*count = index;
	return args;
}

// -1: Failure, 0: Continue, 1: Terminate
VISIBILITY_PRIVATE int parse_command_and_args(Parser* _this, Lexer* lexer, Command** commandAndArgs) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, -1);
	Token prefix = lexer_current_symbol(lexer);
	char* command;
	if (prefix != STRING && prefix != PIPE) {
		ERROR(EINVAL, "Expected a pipe or subcommand, got %s\n", token_names[prefix]);
		return -1;
	} else if (prefix == PIPE && !lexer_next_symbol(lexer)) {
		ERROR(EINVAL, "Unable to parse command following pipe\n");
		return -1;
	} else if ((command = strdup(lexer_current_string(lexer))) == NULL) {
		ERROR(ENOMEM, "unable to duplicate command string\n");
		return -1;
	}
	size_t argCount;
	Args args = parse_args(_this, lexer, &argCount);
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
		ERROR(ENOMEM, "Unable to allocate PipeList of size %d\n", size);
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
	} while (lexer_next_symbol(lexer));
	*count = index;
	return pipes;
}

VISIBILITY_PRIVATE int map_token_to_io_modifier_type(Token token) {
	switch (token) {
		case GREATER: return (IoModifierType) GREATER;
		default: return -1;
	}
}

VISIBILITY_PRIVATE IoModifierList parse_io_modifier_list(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	size_t size = _this->io_modifier_list_base_size;
	IoModifierList ioModifierList = calloc(size, sizeof(*ioModifierList));
	if (ioModifierList == NULL) {
		ERROR(ENOMEM, "Unable to allocate IoModifierList of size %d\n", size);
		return NULL;
	}
	Token symbol;
	size_t index = 0;
	do {
		symbol = lexer_current_symbol(lexer);
		int modifier = map_token_to_io_modifier_type(symbol);
		if (modifier == -1) {
			break;
		}
		IoModifierType type = (IoModifierType) modifier;
		if (!lexer_next_symbol(lexer)) {
			ERROR(EINVAL, "Unable to parse IO modifier target\n");
			return NULL;
		}
		symbol = lexer_current_symbol(lexer);
		if (symbol != STRING) {
			ERROR(EINVAL, "Expected target of IO modifier (string), got %s", token_names[symbol]);
			return NULL;
		} else if (index >= size - 1) {
			HANDLED_REALLOC(ioModifierList, _this->io_modifier_list_base_size);
		}
		ioModifierList[index++] = io_modifier_new(type, strdup(lexer_current_string(lexer)));
		if (ioModifierList[index - 1]->target == NULL) {
			ERROR(ENOMEM, "Unable to duplicate modifier target string\n");
			return NULL;
		}
		printf("Modifier: %s\n", ioModifierList[index - 1]->target);
	} while (lexer_next_symbol(lexer));
	*count = index;
	return ioModifierList;
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
	size_t modifiersCount = 0;
	IoModifierList ioModifiers = parse_io_modifier_list(_this, lexer, &modifiersCount);
	if (ioModifiers == NULL) {
		return NULL;
	}
	BackgroundOp bgOp = parse_background_op(_this, lexer);
	return command_line_new(
		pipes,
		pipeCount,
		ioModifiers,
		modifiersCount,
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
		ERROR(ENOMEM, "Unable to allocate command list of size %d\n", size);
		return NULL;
	}
	size_t index = 0;
	while (lexer_next_symbol(lexer) && lexer_current_symbol(lexer) != EOI) {
		CommandLine* cmdLine = parse_command_line(_this, lexer);
		if (cmdLine == NULL) {
			return NULL;
		} else if (index >= size - 1) {
			HANDLED_REALLOC(table, _this->command_list_base_size);
		}
		table->lines[index++] = cmdLine;
	}
	table->lineCount = index;
	return table;
}
