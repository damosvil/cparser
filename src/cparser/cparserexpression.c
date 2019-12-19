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
#include "cparserobject.h"
#include "cparserexpression.h"


#define VALID_OPERATORS_COUNT			20
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
	uint32_t row;
	uint32_t column;
	void *data;
} expression_token_t;


static const uint8_t *valid_operators[VALID_OPERATORS_COUNT] = {
		_T "!", _T "!=", _T "%",  _T "&",  _T "&&",
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
			printf("defined ");
			break;

		case EXPRESSION_TOKEN_TYPE_IDENTIFIER:
			printf("%s ", (char *)et->data);
			break;

		case EXPRESSION_TOKEN_TYPE_OPERATOR:
			printf("%s ", (char *)et->data);
			break;

		case EXPRESSION_TOKEN_TYPE_OPEN:
			printf("( ");
			break;

		case EXPRESSION_TOKEN_TYPE_CLOSE:
			printf(") ");
			break;

		case EXPRESSION_TOKEN_TYPE_DECODED_VALUE:
			printf("%ld ", (intptr_t)et->data);
			break;

		default:
			break;

		}

		l = LinkedListNext(l);
	}

	printf("\n");
}

static void LinkedExpressionListComputeDefined(cparserlinkedlist_t *l, cparserdictionary_t *defines, cparserexpression_result_t *res)
{
	enum eval_defined_state_e
	{
		EVAL_DEFINED_STATE_IDLE = 0,
		EVAL_DEFINED_STATE_LOOKING
	} state = EVAL_DEFINED_STATE_IDLE;

	expression_token_t *et = NULL;
	uint32_t level = 0;

	// Process tokens
	while (l != NULL)
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
				// Store row and column identifier
				uint32_t row = et->row;
				uint32_t column = et->column;

				// Replace previous define and definition identifier by a boolean decoded value
				intptr_t value = DictionaryExistsKey(defines, et->data) ? 1 : 0;
				cparserlinkedlist_t *defined = LinkedListPrevious(l);

				// Remove identifier
				ExpressionTokenDelete(et);
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

					// Set decoded value in former defined node
					et = malloc(sizeof(expression_token_type_t));
					et->type = EXPRESSION_TOKEN_TYPE_DECODED_VALUE;
					et->row = row;
					et->column = column;
					et->data = (void *)value;
					LinkedListUpdateItem(defined, et);

					// Select next linked list token to defined
					l = LinkedListNext(defined);

					// Return to idle state
					state = EVAL_DEFINED_STATE_IDLE;
				}
				else
				{
					// Syntax error: incorrect number of closing parenthesis
					res->code = EXPRESSION_RESULT_ERROR_CLOSING_PARENTHESYS_DOES_NOT_MATCH;
					res->value = 0;
					res->row = et->row;
					res->column = et->column;
					return;
				}
			}
			else
			{
				// Syntax error: defined operator syntax error
				res->code = EXPRESSION_RESULT_ERROR_DEFINED_OPERATOR;
				res->value = 0;
				res->row = et->row;
				res->column = et->column;
				return;
			}
			break;

		default:
			res->code = EXPRESSION_RESULT_ERROR_DEFINED_EVAL;
			res->value = 0;
			res->row = et->row;
			res->column = et->column;
			return;
			break;

		}
	}

	res->code = EXPRESSION_RESULT_SUCCESS;
	res->value = 0;
	res->row = 0;
	res->column = 0;
}

static void LinkedExpressionListReplaceDefinitions(cparserlinkedlist_t *l, cparserdictionary_t *defines, cparserexpression_result_t *res)
{
	// Process tokens
	while (l != NULL)
	{
		// Get item from linked list node
		expression_token_t * et = LinkedListGetItem(l);

		if (et->type == EXPRESSION_TOKEN_TYPE_IDENTIFIER)
		{
			object_t *oo = DictionaryGetKeyValue(defines, et->data);
			intptr_t value = 0;
			uint32_t row = et->row;
			uint32_t column = et->column;

			// Try to find the preprocessor expression associated to the preprocessor identifier
			if (oo != NULL)
			{
				if (oo->type == OBJECT_TYPE_PREPROCESSOR_EXPRESSION)
				{
					// Recursively evaluate the expression
					ExpressionEvalPreprocessor(defines, oo->data, row, column, res);

					// Check expression result
					if (!res->code == EXPRESSION_RESULT_SUCCESS)
						return;

					// Assign expression result value
					value = res->value;
				}
				else if (oo->type == OBJECT_TYPE_PREPROCESSOR_IDENTIFIER)
				{
					// In this case oo should have a preprocessor expression sibbling
					__builtin_trap(); // TODO: implement looking for preprocessor expression sibbling
				}
				else
				{
					// No valid preprocessor expression can be found in dictionary
					res->code = EXPRESSION_RESULT_ERROR_NO_VALID_PREPROCESSOR_EXPRESSION_FOUND;
					res->value = 0;
					res->row = et->row;
					res->column = et->column;
					return;
				}
			}

			// Replace current identifier expression token by a decoded value one
			ExpressionTokenDelete(et);
			et = malloc(sizeof(expression_token_t));
			et->type = EXPRESSION_TOKEN_TYPE_DECODED_VALUE;
			et->row = row;
			et->column = column;
			et->data = (void *)value;
			LinkedListUpdateItem(l, et);
		}

		// Get next linked list item
		l = LinkedListNext(l);
	}

	res->code = EXPRESSION_RESULT_SUCCESS;
	res->value = 0;
	res->row = 0;
	res->column = 0;
	return;
}

