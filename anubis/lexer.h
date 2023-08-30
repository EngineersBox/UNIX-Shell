#ifndef ANUBIS_TOKENISER_H
#define ANUBIS_TOKENISER_H

#include <stdbool.h>
#include <stddef.h>

#define _TOK_AMPERSAND '&'
#define _TOK_PIPE '|'
#define _TOK_GREATER '>'

#define _IS_RESERVED(c) (c) == _TOK_AMPERSAND || (c) == _TOK_PIPE || (c) == _TOK_GREATER
#define _IS_WHITESPACE(c) (c) == ' ' || (c) == '\t' || (c) == '\n'
#define _IS_STRIPPABLE(c) (c) == '"' || (c) == '\''

typedef enum Token {
	AMPERSAND,
	PIPE,
	GREATER,
	STRING,
	EOI
} Token;

extern const char* token_names[];

typedef struct Lexer {
	size_t pos;
	int cchar;
	int symbol;
	size_t source_len;
	char* source;
	bool is_reading_string;
	size_t string_len;
	size_t string_pos;
	char* string;
} Lexer;

Lexer lexer_new(char* source);
void lexer_free(Lexer* lexer);

int lexer_reset(Lexer* _this, char* source);
int lexer_next_symbol(Lexer* _this);
void lexer_unget_symbol(Lexer* _this);

int lexer_current_symbol(Lexer* _this);
char* lexer_current_string(Lexer* _this);

#endif // ANUBIS_TOKENISER_H
