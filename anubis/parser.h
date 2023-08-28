#ifndef ANUBIS_PARSER_H
#define ANUBIS_PARSER_H

#include <stdbool.h>

#include "tokeniser.h"

#define DEFAULT_ARG_LIST_SIZE 3
#define DEFAULT_PIPES_LIST_SIZE 1
#define DEFAULT_IO_MODIFIER_LIST_SIZE 5
#define DEFAULT_COMMAND_LIST_SIZE 5

typedef char** ArgList;

typedef struct CommandAndArgs {
	char* command;
	ArgList args;
} CommandAndArgs;

typedef CommandAndArgs* PipeList;

typedef enum IoModifierType {
	GREAT // Maps to Token::GREAT
} IoModifierType;

typedef struct IoModifier {
	IoModifierType type;
	char* applicant;
} IoModifier;

typedef IoModifier* IoModifierList;

typedef bool BackgroundOp;

typedef struct CommandLine {
	PipeList pipes;
	IoModifierList ioModifiers;
	BackgroundOp bgOp;
} CommandLine;

typedef CommandLine* CommandList;

typedef struct Parser {
	size_t arg_list_base_size;
	size_t pipes_list_base_size;
	size_t io_modifier_list_base_size;
	size_t command_list_base_size;
} Parser;

Parser parser_default();

CommandList parse(Parser* parser, Tokeniser* tokeniser);

#endif // ANUBIS_PARSER_H
