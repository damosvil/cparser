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
#define VALID_UNARY_OPERATORS_COUNT		4
#define STR(A)							(#A)


typedef enum expression_token_type_e
{
	EXPRESSION_TOKEN_TYPE_DEFINED,
	EXPRESSION_TOKEN_TYPE_IDENTIFIER,
	EXPRESSION_TOKEN_TYPE_OPERATOR,
	EXPRESSION_TOKEN_TYPE_OPEN,
	EXPRESSION_TOKEN_TYPE_CLOSE,
	EXPRESSION_TOKEN_TYPE_DECODED_VALUE
} expression_token_type_t;

typedef struct expression_token_s
{
	expression_token_type_t type;
	void *data;
} expression_token_t;


static const uint8_t *valid_operators[VALID_OPERATORS_COUNT] = {
		_T "!", _T "%",  _T "&",  _T "&&",
		_T "*", _T "+",  _T "-",  _T "/",
		_T "<", _T "<<", _T "<=", _T "==", _T ">", _T ">=", _T ">>",
		_T "^", _T "|",  _T "||", _T "~"
};

static const uint8_t *valid_unary_operators[VALID_UNARY_OPERATORS_COUNT] = {
		_T "!", _T "+",  _T "-",  _T "~"
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

static void ExpressionTokenDelete(expression_token_t *et)
{
	if (et == NULL)
		return;

	// Delete expression token data and token
	if ((et->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE) && (et->data != NULL))
		free(et->data);
	free(et);
}

static void LinkedExpressionListDelete(cparserlinkedlist_t *l)
{
	expression_token_t *et = NULL;

	while (l != NULL)
	{
		// Get expression token
		et = LinkedListGetItem(l);

		ExpressionTokenDelete(et);

		// Delete linked list node
		l = LinkedListDelete(l);
	}
}

static void LinkedExpressionListPrint(cparserlinkedlist_t *l)
{
	expression_token_t *et = NULL;

	l = LinkedListFirst(l);

	while (l != NULL)
	{
		et = LinkedListGetItem(l);

		switch (et->type)
		{

		case EXPRESSION_TOKEN_TYPE_DEFINED:
			printf("<%s:defined> ", STR(EXPRESSION_TOKEN_TYPE_DEFINED));
			break;

		case EXPRESSION_TOKEN_TYPE_IDENTIFIER:
			printf("<%s:%s> ", STR(EXPRESSION_TOKEN_TYPE_IDENTIFIER), (char *)et->data);
			break;

		case EXPRESSION_TOKEN_TYPE_OPERATOR:
			printf("<%s:%s> ", STR(EXPRESSION_TOKEN_TYPE_OPERATOR), (char *)et->data);
			break;

		case EXPRESSION_TOKEN_TYPE_OPEN:
			printf("<%s:(> ", STR(EXPRESSION_TOKEN_TYPE_OPEN));
			break;

		case EXPRESSION_TOKEN_TYPE_CLOSE:
			printf("<%s:)> ", STR(EXPRESSION_TOKEN_TYPE_CLOSE));
			break;

		case EXPRESSION_TOKEN_TYPE_DECODED_VALUE:
			printf("<%s:%ld> ", STR(EXPRESSION_TOKEN_TYPE_DECODED_VALUE), (int64_t)et->data);
			break;

		default:
			break;

		}

		l = LinkedListNext(l);
	}

	printf("\n");
}

static cparserexpression_preprocessor_result_t LinkedExpressionListComputeDefined(cparserlinkedlist_t *l, cparserdictionary_t *defines)
{
	enum eval_defined_state_e
	{
		EVAL_DEFINED_STATE_IDLE = 0,
		EVAL_DEFINED_STATE_LOOKING
	} state = EVAL_DEFINED_STATE_IDLE;

	cparserexpression_preprocessor_result_t res = EXPRESSION_PREPROCESSOR_RESULT_TRUE;
	expression_token_t *et = NULL;
	uint32_t level = 0;

	// Process tokens
	while ((l != NULL) && (res == EXPRESSION_PREPROCESSOR_RESULT_TRUE))
	{
		// Get item from linked list node
		et = LinkedListGetItem(l);

		// Skip tokens other than defined operator
		switch (state)
		{

		case EVAL_DEFINED_STATE_IDLE:
			if (et->type == EXPRESSION_TOKEN_TYPE_DEFINED)
			{
				state = EVAL_DEFINED_STATE_LOOKING;
			}

			// Move to next linked list node
			l = LinkedListNext(l);
			break;

		case EVAL_DEFINED_STATE_LOOKING:
			if (et->type == EXPRESSION_TOKEN_TYPE_OPEN)
			{
				// Increase level
				level++;

				// Remove parenthesis
				ExpressionTokenDelete(et);
				l = LinkedListDelete(l);
			}
			else if (et->type == EXPRESSION_TOKEN_TYPE_IDENTIFIER)
			{
				// Replace previous define and definition identifier by a boolean decoded value
				int64_t value = DictionaryExistsKey(defines, et->data) ? 1 : 0;
				cparserlinkedlist_t *defined = LinkedListPrevious(l);

				// Remove identifier
				ExpressionTokenDelete(LinkedListGetItem(l));
				l = LinkedListDelete(l);

				// Check and delete trailing close parenthesis
				while ((l != NULL) && (level > 0))
				{
					et = LinkedListGetItem(l);
					if (et->type != EXPRESSION_TOKEN_TYPE_CLOSE)
						break;

					level--;

					ExpressionTokenDelete(et);
					l = LinkedListDelete(l);
				}

				// Update defined operator by removing previous expression token and creating a decoded value
				if (level == 0)
				{
					// Get defined expression token and remove it
					et = LinkedListGetItem(defined);
					ExpressionTokenDelete(et);

					// Create new expression token
					et = malloc(sizeof(expression_token_type_t));
					et->type = EXPRESSION_TOKEN_TYPE_DECODED_VALUE;
					et->data = (void *)value;

					// Select next linked list token to defined
					l = LinkedListNext(defined);

					// Return to idle state
					state = EVAL_DEFINED_STATE_IDLE;
				}
				else
				{
					// Syntax error: incorrect number of closing parenthesis
					res = EXPRESSION_PREPROCESSOR_RESULT_ERROR_CLOSING_PARENTHESYS_DOES_NOT_MATCH;
				}
			}
			else
			{
				// Syntax error: defined operator syntax error
				res = EXPRESSION_PREPROCESSOR_RESULT_ERROR_DEFINED_OPERATOR;
			}
			break;

		default:
			res = EXPRESSION_PREPROCESSOR_RESULT_ERROR_DEFINED_EVAL;
			break;

		}
	}

	return (state == EVAL_DEFINED_STATE_IDLE) ? res :
		EXPRESSION_PREPROCESSOR_RESULT_ERROR_DEFINED_WITHOUT_IDENTIFIER;
}

static cparserexpression_preprocessor_result_t LinkedExpressionListReplaceDefinitions(cparserlinkedlist_t *l, cparserdictionary_t *defines)
{
	return EXPRESSION_PREPROCESSOR_RESULT_TRUE;
}

static int64_t ComputeUnary(const uint8_t *op, int64_t a)
{
	if ((op[0] == '!') && (op[1] == 0))
	{
		return !a;
	}
	else if ((op[0] == '~') && (op[1] == 0))
	{
		return ~a;
	}
	else if ((op[0] == '+') && (op[1] == 0))
	{
		return +a;
	}
	else if ((op[0] == '-') && (op[1] == 0))
	{
		return -a;
	}
	else
	{
		return 0;
	}
}

static int64_t ComputeBinary(int64_t a, const uint8_t *op, int64_t b)
{
	if ((op[0] == '+') && (op[1] == 0))
	{
		return a + b;
	}
	else if ((op[0] == '-') && (op[1] == 0))
	{
		return a - b;
	}
	else if ((op[0] == '*') && (op[1] == 0))
	{
		return a * b;
	}
	else if ((op[0] == '/') && (op[1] == 0))
	{
		return a / b;
	}
	else if ((op[0] == '%') && (op[1] == 0))
	{
		return a % b;
	}
	else if ((op[0] == '&') && (op[1] == 0))
	{
		return a & b;
	}
	else if ((op[0] == '|') && (op[1] == 0))
	{
		return a | b;
	}
	else if ((op[0] == '^') && (op[1] == 0))
	{
		return a ^ b;
	}
	else if ((op[0] == '<') && (op[1] == 0))
	{
		return a < b;
	}
	else if ((op[0] == '>') && (op[1] == 0))
	{
		return a > b;
	}
	else if ((op[0] == '&') && (op[1] == '&') && (op[2] == 0))
	{
		return a && b;
	}
	else if ((op[0] == '|') && (op[1] == '|') && (op[2] == 0))
	{
		return a || b;
	}
	else if ((op[0] == '<') && (op[1] == '<') && (op[2] == 0))
	{
		return a << b;
	}
	else if ((op[0] == '>') && (op[1] == '>') && (op[2] == 0))
	{
		return a >> b;
	}
	else if ((op[0] == '<') && (op[1] == '=') && (op[2] == 0))
	{
		return a <= b;
	}
	else if ((op[0] == '>') && (op[1] == '=') && (op[2] == 0))
	{
		return a >= b;
	}
	else if ((op[0] == '=') && (op[1] == '=') && (op[2] == 0))
	{
		return a == b;
	}
	else
	{
		return 0;
	}
}

static cparserexpression_preprocessor_result_t LinkedExpressionListComputeUnary(cparserlinkedlist_t *l)
{
	// Process tokens
	while (l != NULL)
	{
		// Get item from linked list node
		expression_token_t * et = LinkedListGetItem(l);
		cparserlinkedlist_t *u = LinkedListPrevious(l);
		cparserlinkedlist_t *uu = LinkedListPrevious(u);
		expression_token_t *etu = LinkedListGetItem(u);
		expression_token_t *etuu = LinkedListGetItem(uu);

		if (
				// Value preceded by operator at the beginning of the expression
				(
					(etu != NULL) && (etuu == NULL) &&
					(et->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE)
				)
				||
				// Value preceded by operator also preceded by operator
				(
					(etu != NULL) && (etuu != NULL) &&
					(et->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE) &&
					(etu->type == EXPRESSION_TOKEN_TYPE_OPERATOR) &&
					(etuu->type == EXPRESSION_TOKEN_TYPE_OPERATOR)
				)
			)
		{
			// Unary operator before decoded value
			if (!StringInAscendingSet(etu->data, valid_unary_operators, VALID_UNARY_OPERATORS_COUNT))
			{
				return EXPRESSION_PREPROCESSOR_RESULT_ERROR_INVALID_UNARY_OPERATOR_IN_EXPRESSION;
			}
			else if (StrEq(etu->data, "-") && StrEq(etuu->data, "-"))
			{
				return EXPRESSION_PREPROCESSOR_RESULT_ERROR_MINUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_MINUS;
			}
			else if (StrEq(etu->data, "-") && StrEq(etuu->data, "-"))
			{
				return EXPRESSION_PREPROCESSOR_RESULT_ERROR_PLUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_PLUS;
			}
			else
			{
				int64_t value = ComputeUnary(etu->data, (int64_t)et->data);

				// Remove value node
				ExpressionTokenDelete(et);
				LinkedExpressionListDelete(l);

				// Delete previous operand token
				ExpressionTokenDelete(etu);

				// Update node to value
				etu = malloc(sizeof(expression_token_t));
				etu->type = EXPRESSION_TOKEN_TYPE_DECODED_VALUE;
				etu->data = (void *)value;
				LinkedListUpdateItem(u, etu);

				// Select new value as current linked list node
				l = u;
			}
		}
		else
		{
			// Select next token
			l = LinkedListNext(l);
		}
	}

	return EXPRESSION_PREPROCESSOR_RESULT_TRUE;
}

static cparserexpression_preprocessor_result_t LinkedExpressionListComputeBinary(cparserlinkedlist_t *l)
{
	enum eval_defined_state_e
	{
		EVAL_DEFINED_STATE_IDLE = 0,
	} state = EVAL_DEFINED_STATE_IDLE;

	expression_token_t *et = NULL;

	// Process tokens
	while (l != NULL)
	{
		// Get item from linked list node
		et = LinkedListGetItem(l);

		// Skip tokens other than defined operator
		switch (state)
		{

		case EVAL_DEFINED_STATE_IDLE:
			if (et->type == EXPRESSION_TOKEN_TYPE_OPERATOR)
			{
				cparserlinkedlist_t *u = LinkedListPrevious(l);
				cparserlinkedlist_t *v = LinkedListNext(l);
				expression_token_t *etu = LinkedListGetItem(u);
				expression_token_t *etv = LinkedListGetItem(v);

				if ((etu == NULL) || (etv == NULL))
				{
					return EXPRESSION_PREPROCESSOR_RESULT_ERROR_OPERATOR_WITHOUT_OPERANDS_IN_EXPRESSION;
				}

				cparserlinkedlist_t *uu = LinkedListPrevious(u);
				cparserlinkedlist_t *vv = LinkedListNext(u);
				expression_token_t *etuu = LinkedListGetItem(uu);
				expression_token_t *etvv = LinkedListGetItem(vv);

				if ((etuu == NULL) && (etvv == NULL))
				{
					// No more operands in boundary, so compute
					if ((etu->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE) || (etv->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE))
					{
						return EXPRESSION_PREPROCESSOR_RESULT_ERROR_OPERATOR_WITH_INCORRECT_OPERANDS_IN_EXPRESSION;
					}

					int64_t value = ComputeBinary((int64_t)etu->data, et->data, (int64_t)etv->data);
				}
				else if ((etuu != NULL) && (etvv == NULL))
				{

				}
				else if ((etuu == NULL) && (etvv != NULL))
				{

				}
				else
				{

				}
			}
			l = LinkedListNext(l);
			break;

		}
	}

	return EXPRESSION_PREPROCESSOR_RESULT_TRUE;
}

static cparserlinkedlist_t *ExpressionToLinkedList(const uint8_t *expression)
{
	token_t tt = { CPARSER_TOKEN_TYPE_SINGLE_CHAR, true, 1, 0, malloc(strlen(_t expression) + 1) };
	token_source_t source = { &expression, GetNextChar };
	cparserlinkedlist_t *l = NULL;
	expression_token_t *et = NULL;
	bool error = false;

	while (TokenNext(&source, &tt, 0))
	{
		// Skip
		if ((tt.type == CPARSER_TOKEN_TYPE_CPP_COMMENT) || (tt.type == CPARSER_TOKEN_TYPE_C_COMMENT))
			continue;

		if (tt.type == CPARSER_TOKEN_TYPE_IDENTIFIER)
		{
			et = malloc(sizeof(expression_token_t));

			if (StrEq("defined", _t tt.str))
			{
				et->type = EXPRESSION_TOKEN_TYPE_DEFINED;
				et->data = NULL;
			}
			else
			{
				et->type = EXPRESSION_TOKEN_TYPE_IDENTIFIER;
				et->data = strdup(_t tt.str);
			}
		}
		else if ((tt.type == CPARSER_TOKEN_TYPE_SINGLE_CHAR) && ((tt.str[0] == '(') || (tt.str[0] == ')')))
		{
			et = malloc(sizeof(expression_token_t));
			et->type = tt.str[0] == '(' ? EXPRESSION_TOKEN_TYPE_OPEN : EXPRESSION_TOKEN_TYPE_CLOSE;
			et->data = NULL;
		}
		else if ((tt.type == CPARSER_TOKEN_TYPE_OPERATOR) && StringInAscendingSet(tt.str, valid_operators, VALID_OPERATORS_COUNT))
		{
			et = malloc(sizeof(expression_token_t));
			et->type = EXPRESSION_TOKEN_TYPE_OPERATOR;
			et->data = strdup(_t tt.str);
		}
		else
		{
			error = true;
			break;
		}

		// Add expression token to list
		if (l == NULL)
		{
			l = LinkedListNew(et);
		}
		else
		{
			l = LinkedListInsertAfter(l, et);
		}
	}

	// Destroy allocated token buffer
	free(tt.str);

	// On error destroy linked list
	if (error)
		LinkedExpressionListDelete(l);

	return !error ? LinkedListFirst(l) : NULL;
}

cparserexpression_preprocessor_result_t ExpressionEvalPreprocessor(cparserdictionary_t *defines, const uint8_t *expression)
{
	cparserexpression_preprocessor_result_t res = EXPRESSION_PREPROCESSOR_RESULT_FALSE;
	cparserlinkedlist_t *list = ExpressionToLinkedList(expression);

	if (list == NULL)
		return EXPRESSION_PREPROCESSOR_RESULT_ERROR;

	// Print linked expression list
	LinkedExpressionListPrint(list);

	// Compute defined operator and print
	res = LinkedExpressionListComputeDefined(list, defines);
	LinkedExpressionListPrint(list);
	if (res != EXPRESSION_PREPROCESSOR_RESULT_TRUE)
	{
		LinkedExpressionListDelete(list);
		return res;
	}

	// Replace definitions and print
	res = LinkedExpressionListReplaceDefinitions(list, defines);
	LinkedExpressionListPrint(list);
	if (res != EXPRESSION_PREPROCESSOR_RESULT_TRUE)
	{
		LinkedExpressionListDelete(list);
		return res;
	}

	// Compute unary operators and print
	res = LinkedExpressionListComputeUnary(list);
	LinkedExpressionListPrint(list);
	if (res != EXPRESSION_PREPROCESSOR_RESULT_TRUE)
	{
		LinkedExpressionListDelete(list);
		return res;
	}

	// Compute expression evaluator and print
	res = LinkedExpressionListComputeBinary(list);
	LinkedExpressionListPrint(list);
	if ((res != EXPRESSION_PREPROCESSOR_RESULT_TRUE) && (res != EXPRESSION_PREPROCESSOR_RESULT_FALSE))
	{
		LinkedExpressionListDelete(list);
		return res;
	}

	// Free list
	LinkedExpressionListDelete(list);

	__builtin_trap(); // TODO: if literal

	return res;
}


