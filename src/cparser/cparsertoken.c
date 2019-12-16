/*
 * cparsertokenizer.cpp
 *
 *  Created on: 12/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cparsertools.h>
#include <cparsertoken.h>


typedef bool (*acceptance_filter_callback_t)(uint16_t last_char, uint32_t length, uint8_t *end);


//static const uint8_t * set_valid_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \t\r\n+-*/=\\\"'^&|~!?:;,.><#()[]{}";
static const uint8_t * set_empty_chars = _T " \t\r\n";
static const uint8_t * set_lead_identifier_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const uint8_t * set_identifier_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const uint8_t * set_lead_number_literal_chars = _T "01234567890";
static const uint8_t * set_number_literal_chars = _T "0123456789xXbB.fFeEuUlL";
static const uint8_t * set_dual_operator_chars = _T "+-=&|><";
static const uint8_t * set_single_operator_chars = _T "*%^~!";
static const uint8_t * set_single_char_token_chars = _T "?:;,.#()[]{}";


static bool CharInSet(uint8_t c, const uint8_t *set)
{
	while (*set)
		if (c == *set++)
			return true;

	return false;
}

static int16_t NextChar(token_source_t *source, uint32_t *p_row, uint32_t *p_column)
{
	int res = source->read(source->from);

	if (res == '\n')
	{
		*p_row = *p_row + 1;;
		*p_column = 1;
	}
	else if (res != EOF)
	{
		*p_column = *p_column + 1;
	}

	return res;
}

static bool ParseIncludeAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return 	(length > 0 && *(end - 1) != '\n' && *(end - 1) != '>') ||
			(length > 1 && *(end - 1) == '\n' && *(end - 2) == '\\') ||
			(length > 2 && *(end - 1) == '\n' && *(end - 2) == '\r' && *(end - 3) == '\\');
}

static bool ParseIdentifierAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return 	CharInSet(last_char, set_identifier_chars);
}

static bool ParseNumberLiteralAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return 	CharInSet(last_char, set_number_literal_chars);
}

static bool ParseStringLiteralAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return 	(length == 1) ||
			(length > 1 && *(end - 1) != '\"') ||
			(length > 2 && *(end - 1) == '\"' && *(end - 2) == '\\');
}

static bool ParseCharLiteralAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return 	(length == 1) ||
			(length > 1 && *(end - 1) != '\'') ||
			(length > 2 && *(end - 1) == '\'' && *(end - 2) == '\\');
}

static bool ParseCCommentAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return 	!(length > 2 && *(end - 1) == '/' && *(end - 2) == '*');
}

static bool ParseCppCommentAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return  (last_char != '\n') ||
			(length > 0 && last_char == '\n' && *(end - 1) == '\\') ||
			(length > 1 && last_char == '\n' && *(end - 1) == '\r' && *(end - 2) == '\\');
}

static bool ParseDefineFunctionParamsAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return  (length > 0 && *(end - 1) != ')') ||
			(length > 1 && *(end - 1) == '\n' && *(end - 2) == '\\') ||
			(length > 2 && *(end - 1) == '\n' && *(end - 2) == '\r' && *(end - 3) == '\\');
}

/**
 * Digests a string from a file
 *
 * \param[in]		f: 		File from which to digest characters
 * \param[in/out] 	row:	row in text file
 * \param[in/out]	column:	column in text file
 * \param[out]		str:	string buffer to put the digested bytes
 * \param[in]		filter:	Filter that returns true if the byte processed is valid
 */
static void ParseDigestString(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, uint8_t *str, acceptance_filter_callback_t filter)
{
	uint8_t *p = str;

	// Copy first char to str
	*p++ = *p_last_char;
	*p_last_char = NextChar(source, p_row, p_column);

	// Copy identifier into str
	while ((*p_last_char != EOF) && filter(*p_last_char, p - str, p) && ((p - str) < MAX_SENTENCE_LENGTH))
	{
		*p++ = *p_last_char;
		*p_last_char = NextChar(source, p_row, p_column);
	}

	// End str
	*p = 0;
}

static void ParseDefineLiteral(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    DEFINE
	tt->type = CPARSER_TOKEN_TYPE_DEFINE_LITERAL;
	tt->row = *p_row;
	tt->column = *p_column;

	// Check if current char for define literal is different
	if (*p_last_char != '\r' && *p_last_char != '\n')
	{
		// Digest define literal with Cpp comment filter (they behave exactly the same)
		ParseDigestString(source, p_row, p_column, p_last_char, tt->str, ParseCppCommentAcceptanceFilter);
	}
	else
	{
		// No literal so assign empty string
		tt->str[0] = 0;
	}
}

