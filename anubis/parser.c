#include "parser.h"

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include "error.h"
#include "tokeniser.h"

Parser parser_default() {
	return (Parser) {
		.arg_list_base_size = DEFAULT_ARG_LIST_SIZE,
		.pipes_list_base_size = DEFAULT_PIPES_LIST_SIZE,
		.io_modifier_list_base_size = DEFAULT_IO_MODIFIER_LIST_SIZE,
		.command_list_base_size = DEFAULT_COMMAND_LIST_SIZE,
	};
}

ArgList parse_arg_list(Parser* parser, Tokeniser* tokeniser) {
	size_t size = parser->arg_list_base_size;
	ArgList argList = calloc(size, sizeof(*argList));
	if (argList == NULL) {
		ERROR(ENOMEM, "Unable to allocate ArgList of size %d\n", size);
		return NULL;
	}
	Token symbol;
	size_t index = 0;
	while (tokeniser_next_symbol(tokeniser)) {
		symbol = tokeniser_current_symbol(tokeniser);
		if (symbol != STRING) {
			tokeniser_unget_symbol(tokeniser);
			break;
		}
		if (index >= size - 1) {
			size += parser->arg_list_base_size;
			argList = realloc(argList, size);
			if (argList == NULL) {
				ERROR(ENOMEM, "Unable top resize ArgList to size %d\n", size);
				return NULL;
			}
		}
		argList[index++] = strdup(tokeniser_current_string(tokeniser));
	}
	return argList;
}

// -1: Failure, 0: Continue, 1: Terminate
int parse_command_and_args(Parser* parser, Tokeniser* tokeniser, CommandAndArgs* commandAndArgs, Token prefix) {
	if (prefix != STRING && prefix != PIPE) {
		ERROR(EINVAL, "Expected a pipe or subcommand, got %s\n", token_names[prefix]);
		return -1;
	} else if (prefix == PIPE) {
		tokeniser_next_symbol(tokeniser);
	}
	commandAndArgs->command = tokeniser_current_string(tokeniser);
	commandAndArgs->args = parse_arg_list(parser, tokeniser);
	return prefix == PIPE;
} 

PipeList parse_pipe_list(Parser* parser, Tokeniser* tokeniser) {
	size_t size = parser->pipes_list_base_size;
	PipeList pipes = calloc(size, sizeof(*pipes));
	if (pipes == NULL) {
		ERROR(ENOMEM, "Unable to allocate PipeList of size %d\n", size);
		return NULL;
	}
	Token symbol;
	size_t index = 0;
	while (tokeniser_next_symbol(tokeniser)) {
		symbol = tokeniser_current_symbol(tokeniser);
		CommandAndArgs commandAndArgs = {};
		int res = parse_command_and_args(parser, tokeniser, &commandAndArgs, symbol);
		if (res == -1) {
			return NULL;
		}
		if (index >= size - 1) {
			size += parser->pipes_list_base_size;
			pipes = realloc(pipes, size);
			if (pipes == NULL) {
				ERROR(ENOMEM, "Unable to resize PipeList to size %d\n", size);
				return NULL;
			}
		}
		pipes[index++] = commandAndArgs;
		if (res == 1) {
			break;
		}
	}
	return pipes;
}

int map_token_to_io_modifier_type(Token token) {
	switch (token) {
		case GREAT: return (IoModifierType) GREAT;
		default: return -1;
	}
}

IoModifierList parse_io_modifier_list(Parser* parser, Tokeniser* tokeniser) {
	size_t size = parser->io_modifier_list_base_size;
	IoModifierList ioModifierList = calloc(size, sizeof(*ioModifierList));
	if (ioModifierList == NULL) {
		ERROR(ENOMEM, "Unable to allocate IoModifierList of size %d\n", size);
		return NULL;
	}
	Token symbol;
	size_t index = 0;
	while (tokeniser_next_symbol(tokeniser)) {
		symbol = tokeniser_current_symbol(tokeniser);
		int modifier = map_token_to_io_modifier_type(symbol);
		if (modifier == -1) {
			ERROR(EINVAL, "Expected an IO modifier, got %s", token_names[symbol]);
		}
		IoModifierType type = (IoModifierType) modifier;
		symbol = tokeniser_next_symbol(tokeniser);
		if (symbol != STRING) {
			ERROR(EINVAL, "Expected application of IO modifier (string), got %s", token_names[symbol]);
		}
		if (index >= size - 1) {
			size += parser->io_modifier_list_base_size;
			ioModifierList = realloc(ioModifierList, size);
			if (ioModifierList == NULL) {
				ERROR(ENOMEM, "Unable to resize IoModifierList to size %d\n", size);
				return NULL;
			}
		}
		ioModifierList[index++] = (IoModifier) {
			.type = type,
			.applicant = strdup(tokeniser_current_string(tokeniser))
		};
	}
	return ioModifierList;
}

BackgroundOp parse_background_op(Parser* parser, Tokeniser* tokeniser) {
	tokeniser_next_symbol(tokeniser);
	Token token = tokeniser_current_symbol(tokeniser);
	return token == AMPERSAND;
}

int parse_command_line(Parser* parser, Tokeniser* tokeniser, CommandLine* cmdLine) {
	PipeList pipes = parse_pipe_list(parser, tokeniser);
	if (pipes == NULL) {
		return 1;
	}
	IoModifierList ioModifiers = parse_io_modifier_list(parser, tokeniser);
	if (ioModifiers == NULL) {
		return 1;
	}
	BackgroundOp bgOp = parse_background_op(parser, tokeniser);
	cmdLine->pipes = pipes;
	cmdLine->ioModifiers = ioModifiers;
	cmdLine->bgOp = bgOp;
	return 0;
}

CommandList parse(Parser* parser, Tokeniser* tokeniser) {
	size_t size = parser->command_list_base_size;
	CommandList cmdList = calloc(size, sizeof(*cmdList));
	if (cmdList == NULL) {
		ERROR(ENOMEM, "Unable to allocate command list of size %d\n", size);
		return NULL;
	}
	size_t index = 0;
	int res;
	while (tokeniser_current_symbol(tokeniser) != EOI) {
		CommandLine cmdLine = {};
		res = parse_command_line(parser, tokeniser, &cmdLine);
		if (!res) {
			return NULL;
		}
		if (index >= size - 1) {
			size += parser->command_list_base_size;
			cmdList = realloc(cmdList, size);
			if (cmdList == NULL) {
				ERROR(ENOMEM, "Unable to resize CommandList to size %d\n", size);
				return NULL;
			}
		}
		cmdList[index++] = cmdLine;
	}
	return cmdList;
}
