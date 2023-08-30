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

#define DEFAULT_STRING_LEN 100

#define _LEXER_NULL_CHECK_RETURN(_this, returnValue) INSTANCE_NULL_CHECK_RETURN("lexer", _this, returnValue); INSTANCE_NULL_CHECK_RETURN("lexer->source", _this, returnValue)
#define _LEXER_NULL_CHECK(_this) INSTANCE_NULL_CHECK("lexer", _this)

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define DEC_FLOOR(value) ((value) > 0 ? (value) - 1 : (value))
#define DEC_INVERT(value) ((value) > 0 ? (value) : 1)

void strip_ending(char* string) {
	for (int i = MAX(strlen(string), 0); i >= 0; i--) {
		if (_IS_STRIPPABLE(string[i])) {
			string[i] = '\0';
			continue;
		}
		return;
	}
}

// 0: Finished, 1: Ignore, Other: failure
int next_string(Lexer* _this) {
	_LEXER_NULL_CHECK_RETURN(_this, -1);
	if (_this->pos <= _this->string_pos) {
		return 1;
	}
	checked_free(_this->string);
	_this->string = strndup(
		_this->source + DEC_FLOOR(_this->string_pos),
		_this->pos - DEC_INVERT(_this->string_pos)
	);
	if (_this->string == NULL) {
		ERROR(ENOMEM, "Unable to extract string in tokenised sequence");
		return ENOMEM;
	}
	//strip_ending(_this->string);
	_this->is_reading_string = false;
	return 0;
}

#define HANDLE_NEXT_STRING \
	if (_this->is_reading_string) { \
		res = next_string(_this); \
		if (res == 0) { \
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
		break

void remove_char(Lexer* _this) {
	memmove(&_this->source[_this->pos - 1], &_this->source[_this->pos], _this->source_len - _this->pos + 1);
	_this->source_len--;
	_this->cchar = _this->source[_this->pos - 1];
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
				fprintf(stderr, "DQ: %s\n", double_quoted ? "t" : "f");
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
			if (_this->string != NULL) {
				if ((_this->string = realloc(_this->string, DEFAULT_STRING_LEN)) == NULL) {
					ERROR(ENOMEM, "Unable to reallocate string to size %d", DEFAULT_STRING_LEN);
					return -1;
				}
				memset(_this->string, 0, sizeof(char) * DEFAULT_STRING_LEN);
			} else {
				_this->string = calloc(DEFAULT_STRING_LEN, sizeof(char));
			}
			_this->string_len = 0;
			_this->string_pos = _this->pos;
			_this->is_reading_string = true;
			_this->symbol = STRING;
			next_char(_this);
			goto read;
	}
	return 1;
}

void lexer_unget_symbol(Lexer* _this) {
	_LEXER_NULL_CHECK(_this);
	_this->pos = DEC_FLOOR(_this->pos);
	_this->cchar = _this->source[_this->pos];
}

int lexer_current_symbol(Lexer* _this) {
	_LEXER_NULL_CHECK_RETURN(_this, -1);
	return _this->symbol;
}

char* lexer_current_string(Lexer* _this) {
	_LEXER_NULL_CHECK_RETURN(_this, NULL);
	return _this->string;
}
