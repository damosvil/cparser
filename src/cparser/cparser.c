/*
 * cparser.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "cparserpaths.h"
#include "cparsertools.h"
#include "cparsertoken.h"
#include "cparserobject.h"
#include "cparserdictionary.h"
#include "cparser.h"


#define DATATYPE_DEFINED_FLAGS  	(\
									EFLAGS_MODIFIER_SIGNED 		       	|\
									EFLAGS_MODIFIER_UNSIGNED 	      	|\
									EFLAGS_MODIFIER_SHORT 		       	|\
									EFLAGS_MODIFIER_LONG 		       	|\
									EFLAGS_MODIFIER_LONG_LONG 	       	|\
									EFLAGS_VOID 				       	|\
									EFLAGS_CHAR 				       	|\
									EFLAGS_INT 					       	|\
									EFLAGS_FLOAT 				       	|\
									EFLAGS_DOUBLE 				       	|\
									EFLAGS_USER_DEFINED_DATATYPE		 \
									)




typedef enum states_e
{
	STATE_IDLE,
	STATE_PREPROCESSOR,
	STATE_INCLUDE_FILENAME,
	STATE_DEFINE_IDENTIFIER,
	STATE_DEFINE_LITERAL,
	STATE_DATATYPE,
	STATE_PRAGMA,
	STATE_IDENTIFIER,
	STATE_ARRAY_DEFINITION,
	STATE_INITIALIZATION,
	STATE_FUNCTION_PARAMETERS,
	STATE_FUNCTION_DECLARED,
	STATE_ERROR
} states_t;

// Parsing state
typedef struct state_s
{
	FILE *file;
	states_t state;
	cparserdictionary_t *dictionary;
	const cparserpaths_t *paths;
	uint32_t tokenizer_flags;
	token_t token;
} state_t;

enum eflags_e
{
	EFLAGS_NONE 				    = 0,
	EFLAGS_SPECIFIER 			    = 1 << 1,
	EFLAGS_QUALIFIER 			    = 1 << 2,
	EFLAGS_MODIFIER_SIGNED 		    = 1 << 3,
	EFLAGS_MODIFIER_UNSIGNED 	    = 1 << 4,
	EFLAGS_MODIFIER_SHORT 		    = 1 << 5,
	EFLAGS_MODIFIER_LONG 		    = 1 << 6,
	EFLAGS_MODIFIER_LONG_LONG 	    = 1 << 7,
	EFLAGS_VOID 				    = 1 << 8,
	EFLAGS_CHAR 				    = 1 << 9,
	EFLAGS_INT 					    = 1 << 10,
	EFLAGS_FLOAT 				    = 1 << 11,
	EFLAGS_DOUBLE 				    = 1 << 12,
	EFLAGS_USER_DEFINED_DATATYPE	= 1 << 13,
	EFLAGS_COMPOSED_DATATYPE		= 1 << 14
};


static object_t * DigestDataType(object_t *oo, state_t *s)
{
	static uint32_t eflags = EFLAGS_NONE;

	// Initialize acceptance flags
	if (oo->children_count == 0)
	{
		eflags = EFLAGS_NONE;
	}

	// Compose datatype
	if (StrEq(_t s->token.str, "static") || StrEq(_t s->token.str, "extern") || StrEq(_t s->token.str, "auto") ||
			StrEq(_t s->token.str, "register") || StrEq(_t s->token.str, "typedef"))
	{
		// Check specifiers
		if (eflags & EFLAGS_SPECIFIER)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Specifier already defined");
		}
		else
		{
			eflags |= EFLAGS_SPECIFIER;
			oo = ObjectAddChild(oo, OBJECT_TYPE_SPECIFIER, &s->token);
		}
	}
	else if (StrEq(_t s->token.str, "const") || StrEq(_t s->token.str, "volatile"))
	{
		// Check qualifiers
		if (eflags & EFLAGS_QUALIFIER)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Qualifier already defined");
		}
		else
		{
			eflags |= EFLAGS_QUALIFIER;
			oo = ObjectAddChild(oo, OBJECT_TYPE_QUALIFIER, &s->token);
		}
	}
	else if (StrEq(_t s->token.str, "signed") || StrEq(_t s->token.str, "unsigned") || StrEq(_t s->token.str, "short") || StrEq(_t s->token.str, "long"))
	{
		// Check modifiers
		uint32_t newf = 0;
		if (s->token.str[2] == 'g')
			newf = EFLAGS_MODIFIER_SIGNED;
		else if (s->token.str[2] == 's')
			newf = EFLAGS_MODIFIER_UNSIGNED;
		else if (s->token.str[2] == 'o')
			newf = EFLAGS_MODIFIER_SHORT;
		else
			newf = EFLAGS_MODIFIER_LONG;

		if (eflags & newf)
		{
			// long long int is allowed
			if ((eflags & EFLAGS_MODIFIER_LONG) && !(eflags & (EFLAGS_MODIFIER_LONG_LONG | EFLAGS_DOUBLE)))
			{
				eflags |= newf;
				oo = ObjectAddChild(oo, OBJECT_TYPE_MODIFIER, &s->token);
			}
			else
			{
				oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
				oo->info = _T strdup("Cannot apply the same modifier twice");
			}
		}
		else if (eflags & EFLAGS_USER_DEFINED_DATATYPE)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers to user defined datatypes");
		}
		else if ((eflags & EFLAGS_VOID) && newf)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers to void datatype");
		}
		else if ((eflags & EFLAGS_CHAR) && (newf & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifier short nor long to char datatype");
		}
		else if ((eflags & EFLAGS_FLOAT) && newf)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers to float datatype");
		}
		else if ((eflags & EFLAGS_DOUBLE) && (newf & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SIGNED)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers short, unsigned not signed to double datatype");
		}
		else if (((eflags & EFLAGS_MODIFIER_UNSIGNED) && (newf & EFLAGS_MODIFIER_SIGNED)) ||
				((eflags & EFLAGS_MODIFIER_SIGNED) && (newf & EFLAGS_MODIFIER_UNSIGNED)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply signed and unsigned modifiers at the same time");
		}
		else if (((eflags & EFLAGS_MODIFIER_LONG) && (newf & EFLAGS_MODIFIER_SHORT)) ||
				((eflags & EFLAGS_MODIFIER_SHORT) && (newf & EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply long and short modifiers at the same time");
		}
		else
		{
			eflags |= newf;
			oo = ObjectAddChild(oo, OBJECT_TYPE_MODIFIER, &s->token);
		}
	}
	else if (StrEq(_t s->token.str, "void") || StrEq(_t s->token.str, "char") || StrEq(_t s->token.str, "int") || StrEq(_t s->token.str, "float") || StrEq(_t s->token.str, "double"))
	{
		// Check basic built in datatype
		uint32_t newf = 0;
		if (s->token.str[0] == 'v')
			newf = EFLAGS_VOID;
		else if (s->token.str[0] == 'c')
			newf = EFLAGS_CHAR;
		else if (s->token.str[0] == 'i')
			newf = EFLAGS_INT;
		else if (s->token.str[0] == 'f')
			newf = EFLAGS_FLOAT;
		else
			newf = EFLAGS_DOUBLE;

		if (eflags & newf)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify the same basic built in datatype twice");
		}
		else if (eflags & EFLAGS_USER_DEFINED_DATATYPE)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify a basic built in datatype when it is already defined a used defined datatype");
		}
		else if ((newf & EFLAGS_VOID) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify modifiers to void datatype");
		}
		else if ((newf & EFLAGS_CHAR) && (eflags & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify short nor long modifiers to char datatype");
		}
		else if ((newf & EFLAGS_FLOAT) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify modifiers to float datatype");
		}
		else if ((newf & EFLAGS_DOUBLE) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify signed, unsigned nor short modifiers to double datatype");
		}
		else
		{
			eflags |= newf;
			oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_PRIMITIVE, &s->token);
		}
	}
	else if (StrEq(_t s->token.str, "union") || StrEq(_t s->token.str, "enum") || StrEq(_t s->token.str, "struct"))
	{
		// Possible datatype definition, variable definition, function definition
		eflags |= ~EFLAGS_COMPOSED_DATATYPE;

		// Add child
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_USER_DEFINED, &s->token);
		if (s->token.str[0] == 'u')
			oo = ObjectAddChild(oo, OBJECT_TYPE_UNION, &s->token);
		else if (s->token.str[0] == 'e')
			oo = ObjectAddChild(oo, OBJECT_TYPE_ENUM, &s->token);
		else
			oo = ObjectAddChild(oo, OBJECT_TYPE_STRUCT, &s->token);
	}
	else if (StrEq(_t s->token.str, "*"))
	{
		// Remove qualifier restriction and add pointer
		eflags &= ~EFLAGS_QUALIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_POINTER, &s->token);
	}
	else if (StrEq(_t s->token.str, "{"))
	{
		// Datatype embedded definition
		asm( "int $3" ); // TODO: breakpoint
	}
	else if (s->token.type == CPARSER_TOKEN_TYPE_IDENTIFIER)
	{
		// User datatype/variable/function identifier
		if (eflags & DATATYPE_DEFINED_FLAGS)
		{
			// Identifier, so end datatype and add an identifier to parent
			oo = ObjectGetParent(oo);
			oo = ObjectAddChild(oo, OBJECT_TYPE_IDENTIFIER, &s->token);
		}
		else
		{
			// Datatype not defined, so add user defined datatype identifier
			eflags |= EFLAGS_USER_DEFINED_DATATYPE;
			oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_USER_DEFINED, &s->token);
		}
	}
	else
	{
		// Unexpected token
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected token");
	}

	return oo;
}

static object_t * ProcessStateDatatype(object_t *oo, state_t *s)
{
	// Digest the datatype
	oo = DigestDataType(oo, s);

	// Update state depending on what has been digested
	if (oo->type == OBJECT_TYPE_IDENTIFIER)
		s->state = STATE_IDENTIFIER;
	else if (oo->type == OBJECT_TYPE_ERROR)
		s->state = STATE_ERROR;

	// Return to parent
	oo = ObjectGetParent(oo);

	return oo;
}

static object_t * ProcessStateIdle(object_t *oo, state_t *s)
{
	if (s->token.type == CPARSER_TOKEN_TYPE_SINGLE_CHAR)
	{
		if (s->token.str[0] == '#')
		{
			s->state = STATE_PREPROCESSOR;
			oo = ObjectAddChild(oo, OBJECT_TYPE_PREPROCESSOR_DIRECTIVE, &s->token);
		}
		else
		{
			asm( "int $3" ); // TODO: breakpoint
		}
	}
	else if (s->token.type == CPARSER_TOKEN_TYPE_IDENTIFIER)
	{
		// New unclassified identifier
		s->state = STATE_DATATYPE;
		oo = ObjectAddChild(oo, OBJECT_TYPE_TEMPORAL, NULL);
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, &s->token);

		// Add token to datatype declaration or definition
		oo = ProcessStateDatatype(oo, s);
	}
	else
	{
		asm( "int $3" ); // TODO: breakpoint
	}

	return oo;
}

static object_t * ProcessStatePreprocessor(object_t *oo, state_t *s)
{
	if (StrEq(_t s->token.str, "include"))
	{
		s->state = STATE_INCLUDE_FILENAME;
		oo = ObjectAddChild(oo, OBJECT_TYPE_INCLUDE, &s->token);	// Add include to preprocessor
		oo = ObjectGetParent(oo);									// Return to preprocessor
		s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME;
	}
	else if (StrEq(_t s->token.str, "define"))
	{
		s->state = STATE_DEFINE_IDENTIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_DEFINE, &s->token);	// Add define to preprocessor object
		oo = ObjectGetParent(oo);									// Return to preprocessor
		s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
	}
	else if (StrEq(_t s->token.str, "pragma"))
	{
		s->state = STATE_PRAGMA;
	}
	else
	{
		// Unexpected token
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected preprocessor directive");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}

	return oo;
}

static object_t * ProcessStateIncludeFilename(object_t *oo, state_t *s)
{
	s->state = STATE_IDLE;
	oo = ObjectAddChild(oo, OBJECT_TYPE_INCLUDE_FILENAME, &s->token);		// Add include filename
	oo = ObjectGetParent(oo);												// Return to preprocessor
	oo = ObjectGetParent(oo); 												// Return to preprocessor parent

	return oo;
}

static object_t * ProcessStateDefineIdentifier(object_t *oo, state_t *s)
{
	s->state = STATE_DEFINE_LITERAL;
	oo = ObjectAddChild(oo, OBJECT_TYPE_DEFINE_IDENTIFIER, &s->token);	// Add define identifier
	oo = ObjectGetParent(oo);												// Return to preprocessor
	s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_LITERAL;

	return oo;
}

static object_t * ProcessStateDefineLiteral(object_t *oo, state_t *s)
{
	s->state = STATE_IDLE;
	oo = ObjectAddChild(oo, OBJECT_TYPE_DEFINE_EXPRESSION, &s->token);	// Add define expression
	oo = ObjectGetParent(oo);												// Return to preprocessor
	oo = ObjectGetParent(oo);												// Return to preprocessor parent

	return oo;
}

static object_t *ProcessStateIdentifier(object_t *oo, state_t *s)
{
	if (StrEq(_t s->token.str, ";"))
	{
		// Sentence end after variable identifier
		oo->type = OBJECT_TYPE_VARIABLE;
		oo = ObjectAddChild(oo, OBJECT_TYPE_SENTENCE_END, &s->token);	// Add sentence end
		oo = ObjectGetParent(oo);										// Return to variable
		oo = ObjectGetParent(oo);										// Return to variable parent
		s->state = STATE_IDLE;
	}
	else if (StrEq(_t s->token.str, "["))
	{
		// Array variable identifier
		oo->type = OBJECT_TYPE_VARIABLE;
		oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_DEFINITION, &s->token);
		oo = ObjectAddChild(oo, OBJECT_TYPE_OPEN_SQ_BRACKET, &s->token);
		oo = ObjectGetParent(oo);		// return to array definition
		s->state = STATE_ARRAY_DEFINITION;
	}
	else if (StrEq(_t s->token.str, "("))
	{
		// Function identifier
		oo->type = OBJECT_TYPE_FUNCTION;
		oo = ObjectAddChild(oo, OBJECT_TYPE_FUNCTION_PARAMETERS, &s->token);
		oo = ObjectAddChild(oo, OBJECT_TYPE_OPEN_PARENTHESYS, &s->token);
		oo = ObjectGetParent(oo);
		oo = ObjectAddChild(oo, OBJECT_TYPE_PARAMETER, &s->token);
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, &s->token);
		s->state = STATE_FUNCTION_PARAMETERS;
	}
	else if (StrEq(_t s->token.str, "{"))
	{
		// User defined union, enum or struct identifier
		asm( "int $3" ); // TODO: breakpoint
	}
	else if (StrEq(_t s->token.str, "="))
	{
		// Initialization: Initial value assignation
		oo->type = OBJECT_TYPE_VARIABLE;
		oo = ObjectAddChild(oo, OBJECT_TYPE_INITIALIZATION, &s->token);
		oo = ObjectGetParent(oo);		// return to array definition
		s->state = STATE_INITIALIZATION;
	}
	else
	{
		// Unexpected token after identifier
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected token after identifier");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}

	return oo;
}

static object_t *ProcessStateInitialization(object_t *oo, state_t *s)
{
	static int32_t array_data_nesting_level = 0;

	if (oo->type != OBJECT_TYPE_ARRAY_ITEM)
	{
		array_data_nesting_level = 0;
	}

	if (StrEq(_t s->token.str, "{"))
	{
		// Array initialization data
		oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_DATA, &s->token);		// Add new array data
		oo = ObjectAddChild(oo, OBJECT_TYPE_OPEN_BRACKET, &s->token);		// Add new open bracket to array data
		oo = ObjectGetParent(oo);											// Return to array data
		oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_ITEM, &s->token);		// Add new array item
		array_data_nesting_level++;
	}
	else if (StrEq(_t s->token.str, "}"))
	{
		array_data_nesting_level--;

		if (array_data_nesting_level >= 0)
		{
			oo = ObjectGetParent(oo);										// Return to array data
			oo = ObjectAddChild(oo, OBJECT_TYPE_CLOSE_BRACKET, &s->token);	// Add close bracket
			oo = ObjectGetParent(oo);										// Return to array data
			oo = ObjectGetParent(oo);										// Return to array parent
		}
		else
		{
			// Unexpected close bracket
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Unexpected close bracket during variable initialization");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
	}
	else if (StrEq(_t s->token.str, ","))
	{
		if (array_data_nesting_level > 0)
		{
			// Add new array item
			oo = ObjectGetParent(oo);										// Return to array data
			oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_ITEM, &s->token);	// Add new array item
		}
		else
		{
			// Unexpected , token during initialization
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Unexpected , during variable initialization");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
	}
	else if (StrEq(_t s->token.str, ";"))
	{
		if (array_data_nesting_level == 0)
		{
			// Sentence end token
			oo = ObjectAddChild(oo, OBJECT_TYPE_SENTENCE_END, &s->token);	// Add new expression
			oo = ObjectGetParent(oo);										// Return to variable
			oo = ObjectGetParent(oo);										// Return to variable parent
			s->state = STATE_IDLE;
		}
		else
		{
			// Unexpected sentence end
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Unexpected sentence end during array variable initialization");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
	}
	else
	{
		// Expression -> Add expression tokens
		oo = ObjectAddChild(oo, OBJECT_TYPE_EXPRESSION_TOKEN, &s->token);	// Add new expression
		oo = ObjectGetParent(oo);											// Return to array item
	}

	return oo;
}

static object_t *ProcessStateArrayDefinition(object_t *oo, state_t *s)
{
	if (StrEq(_t s->token.str, "["))
	{
		// Unexpected open square bracket
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected open square bracket");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}
	else if (StrEq(_t s->token.str, "]"))
	{
		if (oo->type == OBJECT_TYPE_ARRAY_DEFINITION)
		{
			// Close bracket, so return to identifier state
			oo = ObjectAddChild(oo, OBJECT_TYPE_CLOSE_SQ_BRACKET, &s->token);
			oo = ObjectGetParent(oo);	// return to array definition
			oo = ObjectGetParent(oo);	// return to array definition parent
			s->state = STATE_IDENTIFIER;
		}
		else
		{
			// Unexpected token after identifier
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Unexpected close square bracket");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
	}
	else
	{
		// Digest expression token
		oo = ObjectAddChild(oo, OBJECT_TYPE_EXPRESSION_TOKEN, &s->token);
		oo = ObjectGetParent(oo);	// return to expression
	}

	return oo;
}

static object_t * ProcessStateFunctionParameters(object_t *oo, state_t *s)
{
	if (StrEq(_t s->token.str, ")"))
	{
		s->state = STATE_FUNCTION_DECLARED;

		// Return to parameter (if no identifier has been parsed)
		if (oo->type == OBJECT_TYPE_DATATYPE)
			oo = ObjectGetParent(oo);

		oo = ObjectGetParent(oo);												// Return to function parameters
		oo = ObjectAddChild(oo, OBJECT_TYPE_CLOSE_PARENTHESYS, &s->token);		// Add parenthesys
		oo = ObjectGetParent(oo);												// Return to function parameters
		oo = ObjectGetParent(oo);												// Return to function
	}
	else if (StrEq(_t s->token.str, ","))
	{
		// Return to parameter (if no identifier has been parsed)
		if (oo->type == OBJECT_TYPE_DATATYPE)
			oo = ObjectGetParent(oo);

		oo = ObjectGetParent(oo);												// Return to parameter
		oo = ObjectAddChild(oo, OBJECT_TYPE_PARAMETER_SEPARATOR, &s->token);	// Add new parameter definition
		oo = ObjectGetParent(oo);												// Return to function parameters
		oo = ObjectAddChild(oo, OBJECT_TYPE_PARAMETER, &s->token);				// Add parameter
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, &s->token);				// Add datatype for next parameter
	}
	else
	{
		// Digest the datatype
		oo = DigestDataType(oo, s);
		oo = ObjectGetParent(oo);

		// Update state
		if (oo->type == OBJECT_TYPE_ERROR)
			s->state = STATE_ERROR;
	}

	return oo;
}

static object_t * ProcessStateFunctionDeclared(object_t *oo, state_t *s)
{
	if (StrEq(_t s->token.str, ";"))
	{
		// End of function declaration
		oo->type = OBJECT_TYPE_FUNCTION_DECLARATION;
		oo = ObjectAddChild(oo, OBJECT_TYPE_SENTENCE_END, &s->token);	// Add sentence end
		oo = ObjectGetParent(oo);										// Return to function
		oo = ObjectGetParent(oo);										// Return to function parent
		s->state = STATE_IDLE;
	}
	else if (StrEq(_t s->token.str, "{"))
	{
		// Beginning of function definition
		asm( "int $3" ); // TODO: breakpoint
	}
	else
	{
		// Unexpected token after functiondeclaration
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected token after function declaration");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}

	return oo;
}

object_t *Parse(const cparserpaths_t *paths, const uint8_t *filename)
{
	object_t *oo;
	state_t s = { NULL, STATE_IDLE, DictionaryNew(), paths, 0, { CPARSER_TOKEN_TYPE_INVALID, 0, 0, { 0 } } };

	// Open file
	if (IsCSourceFilename(filename))
	{
		// Open source file
		s.file = fopen(_t filename, "rb");
	}
	else
	{
		// Open header file
		s.file = CParserPathsOpenFile(paths, filename, _T "rb");
	}

	// Create root parse object
	oo = ObjectAddChild(NULL, IsCHeaderFilename(filename) ? OBJECT_TYPE_HEADER_FILE : OBJECT_TYPE_SOURCE_FILE, NULL);
	oo->row = 0;
	oo->column = 0;
	oo->data = _T strdup(_t filename);

	// Check file exists
	if (s.file == NULL)
	{
		oo->info = _T strdup("File not found");
	}

	// Process tokens from file
	while ((s.state != STATE_ERROR) && TokenNext(s.file, &s.token, s.tokenizer_flags))
	{
		// Reset flags after read
		s.tokenizer_flags = 0;

		// Gently printing
		printf("R%d, C%d, %d:%s\n", s.token.row, s.token.column, s.token.type, s.token.str);

		// Process tokens
		if (s.token.type == CPARSER_TOKEN_TYPE_C_COMMENT) {
			oo = ObjectAddChild(oo, OBJECT_TYPE_C_COMMENT, &s.token);
			oo = ObjectGetParent(oo);
		}
		else if (s.token.type == CPARSER_TOKEN_TYPE_CPP_COMMENT) {
			oo = ObjectAddChild(oo, OBJECT_TYPE_CPP_COMMENT, &s.token);
			oo = ObjectGetParent(oo);
		}
		else
		{
			// Check new tokens
			if (s.state == STATE_IDLE)
			{
				oo = ProcessStateIdle(oo, &s);
			}
			else if (s.state == STATE_PREPROCESSOR)
			{
				oo = ProcessStatePreprocessor(oo, &s);
			}
			else if (s.state == STATE_INCLUDE_FILENAME)
			{
				oo = ProcessStateIncludeFilename(oo, &s);
			}
			else if (s.state == STATE_DEFINE_IDENTIFIER)
			{
				oo = ProcessStateDefineIdentifier(oo, &s);
			}
			else if (s.state == STATE_DEFINE_LITERAL)
			{
				oo = ProcessStateDefineLiteral(oo, &s);
			}
			else if (s.state == STATE_DATATYPE)
			{
				oo = ProcessStateDatatype(oo, &s);
			}
			else if (s.state == STATE_IDENTIFIER)
			{
				oo = ProcessStateIdentifier(oo, &s);
			}
			else if (s.state == STATE_ARRAY_DEFINITION)
			{
				oo = ProcessStateArrayDefinition(oo, &s);
			}
			else if (s.state == STATE_INITIALIZATION)
			{
				oo = ProcessStateInitialization(oo, &s);
			}
			else if (s.state == STATE_FUNCTION_PARAMETERS)
			{
				oo = ProcessStateFunctionParameters(oo, &s);
			}
			else if (s.state == STATE_FUNCTION_DECLARED)
			{
				oo = ProcessStateFunctionDeclared(oo, &s);
			}
			else
			{
				asm( "int $3" ); // TODO: breakpoint
			}
		}
	}

	return oo;
}
