/*
 * cparserexpression.h
 *
 *  Created on: 7/12/2019
 *      Author: blue
 */

#ifndef CPARSEREXPRESSION_H_
#define CPARSEREXPRESSION_H_

typedef enum cparserexpression_preprocessor_result_e
{
	EXPRESSION_PREPROCESSOR_RESULT_FALSE,
	EXPRESSION_PREPROCESSOR_RESULT_TRUE,
	EXPRESSION_PREPROCESSOR_RESULT_ERROR
} cparserexpression_preprocessor_result_t;

cparserexpression_preprocessor_result_t ExpressionEvalPreprocessor(cparserdictionary_t *defines, const uint8_t *expression);


#endif /* CPARSEREXPRESSION_H_ */
