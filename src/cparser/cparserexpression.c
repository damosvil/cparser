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

static cparserexpression_result_t res;

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

static cparserexpression_result_t *LinkedExpressionListComputeDefined(cparserlinkedlist_t *l, cparserdictionary_t *defines)
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
					res.code = EXPRESSION_RESULT_CODE_ERROR_CLOSING_PARENTHESYS_DOES_NOT_MATCH;
					res.row = et->row;
					res.column = et->column;
					return &res;
				}
			}
			else
			{
				// Syntax error: defined operator syntax error
				res.code = EXPRESSION_RESULT_CODE_ERROR_DEFINED_OPERATOR;
				res.row = et->row;
				res.column = et->column;
				return &res;
			}
			break;

		default:
			res.code = EXPRESSION_RESULT_CODE_ERROR_DEFINED_EVAL;
			res.row = et->row;
			res.column = et->column;
			return &res;
			break;

		}
	}

	res.code = EXPRESSION_RESULT_CODE_TRUE;
	res.row = 0;
	res.column = 0;
	return &res;
}

static cparserexpression_result_t *LinkedExpressionListReplaceDefinitions(cparserlinkedlist_t *l, cparserdictionary_t *defines)
{
	res.code = EXPRESSION_RESULT_CODE_TRUE;
	res.row = 0;
	res.column = 0;
	return &res;
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

static cparserexpression_result_t *LinkedExpressionListComputeUnary(cparserlinkedlist_t **list)
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
				res.code = EXPRESSION_RESULT_CODE_ERROR_INVALID_UNARY_OPERATOR_IN_EXPRESSION;
				res.row = et->row;
				res.column = et->column;
				return &res;
			}
			else if ((et_prev_prev != NULL) && StrEq(et_prev_prev->data, "-") && StrEq(et_prev->data, "-"))
			{
				res.code = EXPRESSION_RESULT_CODE_ERROR_MINUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_MINUS;
				res.row = et->row;
				res.column = et->column;
				return &res;
			}
			else if ((et_prev_prev != NULL) && StrEq(et_prev_prev->data, "+") && StrEq(et_prev->data, "+"))
			{
				res.code = EXPRESSION_RESULT_CODE_ERROR_PLUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_PLUS;
				res.row = et->row;
				res.column = et->column;
				return &res;
			}
			else
			{
				int64_t value = ComputeUnary(et_prev->data, (int64_t)et->data);
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
				LinkedExpressionListDelete(prev);
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

	res.code = EXPRESSION_RESULT_CODE_TRUE;
	res.row = 0;
	res.column = 0;
	return &res;
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
	else if ((op[0] == '!') && (op[1] == '=') && (op[2] == 0))
	{
		return a == b;
	}
	else
	{
		return 0;
	}
}

static cparserexpression_result_t *LinkedExpressionListComputeBinary(cparserlinkedlist_t **list)
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
				res.code = EXPRESSION_RESULT_CODE_ERROR_INVERTED_PARENTHESIS_NEAR_OPERAND;
				res.row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res.column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return &res;
			}
			else if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE)) ||
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_DECODED_VALUE))
				)
			{
				// Illegal operand besides operand
				res.code = EXPRESSION_RESULT_CODE_ERROR_OPERAND_BESIDES_OPERAND;
				res.row = et->row;
				res.column = et->column;
				return &res;
			}
			else if (
					((et_prev == NULL) && (et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_CLOSE)) ||
					((et_next == NULL) && (et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_OPEN))
				)
			{
				// Illegal operand with partial parenthesis
				res.code = EXPRESSION_RESULT_CODE_ERROR_INCORRECT_PARENTHESIS;
				res.row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res.column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return &res;
			}
			else if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_OPEN)) ||
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_CLOSE))
				)
			{
				// Operand is alone in between parenthesis, so remove them
				ExpressionTokenDelete(et_next);
				ExpressionTokenDelete(et_prev);
				LinkedExpressionListDelete(next);
				LinkedExpressionListDelete(prev);
			}
		}
		else if (et->type == EXPRESSION_TOKEN_TYPE_OPERATOR)
		{
			if ((et_prev == NULL) || (et_next == NULL))
			{
				// Operator without operand
				res.code = EXPRESSION_RESULT_CODE_ERROR_OPERATOR_WITH_NO_OPERANDS;
				res.row = et->row;
				res.column = et->column;
				return &res;
			}
			else if (
					((et_prev != NULL) && (et_prev->type == EXPRESSION_TOKEN_TYPE_OPEN)) ||
					((et_next != NULL) && (et_next->type == EXPRESSION_TOKEN_TYPE_CLOSE))
				)
			{
				// Operator with incorrect parenthesys around (notify parenthesis node)
				res.code = EXPRESSION_RESULT_CODE_ERROR_INCORRECT_PARENTHESIS;
				res.row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res.column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return &res;
			}
			else if (
					((et_prev != NULL) && (et_prev->type != EXPRESSION_TOKEN_TYPE_CLOSE) && (et_prev->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE)) ||
					((et_next != NULL) && (et_next->type != EXPRESSION_TOKEN_TYPE_OPEN) && (et_next->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE))
				)
			{
				// Operator lacks of valid operands or parenthesis arround
				res.code = EXPRESSION_RESULT_CODE_ERROR_OPERATOR_WITH_INVALID_NEIGHBOURS;
				res.row = (et_prev != NULL) ? et_prev->row : et_next->row;
				res.column = (et_prev != NULL) ? et_prev->column : et_next->column;
				return &res;
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
						// There is a operator next so check operator precedence
						(
							(et_next_next != NULL) &&
							(et_next_next->type == EXPRESSION_TOKEN_TYPE_OPERATOR) &&
							OperatorPreference(et->data, et_next_next->data) >= 0
						)
					)
				{
					// Perform operation
					int64_t value = ComputeBinary((int64_t)et_prev->data, et->data, (int64_t)et_next->data);
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
			else
			{
				// Corner case
				__builtin_trap(); // TODO: corner case in compute binary
			}
		}

		l = LinkedListNext(l);
	}

	// Go to first token in list
	*list = LinkedListFirst(*list);

	res.code = EXPRESSION_RESULT_CODE_TRUE;
	res.row = 0;
	res.column = 0;
	return &res;
}

