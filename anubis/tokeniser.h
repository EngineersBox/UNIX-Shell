#ifndef ANUBIS_TOKENISER_H
#define ANUBIS_TOKENISER_H

#include <stdbool.h>
#include <stddef.h>

#define _TOK_AMPERSAND '&'
#define _TOK_PIPE '|'
#define _TOK_GREATER '>'

#define _IS_RESERVED(c) (c) == _TOK_AMPERSAND || (c) == _TOK_PIPE || (c) == _TOK_GREATER
#define _IS_WHITESPACE(c) (c) == ' ' || (c) == '\t' || (c) == '\n'

typedef enum Token {
	AMPERSAND,
	PIPE,
	GREATER,
	STRING,
	EOI
} Token;

static const char* token_names[] = {
	[AMPERSAND] = "AMPERSAND",
	[PIPE] = "PIPE",
	[GREATER] = "GREATER",
	[STRING] = "STRING",
	[EOI] = "EOI"

};
typedef struct Tokeniser {
	size_t pos;
	int cchar;
	int symbol;
	size_t source_len;
	char* source;
	bool is_reading_string;
	size_t string_len;
	size_t string_pos;
	char* string;
} Tokeniser;

Tokeniser tokeniser_new(char* source);

int tokeniser_reset(Tokeniser* _this, char* source);
int tokeniser_next_symbol(Tokeniser* _this);

int tokeniser_current_symbol(Tokeniser* _this);
char* tokeniser_current_string(Tokeniser* _this);

#endif // ANUBIS_TOKENISER_H
