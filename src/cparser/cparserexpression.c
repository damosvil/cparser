/*
 * cparserexpression.c
 *
 *  Created on: 7/12/2019
 *      Author: blue
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cparsertools.h"
#include "cparsertoken.h"
#include "cparserdictionary.h"
#include "cparserlinkedlist.h"
#include "cparserexpression.h"


#define VALID_OPERATORS_COUNT			19


static const uint8_t *valid_operators[VALID_OPERATORS_COUNT] = {
		_T "!", _T "%",  _T "&",  _T "&&",
		_T "*", _T "+",  _T "-",  _T "/",
		_T "<", _T "<<", _T "<=", _T "==", _T ">", _T ">=", _T ">>",
		_T "^", _T "|",  _T "||", _T "~"
};


static int GetNextChar(void *data)
{
	int res;
	char **pp = (char **)data;

	if (!**pp)
		return EOF;

	res = **pp;
	*pp = *pp + 1;

	return res;
}

bool ExpressionEvalPreprocessor(cparserdictionary_t *defines, const uint8_t *expression)
{
	token_t tt = { CPARSER_TOKEN_TYPE_SINGLE_CHAR, true, 1, 0, malloc(strlen(_t expression) + 1) };
	token_source_t source = { &expression, GetNextChar };
	cparserlinkedlist_t *list = NULL;
	bool error = false;

	while (TokenNext(&source, &tt, 0))
	{
		// Skip
		if ((tt.type == CPARSER_TOKEN_TYPE_CPP_COMMENT) || (tt.type == CPARSER_TOKEN_TYPE_C_COMMENT))
			continue;

		// Error
		error = (tt.type == CPARSER_TOKEN_TYPE_INVALID) ||
				(tt.type == CPARSER_TOKEN_TYPE_STRING_LITERAL) ||
				(tt.type == CPARSER_TOKEN_TYPE_DEFINE_LITERAL) ||
				(tt.type == CPARSER_TOKEN_TYPE_INCLUDE_LITERAL) ||
				(tt.type == CPARSER_TOKEN_TYPE_SINGLE_CHAR && tt.str[0] != '(' && tt.str[0] != ')') ||
				(tt.type == CPARSER_TOKEN_TYPE_OPERATOR && !StringInAscendingSet(tt.str, valid_operators, VALID_OPERATORS_COUNT));
		if (error)
			break;

		if (list == NULL)
		{
			list = LinkedListNew(strdup(_t tt.str));
		}
		else
		{
			list = LinkedListInsertAfter(list, strdup(_t tt.str));
		}
	}

	LinkedListPrint(list);
	__builtin_trap(); // TODO: if literal

	// Free allocated double linked list and the items it contains
	while (list != NULL)
	{
		free(LinkedListGetItem(list));
		list = LinkedListDelete(list);
	}

	// Free allocated string buffer
	free(tt.str);

	return false;
}


