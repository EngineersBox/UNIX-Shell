#include "lexer.h"
#include "error.h"
#include "checks.h"
#include "memutils.h"

#include <errno.h>
#include <string.h>
#include <stdint.h>

const char* token_names[] = {
	[AMPERSAND] = "AMPERSAND",
	[PIPE] = "PIPE",
	[GREATER] = "GREATER",
	[STRING] = "STRING",
	[EOI] = "EOI"
};

void next_char(Lexer* _this) {
	_this->cchar = _this->source[_this->pos++];
}

Lexer lexer_new(char* source) {
	Lexer lexer = {};
	lexer_reset(&lexer, source);
	return lexer;
}

void lexer_free(Lexer* lexer) {
	INSTANCE_NULL_CHECK("Lexer", lexer);
	checked_free(lexer->string);
	checked_free(lexer->source);
}

int lexer_reset(Lexer* _this, char* source) {
	INSTANCE_NULL_CHECK_RETURN("Lexer", _this, 0);
	INSTANCE_NULL_CHECK_RETURN("Source", source, 0);
	checked_free(_this->source);
	_this->source = strdup(source);
	_this->pos = 0;
	_this->symbol = -1;
	_this->source_len = strlen(source);
	if (_this->source_len == 0) {
		ERROR(EINVAL, "Source must not be empty\n");
		return 1;
	}
	_this->cchar = source[0];
	_this->is_reading_string = false;
	_this->string_len = 0;
	_this->string_pos = 0;
	if (_this->string != NULL) {
		free(_this->string);
		_this->string = NULL;
	}
	return 1;
}

void lexer_print_state(Lexer* _this) {
	fprintf(
		stderr,
		"STATE: {\n\tPOS: %zu\n\tCHAR: %c\n\tSYMBOL: %s\n\tSOURCE LEN: %zu\n\tSOURCE: %s\n\tIS READING STRING: %s\n\tSTRING LEN: %zu\n\tSTRING POS: %zu\n\tSTRING:%s\n}\n",
		_this->pos,
		_this->cchar,
		token_names[_this->symbol],
		_this->source_len,
		_this->source,
		_this->is_reading_string ? "true" : "false",
		_this->string_len,
		_this->string_pos,
		_this->string
	);
}

#define DEFAULT_STRING_LEN 100

#define _LEXER_NULL_CHECK_RETURN(_this, returnValue) INSTANCE_NULL_CHECK_RETURN("lexer", _this, returnValue); INSTANCE_NULL_CHECK_RETURN("lexer->source", _this, returnValue)
#define _LEXER_NULL_CHECK(_this) INSTANCE_NULL_CHECK("lexer", _this)

#define DEC_FLOOR(value) ((value) > 0 ? (value) - 1 : (value))
#define DEC_INVERT(value) ((value) > 0 ? (value) : 1)

// 0: Finished, 1: Ignore, Other: failure
int next_string(Lexer* _this) {
	_LEXER_NULL_CHECK_RETURN(_this, -1);
	if (_this->pos < _this->string_pos) {
		return 0;
	}
	checked_free(_this->string);
	size_t start = _this->string_pos;
	size_t end = DEC_FLOOR(_this->pos);
	fprintf(stderr, "STRING START: %c @ %zu\n", _this->source[start], start);
	fprintf(stderr, "STRING END  : %c @ %zu\n", _this->source[end - 1], end - 1);
	_this->string = strndup(&_this->source[start], end - start);
	_this->string_len = end - start;
	fprintf(stderr, "STRING: [%s]\n", _this->string);
	if (_this->string == NULL) {
		ERROR(ENOMEM, "Unable to extract string in tokenised sequence");
		return ENOMEM;
	}
	_this->is_reading_string = false;
	lexer_print_state(_this);
	return 1;
}

#define HANDLE_NEXT_STRING \
	if (_this->is_reading_string) { \
		res = next_string(_this); \
		if (res == 1) { \
			break; \
		} else if (res != 1) { \
			return res; \
		} \
	}

#define SINGLE_TOKEN_CASE(literal, token) \
	case literal:\
		HANDLE_NEXT_STRING;\
		next_char(_this);\
		_this->symbol = (token);\
		fprintf(stderr, "TOK OUT: %c -> %c\n", literal, _this->cchar);\
		break

void remove_char(Lexer* _this) {
	lexer_print_state(_this);
	memmove(&_this->source[_this->pos - 1], &_this->source[_this->pos], _this->source_len - _this->pos + 1);
	fprintf(stderr, "REMOVED '%c' STR: [%s]\n", _this->source[_this->pos - 1], _this->source);
	_this->source_len--;
	_this->pos--;
	next_char(_this);
	lexer_print_state(_this);
}

int lexer_next_symbol(Lexer* _this) {
	_LEXER_NULL_CHECK_RETURN(_this, -1);
	if (_this->pos >= _this->source_len - 1) {
		return 0;
	}
	int res;
	bool escape = false;
	bool single_quoted = false;
	bool double_quoted = false;
read:
	switch (_this->cchar) {
		case '\0':
			HANDLE_NEXT_STRING;
			break;
		case ' ':
		case '\t':
		case '\n':
			if (escape) {
				next_char(_this);
				escape = false;
				goto read;
			}
			if (single_quoted || double_quoted) {
				goto string;
			}
			HANDLE_NEXT_STRING
			next_char(_this);
			goto read;
		SINGLE_TOKEN_CASE(EOF, EOI);
		SINGLE_TOKEN_CASE(_TOK_AMPERSAND, AMPERSAND);
		SINGLE_TOKEN_CASE(_TOK_PIPE, PIPE);
		SINGLE_TOKEN_CASE(_TOK_GREATER, GREATER);
		case '\\':
			escape = true;
			goto string;
		case '"':
			if (!escape) {
				double_quoted = !double_quoted;
				if (!double_quoted) {
					remove_char(_this);
				} else {
					next_char(_this);
				}
				goto read;
			}
			goto string;
		case '\'':
			if (!escape) {
				single_quoted = !single_quoted;
				if (!double_quoted) {
					remove_char(_this);
				} else {
					next_char(_this);
				}
				goto read;
			}
			goto string;
		default:
string:
			if (_this->is_reading_string) {
				next_char(_this);
				goto read;
			}
			_this->string_len = 0;
			_this->string_pos = DEC_FLOOR(_this->pos);
			_this->is_reading_string = true;
			_this->symbol = STRING;
			fprintf(stderr, "NEW STRING: %c %zu\n", _this->source[_this->string_pos], _this->string_pos);
			next_char(_this);
			goto read;
	}
	return 1;
}

int lexer_current_symbol(Lexer* _this) {
	_LEXER_NULL_CHECK_RETURN(_this, -1);
	return _this->symbol;
}

char* lexer_current_string(Lexer* _this) {
	_LEXER_NULL_CHECK_RETURN(_this, NULL);
	return _this->string;
}
