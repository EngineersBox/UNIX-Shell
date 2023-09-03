#ifndef ANUBIS_PARSER_H
#define ANUBIS_PARSER_H

#include <stdbool.h>

#include "lexer.h"
#include "structure.h"

#define DEFAULT_ARG_LIST_SIZE 3
#define DEFAULT_PIPES_LIST_SIZE 1
#define DEFAULT_COMMAND_LIST_SIZE 5

typedef struct Parser {
	size_t arg_list_base_size;
	size_t pipes_list_base_size;
	size_t command_list_base_size;
} Parser;

Parser parser_default();

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
CommandTable* parse(Parser* parser, Lexer* lexer);

#endif // ANUBIS_PARSER_H