static void ParseIncludeFilename(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// Include file name literal
	tt->type = CPARSER_TOKEN_TYPE_INCLUDE_LITERAL;
	tt->row = *p_row;
	tt->column = *p_column;

	// Digest include literal with include acceptance filter
	ParseDigestString(source, p_row, p_column, p_last_char, tt->str, ParseIncludeAcceptanceFilter);
}

static void ParseSingleCharToken(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    Single char token
	tt->type = CPARSER_TOKEN_TYPE_SINGLE_CHAR;
	tt->row = *p_row;
	tt->column = *p_column;
	tt->str[0] = *p_last_char;
	tt->str[1] = 0;

	// Prepare next char
	*p_last_char = NextChar(source, p_row, p_column);
}

static void ParseIdentifier(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    Identifier
	tt->type = CPARSER_TOKEN_TYPE_IDENTIFIER;
	tt->row = *p_row;
	tt->column = *p_column;

	// Digest identifier with identifier acceptance filter
	ParseDigestString(source, p_row, p_column, p_last_char, tt->str, ParseIdentifierAcceptanceFilter);
}

static void ParseNumberLiteral(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    Number literal
	tt->type = CPARSER_TOKEN_TYPE_NUMBER_LITERAL;
	tt->row = *p_row;
	tt->column = *p_column;

	// Digest number literal with number acceptance filter
	ParseDigestString(source, p_row, p_column, p_last_char, tt->str, ParseNumberLiteralAcceptanceFilter);
}

static void ParseStringLiteral(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>    String literal
	tt->type = CPARSER_TOKEN_TYPE_STRING_LITERAL;
	tt->row = *p_row;
	tt->column = *p_column;

	// Digest string literal with string acceptance filter
	ParseDigestString(source, p_row, p_column, p_last_char, tt->str, ParseStringLiteralAcceptanceFilter);
}

static void ParseCharLiteral(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    Char literal
	tt->type = CPARSER_TOKEN_TYPE_CHAR_LITERAL;
	tt->row = *p_row;
	tt->column = *p_column;

	// Digest char literal with char acceptance filter
	ParseDigestString(source, p_row, p_column, p_last_char, tt->str, ParseCharLiteralAcceptanceFilter);
}

static void ParseDualOperator(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    =, ==, +, ++, +=, -, --, -=, |, ||, |=, &, &&, &=, >, >=, >>
	// Row and column correspond to operator
	tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
	tt->row = *p_row;
	tt->column = *p_column;
	tt->str[0] = *p_last_char;

	// Prepare next char
	*p_last_char = NextChar(source, p_row, p_column);

	if ((*p_last_char == tt->str[0]) || (*p_last_char == '='))
	{
		// Dual operator
		tt->str[1] = *p_last_char;
		tt->str[2] = 0;

		// Prepare next char
		*p_last_char = NextChar(source, p_row, p_column);
	}
	else
	{
		// Single operator
		tt->str[1] = 0;
	}
}

static void ParseSingleOperator(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    *, *=, %, %=, ^, ^=, !, !=, ~, ~=
	// Row and column correspond to symbol
	tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
	tt->row = *p_row;
	tt->column = *p_column;
	tt->str[0] = *p_last_char;

	// Prepare next char
	*p_last_char = NextChar(source, p_row, p_column);

	if (*p_last_char == '=')
	{
		// Dual operator
		tt->str[1] = *p_last_char;
		tt->str[2] = 0;

		// Prepare next char
		*p_last_char = NextChar(source, p_row, p_column);
	}
	else
	{
		// Single operator
		tt->str[1] = 0;
	}
}