static cparserexpression_result_t *ExpressionToLinkedList(const uint8_t *expression, uint32_t row, uint32_t column, cparserlinkedlist_t **list)
{
	token_t *tt = TokenNew();
	token_source_t source = { &expression, GetNextChar };
	cparserlinkedlist_t *l = NULL;
	expression_token_t *et = NULL;

	while (TokenNext(tt, &source, 0))
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
		else
		{
			res.code = EXPRESSION_RESULT_CODE_ERROR_INCORRECT_TOKEN;
			res.row = tt->row;
			res.column = tt->column;
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

	res.code = EXPRESSION_RESULT_CODE_TRUE;
	res.row = 0;
	res.column = 0;
	return &res;
}

cparserexpression_result_t *ExpressionEvalPreprocessor(cparserdictionary_t *defines, const uint8_t *expression, uint32_t row, uint32_t column)
{
	cparserlinkedlist_t *list;
	cparserexpression_result_t *r = ExpressionToLinkedList(expression, row, column, &list);
	LinkedExpressionListPrint(list);
	if (r->code != EXPRESSION_RESULT_CODE_TRUE)
	{
		LinkedExpressionListDelete(list);
		return r;
	}

	// Compute defined operator and print
	r = LinkedExpressionListComputeDefined(list, defines);
	LinkedExpressionListPrint(list);
	if (r->code != EXPRESSION_RESULT_CODE_TRUE)
	{
		LinkedExpressionListDelete(list);
		return r;
	}

	// Replace definitions and print
	r = LinkedExpressionListReplaceDefinitions(list, defines);
	LinkedExpressionListPrint(list);
	if (r->code != EXPRESSION_RESULT_CODE_TRUE)
	{
		LinkedExpressionListDelete(list);
		return r;
	}

	// Compute unary operators and print
	r = LinkedExpressionListComputeUnary(&list);
	LinkedExpressionListPrint(list);
	if (r->code != EXPRESSION_RESULT_CODE_TRUE)
	{
		LinkedExpressionListDelete(list);
		return r;
	}

	// Compute expression evaluator and print
	while (LinkedListNext(list) != NULL)
	{
		r = LinkedExpressionListComputeBinary(&list);
		LinkedExpressionListPrint(list);
		if ((r->code != EXPRESSION_RESULT_CODE_TRUE) && (r->code != EXPRESSION_RESULT_CODE_FALSE))
		{
			LinkedExpressionListDelete(list);
			return r;
		}
	}

	// Take last expression token
	expression_token_t *et = LinkedListGetItem(list);

	// Check expression token is decoded value
	if (et->type != EXPRESSION_TOKEN_TYPE_DECODED_VALUE)
	{
		// Error: last expression token type shall be decoded value
		res.code = EXPRESSION_RESULT_CODE_ERROR_LAST_EXPRESSION_TOKEN_SHALL_BE_A_DECODED_VALUE;
	}
	else
	{
		// Set code depending on token value
		res.code = (0 != (int64_t)et->data) ? EXPRESSION_RESULT_CODE_TRUE : EXPRESSION_RESULT_CODE_FALSE;
	}
	res.row = row;
	res.column = column;

	// Free list
	LinkedExpressionListDelete(list);

	return &res;
}