static intptr_t ComputeUnary(const uint8_t *op, intptr_t a)
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

static void LinkedExpressionListComputeUnary(cparserlinkedlist_t **list, cparserexpression_result_t *res)
{
	cparserlinkedlist_t *l = *list;

	// Process tokens
	while (l != NULL)
	{
		// Update list with a valid node
		*list = l;

		// Get item from linked list node
		expression_token_t * et = LinkedListGetItem(l);
		cparserlinkedlist_t *prev = LinkedListPrevious(l);
		cparserlinkedlist_t *prev_prev = LinkedListPrevious(prev);
		expression_token_t *et_prev = LinkedListGetItem(prev);
		expression_token_t *et_prev_prev = LinkedListGetItem(prev_prev);

		if (
				// Value preceded by operator at the beginning of the expression
				(
					(et_prev != NULL) && (et_prev_prev == NULL) &&
					(et->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE) &&
					(et_prev->type == EXPRESSION_TOKEN_TYPE_OPERATOR)
				)
				||
				// Value preceded by operator also preceded by operator
				(
					(et_prev != NULL) && (et_prev_prev != NULL) &&
					(et->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE) &&
					(et_prev->type == EXPRESSION_TOKEN_TYPE_OPERATOR) &&
					(et_prev_prev->type == EXPRESSION_TOKEN_TYPE_OPERATOR)
				)
				||
				// Value preceded by operator also preceded by open parenthesis
				(
					(et_prev != NULL) && (et_prev_prev != NULL) &&
					(et->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE) &&
					(et_prev->type == EXPRESSION_TOKEN_TYPE_OPERATOR) &&
					(et_prev_prev->type == EXPRESSION_TOKEN_TYPE_OPEN)
				)
			)
		{
			// Unary operator before decoded value
			if (!StringInAscendingSet(et_prev->data, valid_unary_operators, VALID_UNARY_OPERATORS_COUNT))
			{
				res->code = EXPRESSION_RESULT_ERROR_INVALID_UNARY_OPERATOR_IN_EXPRESSION;
				res->value = 0;
				res->row = et->row;
				res->column = et->column;
				return;
			}
			else if (
					(et_prev_prev != NULL) && (et_prev_prev->type == EXPRESSION_TOKEN_TYPE_OPERATOR) &&
					StrEq(et_prev_prev->data, "-") && StrEq(et_prev->data, "-")
				)
			{
				res->code = EXPRESSION_RESULT_ERROR_MINUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_MINUS;
				res->value = 0;
				res->row = et->row;
				res->column = et->column;
				return;
			}
			else if (
					(et_prev_prev != NULL) && (et_prev_prev->type == EXPRESSION_TOKEN_TYPE_OPERATOR) &&
					StrEq(et_prev_prev->data, "+") && StrEq(et_prev->data, "+")
				)
			{
				res->code = EXPRESSION_RESULT_ERROR_PLUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_PLUS;
				res->value = 0;
				res->row = et->row;
				res->column = et->column;
				return;
			}
			else
			{
				intptr_t value = ComputeUnary(et_prev->data, (intptr_t)et->data);
				uint32_t row = et->row;
				uint32_t column = et->column;

				// Update current operand to value
				ExpressionTokenDelete(et);
				et = malloc(sizeof(expression_token_t));
				et->type = EXPRESSION_TOKEN_TYPE_DECODED_VALUE;
				et->row = row;
				et->column = column;
				et->data = (void *)value;
				LinkedListUpdateItem(l, et);

				// Remove previous operator
				ExpressionTokenDelete(et_prev);
				LinkedListDelete(prev);
			}
		}
		else
		{
			// Select next token
			l = LinkedListNext(l);
		}
	}

	// Go to first token in list
	*list = LinkedListFirst(*list);

	res->code = EXPRESSION_RESULT_SUCCESS;
	res->value = 0;
	res->row = 0;
	res->column = 0;
}

