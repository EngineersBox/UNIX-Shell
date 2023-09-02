#include "structure.h"

#include "checks.h"
#include "mem_utils.h"

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
	checked_array_free(command->args, command->argCount - 1, checked_free);
	checked_free(command->args);
	checked_free(command);
}

IoModifiers* io_modifiers_new(char* out) {
	IoModifiers* modifiers = malloc(sizeof(*modifiers));
	INSTANCE_NULL_CHECK_RETURN("IoModifiers", modifiers, NULL);
	modifiers->out = out;
	return modifiers;
}

void io_modifiers_free(IoModifiers* modifiers) {
	INSTANCE_NULL_CHECK("IoModifiers", modifiers);
	checked_free(modifiers->out);
	checked_free(modifiers);
}

CommandLine* command_line_new(PipeList pipes, size_t pipeCount, IoModifiers* ioModifiers, BackgroundOp bgOp) {
	CommandLine* cmdLine = malloc(sizeof(*cmdLine));
	INSTANCE_NULL_CHECK_RETURN("CommandLine", cmdLine, NULL);
	cmdLine->pipes = pipes;
	cmdLine->pipeCount = pipeCount;
	cmdLine->ioModifiers = ioModifiers;
	cmdLine->bgOp = bgOp;
	return cmdLine;
}

void command_line_free(CommandLine* line) {
	INSTANCE_NULL_CHECK("CommandLine", line);
	checked_array_free(line->pipes, line->pipeCount, command_free);
	checked_free(line->pipes);
	io_modifiers_free(line->ioModifiers);
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
	if (table == NULL) {
		return;
	}
	checked_array_free(table->lines, table->lineCount, command_line_free);
	checked_free(table->lines);
	checked_free(table);
}

void command_table_dump(CommandTable* table) {
	INSTANCE_NULL_CHECK("CommandTable", table);
	for (int i = 0; i < table->lineCount; i++) {
		CommandLine* line = table->lines[i];
		for (int j = 0; j < line->pipeCount; j++) {
			Command* cmdArgs = line->pipes[j];
			fprintf(stderr, "[%d] %s [", i, cmdArgs->command);
			for (int k = 0; k < cmdArgs->argCount; k++) {
				fprintf(stderr, "%s%s", cmdArgs->args[k], k == cmdArgs->argCount - 1 ? "" : ",");
			}
			fprintf(stderr, "]\n");
		}
		if (line->ioModifiers != NULL) {
			if (line->ioModifiers->out != NULL) {
				fprintf(stderr, "   [>] %s\n", line->ioModifiers->out);
			}
		}
		fprintf(stderr, "   [&] %s\n", line->bgOp ? "true" : "false");
	}
}