static void ParseSlash(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>    /, /=, /*, //
	// Row and column correspond to symbol
	tt->row = *p_row;
	tt->column = *p_column;
	tt->str[0] = *p_last_char;

	// Prepare next char
	*p_last_char = NextChar(source, p_row, p_column);

	if (*p_last_char == '=')
	{
		// Dual operator
		tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
		tt->str[1] = *p_last_char;
		tt->str[2] = 0;

		// Prepare next char
		*p_last_char = NextChar(source, p_row, p_column);
	}
	else if (*p_last_char == '*')
	{
		// C comment
		tt->type = CPARSER_TOKEN_TYPE_C_COMMENT;

		// Append comment string to str
		ParseDigestString(source, p_row, p_column, p_last_char, tt->str + 1, ParseCCommentAcceptanceFilter);
	}
	else if (*p_last_char == '/')
	{
		// Set type
		tt->type = CPARSER_TOKEN_TYPE_CPP_COMMENT;

		// Append comment string to str
		ParseDigestString(source, p_row, p_column, p_last_char, tt->str + 1, ParseCppCommentAcceptanceFilter);
	}
	else
	{
		// Single operator
		tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
		tt->str[1] = 0;
	}
}

static void ParseInvalidCharacter(token_source_t *source, uint32_t *p_row, uint32_t *p_column, int16_t *p_last_char, token_t *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    Invalid character
	tt->type = CPARSER_TOKEN_TYPE_INVALID;
	tt->row = *p_row;
	tt->column = *p_column;
	tt->str[0] = *p_last_char;
	tt->str[1] = 0;
}

token_t *TokenNew(void)
{
	token_t *tt = malloc(sizeof(token_t));

	tt->type = CPARSER_TOKEN_TYPE_INVALID;
	tt->first_token_in_line = true;
	tt->row = 1;
	tt->column = 0;
	tt->last_char = 0;
	tt->str = malloc(MAX_SENTENCE_LENGTH + 10);

	return tt;
}

void TokenDelete(token_t *tt)
{
	// Delete string buffer in token and token itself
	free(tt->str);
	free(tt);
}

bool TokenNext(token_t *tt, token_source_t *source, uint32_t flags)
{
	bool res = true;

	// In the beginning source next char
	if (tt->row == 1 && tt->column == 0)
	{
		tt->first_token_in_line = true;			// In the beginning first token in line shall be true
		tt->last_char = NextChar(source, &tt->row, &tt->column);
	}
	else
	{
		// Clear first token in line
		tt->first_token_in_line = false;
	}

	// Skip empty chars if define literal (they can be empty)
	if (flags & CPARSER_TOKEN_FLAG_PARSE_PREPROCESSOR_LITERAL)
	{
		// Skip spaces and tabs
		while (tt->last_char == ' ' || tt->last_char == '\t')
		{
			tt->last_char = NextChar(source, &tt->row, &tt->column);
		}
	}
	else
	{
		// Skip spaces, tabs, new lines and returns
		while (CharInSet(tt->last_char, set_empty_chars))
		{
			tt->first_token_in_line |= tt->last_char == '\n';
			tt->last_char = NextChar(source, &tt->row, &tt->column);
		}
	}

	// Token discovery
	if (flags & CPARSER_TOKEN_FLAG_PARSE_PREPROCESSOR_LITERAL)
	{
		// Define literal
		ParseDefineLiteral(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (flags & CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME)
	{
		// Include file name literal
		ParseIncludeFilename(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (flags & CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER)
	{
		// Preprocessor token
		ParseIdentifier(source, &tt->row, &tt->column, &tt->last_char, tt);

		// Check define function macro
		if (tt->last_char == '(')
		{
			// Add macro function parameters to definition identifier
			ParseDigestString(source, &tt->row, &tt->column, &tt->last_char, tt->str + strlen(_t tt->str), ParseDefineFunctionParamsAcceptanceFilter);
		}
	}
	else if (CharInSet(tt->last_char, set_single_char_token_chars))
	{
		// Single char token
		ParseSingleCharToken(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (CharInSet(tt->last_char, set_lead_identifier_chars))
	{
		// Identifier token
		ParseIdentifier(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (CharInSet(tt->last_char, set_lead_number_literal_chars))
	{
		// Number literal token
		ParseNumberLiteral(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (tt->last_char == '\"')
	{
		// String literal token
		ParseStringLiteral(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (tt->last_char == '\'')
	{
		// Character literal token
		ParseCharLiteral(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (CharInSet(tt->last_char, set_dual_operator_chars))
	{
		// Dual operator token
		ParseDualOperator(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (CharInSet(tt->last_char, set_single_operator_chars))
	{
		// Single operator token
		ParseSingleOperator(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else if (tt->last_char == '/')
	{
		// Slash token
		ParseSlash(source, &tt->row, &tt->column, &tt->last_char, tt);
	}
	else
	{
		// Invalid character
		ParseInvalidCharacter(source, &tt->row, &tt->column, &tt->last_char, tt);
		res = false;
	}

	return res;
}
