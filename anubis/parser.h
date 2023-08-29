#ifndef ANUBIS_PARSER_H
#define ANUBIS_PARSER_H

#include <stdbool.h>

#include "lexer.h"

#define DEFAULT_ARG_LIST_SIZE 3
#define DEFAULT_PIPES_LIST_SIZE 1
#define DEFAULT_IO_MODIFIER_LIST_SIZE 5
#define DEFAULT_COMMAND_LIST_SIZE 5

typedef char** Args;

typedef struct Command {
	size_t argCount;
	char* command;
	Args args;
} Command;

typedef Command* PipeList;

typedef enum IoModifierType {
	GREAT // Maps to Token::GREAT
} IoModifierType;

typedef struct IoModifier {
	IoModifierType type;
	char* target;
} IoModifier;

typedef IoModifier* IoModifierList;

typedef bool BackgroundOp;

typedef struct CommandLine {
	size_t pipeCount;
	size_t modifiersCount;
	PipeList pipes;
	IoModifierList ioModifiers;
	BackgroundOp bgOp;
} CommandLine;

typedef CommandLine* CommandTable;

typedef struct Parser {
	size_t arg_list_base_size;
	size_t pipes_list_base_size;
	size_t io_modifier_list_base_size;
	size_t command_list_base_size;
} Parser;

Parser parser_default();

/**
 * PARSER GRAMMAR:
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
CommandTable parse(Parser* parser, Lexer* lexer, size_t* count);

#endif // ANUBIS_PARSER_H