static int OperatorPreference(const char *a, const char *b)
{
	if (StrEq(a, "*") || StrEq(a, "/") || StrEq(a, "%"))
	{
		return 1;
	}
	else if (StrEq(a, "+") || StrEq(a, "-"))
	{
		if (StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%"))
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "<<") || StrEq(a, ">>"))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "<") || StrEq(a, "<=") || StrEq(a, ">") || StrEq(a, ">="))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-") ||
				StrEq(b, "<<") || StrEq(b, ">>")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "==") || StrEq(a, "!="))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-") ||
				StrEq(b, "<<") || StrEq(b, ">>") ||
				StrEq(b, "<") || StrEq(b, "<=") || StrEq(b, ">") || StrEq(b, ">=")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "&"))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-") ||
				StrEq(b, "<<") || StrEq(b, ">>") ||
				StrEq(b, "<") || StrEq(b, "<=") || StrEq(b, ">") || StrEq(b, ">=") ||
				StrEq(b, "==") || StrEq(b, "!=")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "^"))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-") ||
				StrEq(b, "<<") || StrEq(b, ">>") ||
				StrEq(b, "<") || StrEq(b, "<=") || StrEq(b, ">") || StrEq(b, ">=") ||
				StrEq(b, "==") || StrEq(b, "!=") ||
				StrEq(b, "&")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "|"))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-") ||
				StrEq(b, "<<") || StrEq(b, ">>") ||
				StrEq(b, "<") || StrEq(b, "<=") || StrEq(b, ">") || StrEq(b, ">=") ||
				StrEq(b, "==") || StrEq(b, "!=") ||
				StrEq(b, "&") ||
				StrEq(b, "^")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "&&"))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-") ||
				StrEq(b, "<<") || StrEq(b, ">>") ||
				StrEq(b, "<") || StrEq(b, "<=") || StrEq(b, ">") || StrEq(b, ">=") ||
				StrEq(b, "==") || StrEq(b, "!=") ||
				StrEq(b, "&") ||
				StrEq(b, "^") ||
				StrEq(b, "|")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if (StrEq(a, "||"))
	{
		if (
				StrEq(b, "*") || StrEq(b, "/") || StrEq(b, "%") ||
				StrEq(b, "+") || StrEq(b, "-") ||
				StrEq(b, "<<") || StrEq(b, ">>") ||
				StrEq(b, "<") || StrEq(b, "<=") || StrEq(b, ">") || StrEq(b, ">=") ||
				StrEq(b, "==") || StrEq(b, "!=") ||
				StrEq(b, "&") ||
				StrEq(b, "^") ||
				StrEq(b, "|") ||
				StrEq(b, "&&")
			)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}

	return 0;
}

static intptr_t ComputeBinary(intptr_t a, const uint8_t *op, intptr_t b)
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
	else if ((op[0] == '!') && (op[1] == '=') && (op[2] == 0))
	{
		return a == b;
	}
	else
	{
		return 0;
	}
}

