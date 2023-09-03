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


typedef struct IoModifiers {
	char* outTrunc; // NOTE: Add support for other IO operators here (e.g. >>, <, etc)
} IoModifiers;

IoModifiers* io_modifiers_new(char* outTrunc);
void io_modifiers_free(IoModifiers* modifiers);

typedef bool BackgroundOp;

typedef struct __attribute__((__packed__)) CommandLine {
	size_t pipeCount;
	PipeList pipes;
	IoModifiers* ioModifiers;
	BackgroundOp bgOp;
} CommandLine;

CommandLine* command_line_new(PipeList pipes, size_t pipeCount, IoModifiers* ioModifiers, BackgroundOp bgOp);
void command_line_free(CommandLine* line);

typedef struct CommandTable {
	size_t lineCount;
	CommandLine** lines;
} CommandTable;

CommandTable* command_table_new();
void command_table_free(CommandTable* table);

void command_table_dump(CommandTable* table);

#endif // ANUBIS_STRUCTURE_H
