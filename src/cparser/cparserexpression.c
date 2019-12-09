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

	while (TokenNext(&source, &tt, 0))
	{
		if (list == NULL)
		{
			list = LinkedListNew(strdup(_t tt.str));
		}
		else
		{
			list = LinkedListInsertAfter(list, strdup(_t tt.str));
		}
	}

	// Free allocated double linked list
	while (list != NULL)
		list = LinkedListDelete(list);

	// Free allocated string buffer
	free(tt.str);

	return false;
}