static void LinkedExpressionListComputeBinary(cparserlinkedlist_t **list, cparserexpression_result_t *res)
{
	cparserlinkedlist_t *l = *list;

	// Process tokens
	while (l != NULL)
	{
		// Update list
		*list = l;

		// Get items from linked list node
		expression_token_t *et = LinkedListGetItem(l);

		cparserlinkedlist_t *prev = LinkedListPrevious(l);
		cparserlinkedlist_t *next = LinkedListNext(l);
		expression_token_t *et_prev = LinkedListGetItem(prev);
		expression_token_t *et_next = LinkedListGetItem(next);

		cparserlinkedlist_t *next_next = LinkedListNext(next);
		expression_token_t *et_next_next = LinkedListGetItem(next_next);

		if (et->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE)
		{
			if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_CLOSE)) ||
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_OPEN))
				)
			{
				// Inverted parenthesis near operand
				res->code = EXPRESSION_RESULT_ERROR_INVERTED_PARENTHESIS_NEAR_OPERAND;
				res->value = 0;
				res->row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res->column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return;
			}
			else if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE)) ||
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE))
				)
			{
				// Illegal operand besides operand
				res->code = EXPRESSION_RESULT_ERROR_OPERAND_BESIDES_OPERAND;
				res->value = 0;
				res->row = et->row;
				res->column = et->column;
				return;
			}
			else if (
					((et_prev == NULL) && (et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_CLOSE)) ||
					((et_next == NULL) && (et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_OPEN))
				)
			{
				// Illegal operand with partial parenthesis
				res->code = EXPRESSION_RESULT_ERROR_INCORRECT_PARENTHESIS;
				res->value = 0;
				res->row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res->column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return;
			}
			else if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_OPEN)) &&
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_CLOSE))
				)
			{
				// Operand is alone in between parenthesis, so remove them
				ExpressionTokenDelete(et_next);
				ExpressionTokenDelete(et_prev);
				LinkedListDelete(next);
				LinkedListDelete(prev);
			}
		}
		else if (et->type == EXPRESSION_TOKEN_TYPE_OPERATOR)
		{
			if ((et_prev == NULL) || (et_next == NULL))
			{
				// Operator without operand
				res->code = EXPRESSION_RESULT_ERROR_OPERATOR_WITH_NO_OPERANDS;
				res->value = 0;
				res->row = et->row;
				res->column = et->column;
				return;
			}
			else if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_OPEN)) ||
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_CLOSE))
				)
			{
				// Operator with incorrect parenthesys around (notify parenthesis node)
				res->code = EXPRESSION_RESULT_ERROR_INCORRECT_PARENTHESIS;
				res->value = 0;
				res->row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res->column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return;
			}
			else if (
					((et_prev != NULL) && (et_prev->type != EXPRESSION_TOKEN_TYPE_CLOSE) && (et_prev->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE)) ||
					((et_next != NULL) && (et_next->type != EXPRESSION_TOKEN_TYPE_OPEN) && (et_next->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE))
				)
			{
				// Operator lacks of valid operands or parenthesis arround
				res->code = EXPRESSION_RESULT_ERROR_OPERATOR_WITH_INVALID_NEIGHBOURS;
				res->value = 0;
				res->row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res->column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return;
			}
			else if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE)) &&
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE))
				)
			{
				if (
						// No next operator so no precedence checking
						(et_next_next == NULL)
						||
						// Close parenthesis so no precedence checking
						(
							(et_next_next != NULL) &&
							(et_next_next->type == EXPRESSION_TOKEN_TYPE_CLOSE))
						||
						// There is a operator next so check operator precedence
						(
							(et_next_next != NULL) &&
							(et_next_next->type == EXPRESSION_TOKEN_TYPE_OPERATOR) &&
							OperatorPreference(et->data, et_next_next->data) >= 0
						)
					)
				{
					// Perform operation
					intptr_t value = ComputeBinary((intptr_t)et_prev->data, et->data, (intptr_t)et_next->data);
					uint32_t row = et->row;
					uint32_t column = et->column;

					// Replace current by value
					ExpressionTokenDelete(et);
					et = malloc(sizeof(expression_token_t));
					et->type = EXPRESSION_TOKEN_TYPE_DECODED_VALUE;
					et->row = row;
					et->column = column;
					et->data = (void *)value;
					LinkedListUpdateItem(l, et);

					// Delete prev and next
					ExpressionTokenDelete(et_prev);
					ExpressionTokenDelete(et_next);
					LinkedListDelete(prev);
					LinkedListDelete(next);
				}
			}
		}

		l = LinkedListNext(l);
	}

	// Go to first token in list
	*list = LinkedListFirst(*list);

	res->code = EXPRESSION_RESULT_SUCCESS;
	res->value = 0;
	res->row = 0;
	res->column = 0;
}

