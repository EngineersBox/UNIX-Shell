#include "tokeniser.h"
#include "error.h"

#include <errno.h>
#include <string.h>

Tokeniser tokeniser_new(char* source) {
	return (Tokeniser) {
		.pos = 0,
		.cchar = ' ',
		.symbol = -1,
		.source_len = strlen(source),
		.source = source,
		.is_reading_string = false,
		.string_len = 0,
		.string_pos = 0,
		.string = NULL,
	};
}

int tokeniser_reset(Tokeniser* _this, char* source) {
	if (_this == NULL) {
		ERROR(EINVAL, "Cannot reset NULL tokeniser instance");
		return EINVAL;
	}
	_this->source = source;
	_this->pos = 0;
	_this->cchar = ' ';
	_this->symbol = -1;
	_this->source_len = strlen(source);
	_this->is_reading_string = false;
	_this->string_len = 0;
	_this->string_pos = 0;
	if (_this->string != NULL) {
		free(_this->string);
		_this->string = NULL;
	}
	return 1;
}

void next_char(Tokeniser* _this) {
	_this->cchar = _this->source[_this->pos++];
}

#define DEFAULT_STRING_LEN 100

// 0: Finished, 1: Ignore, Other: failure
int next_string(Tokeniser* _this) {
	if (_this->pos <= _this->string_pos) {
		return 1;
	} else if (_this->string != NULL) {
		free(_this->string);
	}
	if ((_this->string = strndup(_this->source + _this->string_pos, _this->pos - _this->string_pos)) == NULL) {
		ERROR(ENOMEM, "Unable to extract string in tokenised sequence");
		return ENOMEM;
	}
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
	} \

int tokeniser_next_symbol(Tokeniser* _this) {
	if (_this->pos >= _this->source_len - 1) {
		return 0;
	}
	int res;
	bool escape = false;
read:
	switch (_this->cchar) {
		case ' ':
		case '\t':
		case '\n':
			if (escape) {
				next_char(_this);
				escape = false;
				goto read;
			}
			HANDLE_NEXT_STRING
			next_char(_this);
			goto read;
		case EOF:
			HANDLE_NEXT_STRING
			next_char(_this);
			_this->symbol = EOI;
			break;
		case _TOK_AMPERSAND:
			HANDLE_NEXT_STRING
			next_char(_this);
			_this->symbol = AMPERSAND;
			break;
		case _TOK_PIPE:
			HANDLE_NEXT_STRING
			next_char(_this);
			_this->symbol = PIPE;
			break;
		case _TOK_GREATER:
			HANDLE_NEXT_STRING
			next_char(_this);
			_this->symbol = GREATER;
			break;
		case '\\':
			escape = true;
		default:
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
			_this->string_pos = _this->pos - 1;
			_this->is_reading_string = true;
			_this->symbol = STRING;
			goto read;
	}
	return 1;
}

void tokeniser_unget_token(Tokeniser* _this) {
	_this->cchar = _this->source[--_this->pos];
}

int tokeniser_current_symbol(Tokeniser* _this) {
	return _this->symbol;
}

char* tokeniser_current_string(Tokeniser* _this) {
	return _this->string;
}
