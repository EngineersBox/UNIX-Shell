#ifndef ANUBIS_STRUCTURE_H
#define ANUBIS_STRUCTURE_H

#include <stdbool.h>
#include <stddef.h>

typedef char** Args;

typedef struct __attribute__((__packed__)) Command {
	size_t argCount;
	char* command;
	Args args;
} Command;

Command* command_new(char* command, Args args, size_t argCount);
void command_free(Command* command);

typedef Command** PipeList;

typedef enum IoModifierType {
	GREAT // Maps to Token::GREAT
} IoModifierType;

typedef struct IoModifier {
	IoModifierType type;
	char* target;
} IoModifier;

IoModifier* io_modifier_new(IoModifierType type, char* target);
void io_modifier_free(IoModifier* modifier);

typedef IoModifier** IoModifierList;

typedef bool BackgroundOp;

typedef struct __attribute__((__packed__)) CommandLine {
	size_t pipeCount;
	size_t modifiersCount;
	PipeList pipes;
	IoModifierList ioModifiers;
	BackgroundOp bgOp;
} CommandLine;

CommandLine* command_line_new(PipeList pipes, size_t pipeCount, IoModifierList ioModifiers, size_t modifiersCount, BackgroundOp bgOp);
void command_line_free(CommandLine* line);

typedef struct CommandTable {
	size_t lineCount;
	CommandLine** lines;
} CommandTable;

CommandTable* command_table_new();
void command_table_free(CommandTable* table);

void command_table_dump(CommandTable* table);

#endif // ANUBIS_STRUCTURE_H