static void ExpressionToLinkedList(const uint8_t *expression, uint32_t row, uint32_t column, cparserlinkedlist_t **list, cparserexpression_result_t *res)
{
	token_t *tt = TokenNew();
	token_source_t source;
	cparserlinkedlist_t *l = NULL;
	expression_token_t *et = NULL;

	// Initialize result
	res->code = EXPRESSION_RESULT_SUCCESS;
	res->value = 0;
	res->row = 0;
	res->column = 0;

	// Initialize token source
	TokenSourceInit(&source, &expression, GetNextChar);

	// Parse tokens into linked list
	while ((res->code == EXPRESSION_RESULT_SUCCESS) && TokenNext(tt, &source, 0))
	{
		// Skip
		if ((tt->type == CPARSER_TOKEN_TYPE_CPP_COMMENT) || (tt->type == CPARSER_TOKEN_TYPE_C_COMMENT))
			continue;

		if (tt->type == CPARSER_TOKEN_TYPE_IDENTIFIER)
		{
			et = malloc(sizeof(expression_token_t));

			if (StrEq("defined", _t tt->str))
			{
				et->type = EXPRESSION_TOKEN_TYPE_DEFINED;
				et->row = tt->row;
				et->column = tt->column;
				et->data = NULL;
			}
			else
			{
				et->type = EXPRESSION_TOKEN_TYPE_IDENTIFIER;
				et->row = tt->row;
				et->column = tt->column;
				et->data = strdup(_t tt->str);
			}
		}
		else if ((tt->type == CPARSER_TOKEN_TYPE_SINGLE_CHAR) && ((tt->str[0] == '(') || (tt->str[0] == ')')))
		{
			et = malloc(sizeof(expression_token_t));
			et->type = tt->str[0] == '(' ? EXPRESSION_TOKEN_TYPE_OPEN : EXPRESSION_TOKEN_TYPE_CLOSE;
			et->row = tt->row;
			et->column = tt->column;
			et->data = NULL;
		}
		else if ((tt->type == CPARSER_TOKEN_TYPE_OPERATOR) && StringInAscendingSet(tt->str, valid_operators, VALID_OPERATORS_COUNT))
		{
			et = malloc(sizeof(expression_token_t));
			et->type = EXPRESSION_TOKEN_TYPE_OPERATOR;
			et->row = tt->row;
			et->column = tt->column;
			et->data = strdup(_t tt->str);
		}
		else if (tt->type == CPARSER_TOKEN_TYPE_NUMBER_LITERAL)
		{
			et = malloc(sizeof(expression_token_t));
			et->type = EXPRESSION_TOKEN_TYPE_DECODED_VALUE;
			et->row = tt->row;
			et->column = tt->column;
			et->data = (void *)atoll(_t tt->str);
		}
		else if (tt->type == CPARSER_TOKEN_TYPE_BACKSLASH)
		{
			/* Do not add backslash */
			continue;
		}
		else
		{
			res->code = EXPRESSION_RESULT_ERROR_INCORRECT_TOKEN;
			res->value = 0;
			res->row = tt->row;
			res->column = tt->column;
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
	TokenDelete(tt);

	// Assign the decoded list to l
	*list = LinkedListFirst(l);
}

void ExpressionEvalPreprocessor(cparserdictionary_t *defines, const uint8_t *expression, uint32_t row, uint32_t column, cparserexpression_result_t *res)
{
	cparserlinkedlist_t *list;

	ExpressionToLinkedList(expression, row, column, &list, res);
	LinkedExpressionListPrint(list);
	if (res->code != EXPRESSION_RESULT_SUCCESS)
	{
		LinkedExpressionListDelete(list);
		return;
	}

	// Compute defined operator and print
	LinkedExpressionListComputeDefined(list, defines, res);
	LinkedExpressionListPrint(list);
	if (res->code != EXPRESSION_RESULT_SUCCESS)
	{
		LinkedExpressionListDelete(list);
		return;
	}

	// Replace definitions and print
	LinkedExpressionListReplaceDefinitions(list, defines, res);
	LinkedExpressionListPrint(list);
	if (res->code != EXPRESSION_RESULT_SUCCESS)
	{
		LinkedExpressionListDelete(list);
		return;
	}

	// Compute unary operators and print
	LinkedExpressionListComputeUnary(&list, res);
	LinkedExpressionListPrint(list);
	if (res->code != EXPRESSION_RESULT_SUCCESS)
	{
		LinkedExpressionListDelete(list);
		return;
	}

	// Compute expression evaluator and print
	while (LinkedListNext(list) != NULL)
	{
		LinkedExpressionListComputeBinary(&list, res);
		LinkedExpressionListPrint(list);
		if (res->code != EXPRESSION_RESULT_SUCCESS)
		{
			LinkedExpressionListDelete(list);
			return;
		}
	}

	// Take last expression token
	expression_token_t *et = LinkedListGetItem(list);

	// Check expression token is decoded value
	if (et->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE)
	{
		// Error: last expression token type shall be decoded value
		res->code = EXPRESSION_RESULT_ERROR_LAST_EXPRESSION_TOKEN_SHALL_BE_A_DECODED_VALUE;
		res->value = 0;
	}
	else
	{
		// Set code depending on token value
		res->code = EXPRESSION_RESULT_SUCCESS;
		res->value = (intptr_t)et->data;
	}
	res->row = row;
	res->column = column;

	// Free list
	LinkedExpressionListDelete(list);
}


