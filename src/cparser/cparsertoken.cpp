/*
 * cparsertokenizer.cpp
 *
 *  Created on: 12/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdio.h>
#include <cparsertools.h>
#include <cparsertoken.h>

namespace cparser {


typedef bool (*acceptance_filter_callback_t)(uint16_t last_char, uint32_t length, uint8_t *end);


//static const uint8_t * set_valid_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \t\r\n+-*/=\\\"'^&|~!?:;,.><#()[]{}";
static const uint8_t * set_empty_chars = _T " \t\r\n";
static const uint8_t * set_lead_identifier_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const uint8_t * set_identifier_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const uint8_t * set_lead_number_literal_chars = _T "01234567890";
static const uint8_t * set_number_literal_chars = _T "0123456789xXbB.fFeEuUlL";
static const uint8_t * set_dual_operator_chars = _T "+-=&|><";
static const uint8_t * set_single_operator_chars = _T "*^~!";
static const uint8_t * set_single_char_token_chars = _T "?:;,.#()[]{}";


static bool CharInSet(uint8_t c, const uint8_t *set)
{
	while (*set)
		if (c == *set++)
			return true;

	return false;
}

static int16_t NextChar(FILE *f, uint32_t &row, uint32_t &column)
{
	int16_t res = fgetc(f);

	if (res == '\n')
	{
		row++;
		column = 1;
	} else if (res != EOF)
	{
		column++;
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
	return 	(length > 1 && *(end - 1) != '\"') ||
			(length > 2 && *(end - 1) == '\"' && *(end - 2) == '\\');
}

static bool ParseCharLiteralAcceptanceFilter(uint16_t last_char, uint32_t length, uint8_t *end)
{
	return 	(length > 1 && *(end - 1) != '\'') ||
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
static void ParseDigestString(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, uint8_t *str, acceptance_filter_callback_t filter)
{
	uint8_t *p = str;

	// Copy first char to str
	*p++ = last_char;
	last_char = NextChar(f, row, column);

	// Copy identifier into str
	while ((last_char != EOF) && filter(last_char, p - str, p))
	{
		*p++ = last_char;
		last_char = NextChar(f, row, column);
	}

	// End str
	*p = 0;
}

static void ParseDefineLiteral(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    DEFINE
	tt->type = CPARSER_TOKEN_TYPE_DEFINE_LITERAL;
	tt->row = row;
	tt->column = column;

	// Digest define literal with Cpp comment filter (they behave exactly the same)
	ParseDigestString(f, row, column, last_char, tt->str, ParseCppCommentAcceptanceFilter);
}

static void ParseIncludeFilename(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// Include file name literal
	tt->type = CPARSER_TOKEN_TYPE_INCLUDE_LITERAL;
	tt->row = row;
	tt->column = column;

	// Digest include literal with include acceptance filter
	ParseDigestString(f, row, column, last_char, tt->str, ParseIncludeAcceptanceFilter);
}

static void ParseSingleCharToken(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    Single char token
	tt->type = CPARSER_TOKEN_TYPE_SINGLE_CHAR;
	tt->row = row;
	tt->column = column;
	tt->str[0] = last_char;
	tt->str[1] = 0;

	// Prepare next char
	last_char = NextChar(f, row, column);
}

static void ParseIdentifier(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    Identifier
	tt->type = CPARSER_TOKEN_TYPE_IDENTIFIER;
	tt->row = row;
	tt->column = column;

	// Digest identifier with identifier acceptance filter
	ParseDigestString(f, row, column, last_char, tt->str, ParseIdentifierAcceptanceFilter);
}

static void ParseNumberLiteral(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    Number literal
	tt->type = CPARSER_TOKEN_TYPE_NUMBER_LITERAL;
	tt->row = row;
	tt->column = column;

	// Digest number literal with number acceptance filter
	ParseDigestString(f, row, column, last_char, tt->str, ParseNumberLiteralAcceptanceFilter);
}

static void ParseStringLiteral(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>    String literal
	tt->type = CPARSER_TOKEN_TYPE_STRING_LITERAL;
	tt->row = row;
	tt->column = column;

	// Digest string literal with string acceptance filter
	ParseDigestString(f, row, column, last_char, tt->str, ParseStringLiteralAcceptanceFilter);
}

static void ParseCharLiteral(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    Char literal
	tt->type = CPARSER_TOKEN_TYPE_CHAR_LITERAL;
	tt->row = row;
	tt->column = column;

	// Digest char literal with char acceptance filter
	ParseDigestString(f, row, column, last_char, tt->str, ParseCharLiteralAcceptanceFilter);
}

static void ParseDualOperator(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    =, ==, +, ++, +=, -, --, -=, |, ||, |=, &, &&, &=, >, >=, >>
	// Row and column correspond to operator
	tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
	tt->row = row;
	tt->column = column;
	tt->str[0] = last_char;

	// Prepare next char
	last_char = NextChar(f, row, column);

	if ((last_char == tt->str[0]) || (last_char == '='))
	{
		// Dual operator
		tt->str[1] = last_char;
		tt->str[2] = 0;

		// Prepare next char
		last_char = NextChar(f, row, column);
	}
	else
	{
		// Single operator
		tt->str[1] = 0;
	}
}

static void ParseSingleOperator(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>    *, *=, ^, ^=
	// Row and column correspond to symbol
	tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
	tt->row = row;
	tt->column = column;
	tt->str[0] = last_char;

	// Prepare next char
	last_char = NextChar(f, row, column);

	if (last_char == '=')
	{
		// Dual operator
		tt->str[1] = last_char;
		tt->str[2] = 0;

		// Prepare next char
		last_char = NextChar(f, row, column);
	}
	else
	{
		// Single operator
		tt->str[1] = 0;
	}
}

static void ParseSlash(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>    /, /=, /*, //
	// Row and column correspond to symbol
	tt->row = row;
	tt->column = column;
	tt->str[0] = last_char;

	// Prepare next char
	last_char = NextChar(f, row, column);

	if (last_char == '=')
	{
		// Dual operator
		tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
		tt->str[1] = last_char;
		tt->str[2] = 0;

		// Prepare next char
		last_char = NextChar(f, row, column);
	}
	else if (last_char == '*')
	{
		// C comment
		tt->type = CPARSER_TOKEN_TYPE_C_COMMENT;

		// Append comment string to str
		ParseDigestString(f, row, column, last_char, tt->str + 1, ParseCCommentAcceptanceFilter);
	}
	else if (last_char == '/')
	{
		// Set type
		tt->type = CPARSER_TOKEN_TYPE_CPP_COMMENT;

		// Append comment string to str
		ParseDigestString(f, row, column, last_char, tt->str + 1, ParseCppCommentAcceptanceFilter);
	}
	else
	{
		// Single operator
		tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
		tt->str[1] = 0;
	}
}

static void ParseInvalidCharacter(FILE *f, uint32_t &row, uint32_t &column, int16_t &last_char, token_s *tt)
{
	// >>>>>>>>>>>>>>>>>>>>>>>>>    Invalid character
	tt->type = CPARSER_TOKEN_TYPE_INVALID;
	tt->row = row;
	tt->column = column;
	tt->str[0] = last_char;
	tt->str[1] = 0;
}

bool TokenNext(FILE *f, token_s *tt, uint32_t flags)
{
	static uint32_t row = 1, column = 0;
	static int16_t last_char = 0;
	bool res = true;

	// In the beginning read next char
	if (row == 1 && column == 0)
	{
		last_char = NextChar(f, row, column);
	}

	// Skip empty chars
	while (CharInSet(last_char, set_empty_chars))
	{
		last_char = NextChar(f, row, column);
	}

	// Token discovery
	if (flags & CPARSER_TOKEN_FLAG_PARSE_DEFINE_LITERAL)
	{
		// Define literal
		ParseDefineLiteral(f, row, column, last_char, tt);
	}
	else if (flags & CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME)
	{
		// Include file name literal
		ParseIncludeFilename(f, row, column, last_char, tt);
	}
	else if (flags & CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER)
	{
		// Preprocessor token
		ParseIdentifier(f, row, column, last_char, tt);

		// Check define function macro
		if (last_char == '(')
		{
			// Add macro function parameters to definition identifier
			ParseDigestString(f, row, column, last_char, tt->str + StrLen(tt->str), ParseDefineFunctionParamsAcceptanceFilter);
		}
	}
	else if (CharInSet(last_char, set_single_char_token_chars))
	{
		// Single char token
		ParseSingleCharToken(f, row, column, last_char, tt);
	}
	else if (CharInSet(last_char, set_lead_identifier_chars))
	{
		// Identifier token
		ParseIdentifier(f, row, column, last_char, tt);
	}
	else if (CharInSet(last_char, set_lead_number_literal_chars))
	{
		// Number literal token
		ParseNumberLiteral(f, row, column, last_char, tt);
	}
	else if (last_char == '\"')
	{
		// String literal token
		ParseStringLiteral(f, row, column, last_char, tt);
	}
	else if (last_char == '\'')
	{
		// Character literal token
		ParseCharLiteral(f, row, column, last_char, tt);
	}
	else if (CharInSet(last_char, set_dual_operator_chars))
	{
		// Dual operator token
		ParseDualOperator(f, row, column, last_char, tt);
	}
	else if (CharInSet(last_char, set_single_operator_chars))
	{
		// Single operator token
		ParseSingleOperator(f, row, column, last_char, tt);
	}
	else if (last_char == '/')
	{
		// Slash token
		ParseSlash(f, row, column, last_char, tt);
	}
	else
	{
		// Invalid character
		ParseInvalidCharacter(f, row, column, last_char, tt);
		res = false;
	}

	return res;
}

} /* namespace cparser */
