#include "structure.h"

#include "checks.h"
#include "memutils.h"

Command* command_new(char* command, Args args, size_t argCount) {
	Command* cmd = malloc(sizeof(*cmd));
	INSTANCE_NULL_CHECK_RETURN("command", cmd, NULL);
	cmd->command = command;
	cmd->args = args;
	cmd->argCount = argCount;
	return cmd;
}

void command_free(Command* command) {
	INSTANCE_NULL_CHECK("command", command);
	checked_free(command->command);
	checked_array_free(command->args, command->argCount, checked_free);
	checked_free(command->args);
	checked_free(command);
}

IoModifier* io_modifier_new(IoModifierType type, char* target) {
	IoModifier* modifier = malloc(sizeof(*modifier));
	INSTANCE_NULL_CHECK_RETURN("IoModifier", modifier, NULL);
	modifier->type = type;
	modifier->target = target;
	return modifier;
}

void io_modifier_free(IoModifier* modifier) {
	INSTANCE_NULL_CHECK("IoModifier", modifier);
	checked_free(modifier->target);
	checked_free(modifier);
}

CommandLine* command_line_new(PipeList pipes, size_t pipeCount, IoModifierList ioModifiers, size_t modifiersCount, BackgroundOp bgOp) {
	CommandLine* cmdLine = malloc(sizeof(*cmdLine));
	INSTANCE_NULL_CHECK_RETURN("CommandLine", cmdLine, NULL);
	cmdLine->pipes = pipes;
	cmdLine->pipeCount = pipeCount;
	cmdLine->ioModifiers = ioModifiers;
	cmdLine->modifiersCount = modifiersCount;
	cmdLine->bgOp = bgOp;
	return cmdLine;
}

void command_line_free(CommandLine* line) {
	INSTANCE_NULL_CHECK("CommandLine", line);
	checked_array_free(line->pipes, line->pipeCount, command_free);
	checked_free(line->pipes);
	checked_array_free(line->ioModifiers, line->modifiersCount, io_modifier_free);
	checked_free(line->ioModifiers);
	checked_free(line);
}

CommandTable* command_table_new() {
	CommandTable* table = malloc(sizeof(*table));
	INSTANCE_NULL_CHECK_RETURN("CommandTable", table, NULL);
	table->lineCount = 0;
	table->lines = NULL;
	return table;
}

void command_table_free(CommandTable* table) {
	INSTANCE_NULL_CHECK("CommandTable", table);
	checked_array_free(table->lines, table->lineCount, command_line_free);
	checked_free(table->lines);
	checked_free(table);
}

