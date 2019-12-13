/*
 * cparserexpression.h
 *
 *  Created on: 7/12/2019
 *      Author: blue
 */

#ifndef CPARSEREXPRESSION_H_
#define CPARSEREXPRESSION_H_

typedef enum cparserexpression_result_code_e
{
	EXPRESSION_RESULT_CODE_FALSE,
	EXPRESSION_RESULT_CODE_TRUE,
	EXPRESSION_RESULT_CODE_ERROR_INCORRECT_TOKEN,
	EXPRESSION_RESULT_CODE_ERROR_CLOSING_PARENTHESYS_DOES_NOT_MATCH,
	EXPRESSION_RESULT_CODE_ERROR_DEFINED_OPERATOR,
	EXPRESSION_RESULT_CODE_ERROR_DEFINED_EVAL,
	EXPRESSION_RESULT_CODE_ERROR_DEFINED_WITHOUT_IDENTIFIER,
	EXPRESSION_RESULT_CODE_ERROR_MINUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_MINUS,
	EXPRESSION_RESULT_CODE_ERROR_PLUS_OPERATOR_CANNOT_BE_AFTER_ANOTHER_PLUS,
	EXPRESSION_RESULT_CODE_ERROR_INVALID_UNARY_OPERATOR_IN_EXPRESSION
} cparserexpression_result_code_t;

typedef struct cparserexpression_result_e
{
	cparserexpression_result_code_t code;
	uint32_t row;
	uint32_t column;
} cparserexpression_result_t;

cparserexpression_result_t *ExpressionEvalPreprocessor(cparserdictionary_t *defines, const uint8_t *expression, uint32_t row, uint32_t column);


#endif /* CPARSEREXPRESSION_H_ */
