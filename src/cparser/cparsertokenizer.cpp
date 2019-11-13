/*
 * cparsertokenizer.cpp
 *
 *  Created on: 12/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdio.h>
#include <cparsertools.h>
#include <cparsertokenizer.h>

namespace cparser {

//static const uint8_t * set_valid_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \t\r\n+-*/=\\\"'^&|~!?:;,.><#()[]{}";
static const uint8_t * set_empty_chars = _T " \t\r\n";
static const uint8_t * set_lead_identifier_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const uint8_t * set_identifier_chars = _T "_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const uint8_t * set_lead_number_literal_chars = _T "01234567890";
static const uint8_t * set_number_literal_chars = _T "0123456789xXbB.fFeEuUlL";
static const uint8_t * set_dual_operator_chars = _T "+-=&|><";
static const uint8_t * set_single_operator_chars = _T "*^~!";
static const uint8_t * set_single_char_token_chars = _T "?:;,.#()[]{}";


bool cparser_tokenizer::CharInSet(uint8_t c, const uint8_t *set)
{
	while (*set)
		if (c == *set++)
			return true;

	return false;
}

int16_t cparser_tokenizer::NextChar(FILE *f, uint32_t &row, uint32_t &column)
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

bool cparser_tokenizer::NextToken(FILE *f, token_s *tt, uint32_t flags)
{
	static uint32_t row = 1, column = 0;
	static int16_t last_char = 0;

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
	if (flags | CPARSER_TOKEN_FLAG_PARSE_DEFINE)
	{
		// >>>>>>>>>>>>>>>>>>>>>>>>>    DEFINE
		uint8_t *p = tt->str;

		// Row and column correspond to identifier
		tt->type = CPARSER_TOKEN_TYPE_DEFINE_LITERAL;
		tt->row = row;
		tt->column = column;

		// Copy first char to str
		*p++ = last_char;
		last_char = NextChar(f, row, column);

		// Copy identifier into str
		while ((last_char != EOF) && (
				*(p - 1) != '\n' ||
				(*(p - 1) == '\n' && *(p - 2) == '\\') ||
				(*(p - 1) == '\n' && *(p - 2) == '\r' && *(p - 3) == '\\')
				))
		{
			*p++ = last_char;
			last_char = NextChar(f, row, column);
		}

		// End str
		*p = 0;
		return true;
	}
	else if (flags | CPARSER_TOKEN_FLAG_PARSE_INCLUDE)
	{
		// Include filename
		uint8_t *p = tt->str;

		// Set type
		tt->type = CPARSER_TOKEN_TYPE_INCLUDE_LITERAL;

		// Add < and first char
		*p++ = last_char;
		last_char = NextChar(f, row, column);
		*p++ = last_char;
		last_char = NextChar(f, row, column);

		// Add comment chars
		while (
				(last_char != EOF) &&
				(
					(*(p - 1) != '\n' && *(p - 1) != '>') ||
					(*(p - 1) == '\n' && *(p - 2) == '\\') ||
					(*(p - 1) == '\n' && *(p - 2) == '\r' && *(p - 3) == '\\')
				)
			)
		{
			*p++ = last_char;
			last_char = NextChar(f, row, column);
		}

		// End str
		*p = 0;
		return true;
	}
	else if (CharInSet(last_char, set_single_char_token_chars))
	{
		// >>>>>>>>>>>>>>>>>>>>>>>>>    Single char token
		tt->type = CPARSER_TOKEN_TYPE_SINGLE_CHAR;
		tt->row = row;
		tt->column = column;
		tt->str[0] = last_char;
		tt->str[1] = 0;

		// Prepare next char
		last_char = NextChar(f, row, column);

		return true;
	}
	else if (CharInSet(last_char, set_lead_identifier_chars))
	{
		// >>>>>>>>>>>>>>>>>>>>>>>>>    Identifier
		uint8_t *p = tt->str;

		// Row and column correspond to identifier
		tt->type = CPARSER_TOKEN_TYPE_IDENTIFIER;
		tt->row = row;
		tt->column = column;

		// Copy first char to str
		*p++ = last_char;
		last_char = NextChar(f, row, column);

		// Copy identifier into str
		while (CharInSet(last_char, set_identifier_chars))
		{
			*p++ = last_char;
			last_char = NextChar(f, row, column);
		}

		// End str
		*p = 0;

		return true;
	}
	else if (CharInSet(last_char, set_lead_number_literal_chars))
	{
		// >>>>>>>>>>>>>>>>>>>>>>>    Number literal
		uint8_t *p = tt->str;

		// Row and column correspond to literal
		tt->type = CPARSER_TOKEN_TYPE_NUMBER_LITERAL;
		tt->row = row;
		tt->column = column;

		// Copy first char to str
		*p++ = last_char;
		last_char = NextChar(f, row, column);

		// Copy string literal
		while (CharInSet(last_char, set_number_literal_chars))
		{
			*p++ = last_char;
			last_char = NextChar(f, row, column);
		}

		// End string
		*p = 0;

		return true;
	}
	else if (last_char == '\"')
	{
		// >>>>>>>>>>>>>>>>>>>>>>>>    String literal
		uint8_t *p = tt->str;

		// Row and column correspond to literal
		tt->type = CPARSER_TOKEN_TYPE_STRING_LITERAL;
		tt->row = row;
		tt->column = column;

		// Copy first 2 chars to str
		*p++ = last_char;
		last_char = NextChar(f, row, column);
		*p++ = last_char;
		last_char = NextChar(f, row, column);

		// Copy string literal
		while ((last_char != EOF) && ((*(p - 1) != '\"') || (*(p - 1) == '\"' && *(p - 2) == '\\')))
		{
			*p++ = last_char;
			last_char = NextChar(f, row, column);
		}

		// End string
		*p = 0;

		return true;
	}
	else if (last_char == '\'')
	{
		// >>>>>>>>>>>>>>>>>>>>>>>    Char literal
		uint8_t *p = tt->str;

		// Row and column correspond to literal
		tt->type = CPARSER_TOKEN_TYPE_CHAR_LITERAL;
		tt->row = row;
		tt->column = column;

		// Copy first char to str
		*p++ = last_char;
		last_char = NextChar(f, row, column);
		*p++ = last_char;
		last_char = NextChar(f, row, column);

		// Copy string literal
		while ((last_char != EOF) && ((*(p - 1) != '\'') || (*(p - 1) == '\'' && *(p - 2) == '\\')))
		{
			*p++ = last_char;
			last_char = NextChar(f, row, column);
		}

		// End string
		*p = 0;

		return true;
	}
	else if (CharInSet(last_char, set_dual_operator_chars))
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

		return true;
	}
	else if (CharInSet(last_char, set_single_operator_chars))
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

		return true;
	}
	else if (last_char == '/')
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
			uint8_t *p = tt->str + 1;

			// Set type
			tt->type = CPARSER_TOKEN_TYPE_C_COMMENT;

			// Add *
			*p++ = last_char;
			last_char = NextChar(f, row, column);

			// Add comment chars
			while (last_char != EOF && !(*(p - 1) == '/' && *(p - 2) == '*'))
			{
				*p++ = last_char;
				last_char = NextChar(f, row, column);
			}

			// End str
			*p = 0;
		}
		else if (last_char == '/')
		{
			// C++ comment
			uint8_t *p = tt->str + 1;

			// Set type
			tt->type = CPARSER_TOKEN_TYPE_CPP_COMMENT;

			// Add /
			*p++ = last_char;
			last_char = NextChar(f, row, column);

			// Add comment chars
			while (
					(last_char != EOF) &&
					(
						(*(p - 1) != '\n') ||
						(*(p - 1) == '\n' && *(p - 2) == '\\') ||
						(*(p - 1) == '\n' && *(p - 2) == '\r' && *(p - 3) == '\\')
					)
				)
			{
				*p++ = last_char;
				last_char = NextChar(f, row, column);
			}

			// End str
			*p = 0;
		}
		else
		{
			// Single operator
			tt->type = CPARSER_TOKEN_TYPE_OPERATOR;
			tt->str[1] = 0;
		}

		return true;
	}
	else
	{
		// >>>>>>>>>>>>>>>>>>>>>>>>>    Invalid character
		tt->type = CPARSER_TOKEN_TYPE_INVALID;
		tt->row = row;
		tt->column = column;
		tt->str[0] = last_char;
		tt->str[1] = 0;

		return false;
	}
}

} /* namespace cparser */
