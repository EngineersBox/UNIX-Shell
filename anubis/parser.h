#ifndef ANUBIS_PARSER_H
#define ANUBIS_PARSER_H

#include <stdbool.h>

#include "tokeniser.h"

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

} Parser;

CommandList parse(Parser* parser, Tokeniser* tokeniser);

#endif // ANUBIS_PARSER_H
