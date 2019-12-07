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
#include "cparsertoken.h"
#include "cparserdictionary.h"

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

bool ExpressionEvalPreprocessor(cparserdictionary_t *defines, const char *expression)
{
	token_t tt = { CPARSER_TOKEN_TYPE_SINGLE_CHAR, true, 1, 0, malloc(strlen(expression) + 1) };
	token_source_t source = { &expression, GetNextChar };

	while (TokenNext(&source, &tt, 0))
	{

	}

	// Free allocated string buffer
	free(tt.str);

	return false;
}


