#include "parser.h"

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>

#include "error.h"
#include "checks.h"
#include "lexer.h"

Parser parser_default() {
	return (Parser) {
		.arg_list_base_size = DEFAULT_ARG_LIST_SIZE,
		.pipes_list_base_size = DEFAULT_PIPES_LIST_SIZE,
		.io_modifier_list_base_size = DEFAULT_IO_MODIFIER_LIST_SIZE,
		.command_list_base_size = DEFAULT_COMMAND_LIST_SIZE,
	};
}

ArgList parse_arg_list(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);	
	if (lexer_current_symbol(lexer) != STRING) {
		*count = 0;
		return NULL;
	}
	size_t size = _this->arg_list_base_size * sizeof(char*);
	ArgList argList = malloc(size);
	if (argList == NULL) {
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
			size += _this->arg_list_base_size * sizeof(char*);
			if ((argList = realloc(argList, size)) == NULL) {
				ERROR(ENOMEM, "Unable to resize ArgList to size %d\n", size);
				return NULL;
			}
		}
		if ((argList[index++] = strdup(lexer_current_string(lexer))) == NULL) {
			ERROR(ENOMEM, "Unable to duplicate argument string\n");
			return NULL;
		}
	}
	*count = index;
	return argList;
}

// -1: Failure, 0: Continue, 1: Terminate
int parse_command_and_args(Parser* _this, Lexer* lexer, CommandAndArgs* commandAndArgs) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, -1);
	Token prefix = lexer_current_symbol(lexer);
	if (prefix != STRING && prefix != PIPE) {
		ERROR(EINVAL, "Expected a pipe or subcommand, got %s\n", token_names[prefix]);
		return -1;
	} else if (prefix == PIPE && !lexer_next_symbol(lexer)) {
		ERROR(EINVAL, "Unable to parse command following pipe\n");
		return -1;
	} else if ((commandAndArgs->command = strdup(lexer_current_string(lexer))) == NULL) {
		ERROR(ENOMEM, "unable to duplicate command string\n");
		return -1;
	}
	commandAndArgs->args = parse_arg_list(_this, lexer, &commandAndArgs->argCount);
	printf("CMD: %s :: %s\n", commandAndArgs->command, token_names[lexer_current_symbol(lexer)]);
	return lexer_current_symbol(lexer) != PIPE;
} 

PipeList parse_pipe_list(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	size_t size = _this->pipes_list_base_size * sizeof(CommandAndArgs);
	PipeList pipes = malloc(size);
	if (pipes == NULL) {
		ERROR(ENOMEM, "Unable to allocate PipeList of size %d\n", size);
		return NULL;
	}
	size_t index = 0;
	do {
		CommandAndArgs commandAndArgs = {};
		int res = parse_command_and_args(_this, lexer, &commandAndArgs);
		if (res == -1) {
			return NULL;
		} else if (index >= size - 1) {
			size += _this->pipes_list_base_size * sizeof(CommandAndArgs);
			if ((pipes = realloc(pipes, size)) == NULL) {
				ERROR(ENOMEM, "Unable to resize PipeList to size %d\n", size);
				return NULL;
			}
		}
		pipes[index++] = commandAndArgs;
		if (res == 1) {
			break;
		}
	} while (lexer_next_symbol(lexer));
	*count = index;
	return pipes;
}

int map_token_to_io_modifier_type(Token token) {
	switch (token) {
		case GREATER: return (IoModifierType) GREATER;
		default: return -1;
	}
}

IoModifierList parse_io_modifier_list(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	size_t size = _this->io_modifier_list_base_size * sizeof(IoModifier);
	IoModifierList ioModifierList = malloc(size);
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
			size += _this->io_modifier_list_base_size * sizeof(IoModifier);
			if ((ioModifierList = realloc(ioModifierList, size)) == NULL) {
				ERROR(ENOMEM, "Unable to resize IoModifierList to size %d\n", size);
				return NULL;
			}
		}
		ioModifierList[index++] = (IoModifier) {
			.type = type,
			.applicant = strdup(lexer_current_string(lexer))
		};
		if (ioModifierList[index - 1].applicant == NULL) {
			ERROR(ENOMEM, "Unable to duplicate modifier target applicant string\n");
			return NULL;
		}
		printf("Modifier: %s\n", ioModifierList[index - 1].applicant);
	} while (lexer_next_symbol(lexer));
	*count = index;
	return ioModifierList;
}

BackgroundOp parse_background_op(Parser* _this, Lexer* lexer) {
	Token token = lexer_current_symbol(lexer);
	return token == AMPERSAND;
}

int parse_command_line(Parser* _this, Lexer* lexer, CommandLine* cmdLine) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, 0)
	PipeList pipes = parse_pipe_list(_this, lexer, &cmdLine->pipeCount);
	if (pipes == NULL) {
		return 0;
	}
	IoModifierList ioModifiers = parse_io_modifier_list(_this, lexer, &cmdLine->modifiersCount);
	if (ioModifiers == NULL) {
		return 0;
	}
	BackgroundOp bgOp = parse_background_op(_this, lexer);
	cmdLine->pipes = pipes;
	cmdLine->ioModifiers = ioModifiers;
	cmdLine->bgOp = bgOp;
	return 1;
}

CommandList parse(Parser* _this, Lexer* lexer, size_t* count) {
	INSTANCE_NULL_CHECK_RETURN("parser", _this, NULL);
	size_t size = _this->command_list_base_size * sizeof(CommandLine);
	CommandList cmdList = malloc(size);
	if (cmdList == NULL) {
		ERROR(ENOMEM, "Unable to allocate command list of size %d\n", size);
		return NULL;
	}
	size_t index = 0;
	int res;
	while (lexer_next_symbol(lexer) && lexer_current_symbol(lexer) != EOI) {
		CommandLine cmdLine = {};
		res = parse_command_line(_this, lexer, &cmdLine);
		if (!res) {
			return NULL;
		} else if (index >= size - 1) {
			size += _this->command_list_base_size * sizeof(CommandLine);
			if ((cmdList = realloc(cmdList, size))== NULL) {
				ERROR(ENOMEM, "Unable to resize CommandList to size %d\n", size);
				return NULL;
			}
		}
		cmdList[index++] = cmdLine;
	}
	*count = index;
	return cmdList;
}
