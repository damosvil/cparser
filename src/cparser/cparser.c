/*
 * cparser.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cparserpaths.h"
#include "cparsertools.h"
#include "cparsertoken.h"
#include "cparserobject.h"
#include "cparserdictionary.h"
#include "cparserstack.h"
#include "cparser.h"

#define KEYWORDS_C_COUNT				34
#define KEYWORDS_PREPROCESSOR_COUNT		13

#define DATATYPE_DEFINED_FLAGS  		(\
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
	STATE_DATATYPE,
	STATE_PRAGMA,
	STATE_IDENTIFIER,
	STATE_ARRAY_DEFINITION,
	STATE_INITIALIZATION,
	STATE_FUNCTION_PARAMETERS,
	STATE_FUNCTION_DECLARED,
	STATE_ERROR
} states_t;

typedef enum preprocessor_state_e
{
	PREPROCESSOR_STATE_IDLE = 0,			// Not parsing preprocessor
	PREPROCESSOR_STATE_NEW_DIRECTIVE,		// Parsing new preprocessor directive
	PREPROCESSOR_STATE_INCLUDE_FILENAME,	// Parsing include filename
	PREPROCESSOR_STATE_DEFINE_IDENTIFIER,	// Parsing define identifier
	PREPROCESSOR_STATE_DEFINE_LITERAL,		// Parsing define literal or expression
	PREPROCESSOR_STATE_IFNDEF,				// Parsing ifndef identifier
	PREPROCESSOR_STATE_IFDEF,				// Parsing ifdef identifier
	PREPROCESSOR_STATE_ERROR,				// Parsing error string literal
} preprocessor_state_t;

typedef enum conditional_compilation_state_s
{
	CONDITIONAL_COMPILATION_STATE_IDLE = 0,			// Like accepting but before any #if???? expression
	CONDITIONAL_COMPILATION_STATE_ACCEPTING,		// Accepting tokens after a valid #if expression until an #elif or an #endif
	CONDITIONAL_COMPILATION_STATE_LOOKING,			// Looking for a valid expression, an #else or an #endif
	CONDITIONAL_COMPILATION_STATE_SKIPPING,			// Skipping tokens until an #elif, #else or an #endif
	CONDITIONAL_COMPILATION_STATE_SKIPPING_ELSE,	// Skipping tokens until #endif
	CONDITIONAL_COMPILATION_STATE_ACCEPTING_ELSE	// Accepting tokens untiel #endif
} conditional_compilation_state_t;

// Parsing state
typedef struct state_s
{
	FILE *file;
	states_t state;
	preprocessor_state_t preprocessor_state;
	cparserdictionary_t *dictionary;
	cparserpaths_t *paths;
	uint32_t tokenizer_flags;
	token_t token;
	cparserstack_t *conditional_compilation_stack;
	conditional_compilation_state_t conditional_compilation_state;
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


static const uint8_t *keywords_c[KEYWORDS_C_COUNT] =
{
	_T "auto",		_T "break",		_T "case",		_T "char",
	_T "const",		_T "continue",	_T "default",	_T "do",
	_T "double",	_T "else",		_T "enum",		_T "extern",
	_T "float",		_T "for",		_T "goto",		_T "if",
	_T "inline",	_T "int",		_T "long",		_T "register",
	_T "restrict",	_T "return",	_T "short",		_T "signed",
	_T "sizeof",	_T "static",	_T "struct",	_T "switch",
	_T "typedef",	_T "union",		_T "unsigned",	_T "void",
	_T "volatile",	_T "while"
};

static const uint8_t *keywords_preprocessor[KEYWORDS_PREPROCESSOR_COUNT] =
{
	_T "define", 	_T "defined",	_T "elif",		_T "else",
	_T "endif",		_T "error",		_T "if",		_T "ifdef",
	_T "ifndef",	_T "include",	_T "line",		_T "pragma",
	_T "undef"
};


static int CompareStringInSet(const void *a, const void *b)
{
	char *aa = (char *)a;
	char *bb = (char *)b;

	return strcmp(aa, bb);
}

static bool StringInSet(const uint8_t *string, const uint8_t **set, uint32_t lenght)
{
	return bsearch(string, set, lenght, sizeof(uint8_t *), CompareStringInSet) != NULL;
}

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
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Specifier already defined");
		}
		else
		{
			eflags |= EFLAGS_SPECIFIER;
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_SPECIFIER, &s->token);
		}
	}
	else if (StrEq(_t s->token.str, "const") || StrEq(_t s->token.str, "volatile"))
	{
		// Check qualifiers
		if (eflags & EFLAGS_QUALIFIER)
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Qualifier already defined");
		}
		else
		{
			eflags |= EFLAGS_QUALIFIER;
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_QUALIFIER, &s->token);
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
				oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_MODIFIER, &s->token);
			}
			else
			{
				oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
				oo->info = _T strdup("Cannot apply the same modifier twice");
			}
		}
		else if (eflags & EFLAGS_USER_DEFINED_DATATYPE)
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers to user defined datatypes");
		}
		else if ((eflags & EFLAGS_VOID) && newf)
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers to void datatype");
		}
		else if ((eflags & EFLAGS_CHAR) && (newf & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifier short nor long to char datatype");
		}
		else if ((eflags & EFLAGS_FLOAT) && newf)
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers to float datatype");
		}
		else if ((eflags & EFLAGS_DOUBLE) && (newf & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SIGNED)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply modifiers short, unsigned not signed to double datatype");
		}
		else if (((eflags & EFLAGS_MODIFIER_UNSIGNED) && (newf & EFLAGS_MODIFIER_SIGNED)) ||
				((eflags & EFLAGS_MODIFIER_SIGNED) && (newf & EFLAGS_MODIFIER_UNSIGNED)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply signed and unsigned modifiers at the same time");
		}
		else if (((eflags & EFLAGS_MODIFIER_LONG) && (newf & EFLAGS_MODIFIER_SHORT)) ||
				((eflags & EFLAGS_MODIFIER_SHORT) && (newf & EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot apply long and short modifiers at the same time");
		}
		else
		{
			eflags |= newf;
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_MODIFIER, &s->token);
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
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify the same basic built in datatype twice");
		}
		else if (eflags & EFLAGS_USER_DEFINED_DATATYPE)
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify a basic built in datatype when it is already defined a used defined datatype");
		}
		else if ((newf & EFLAGS_VOID) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify modifiers to void datatype");
		}
		else if ((newf & EFLAGS_CHAR) && (eflags & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify short nor long modifiers to char datatype");
		}
		else if ((newf & EFLAGS_FLOAT) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify modifiers to float datatype");
		}
		else if ((newf & EFLAGS_DOUBLE) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT)))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Cannot specify signed, unsigned nor short modifiers to double datatype");
		}
		else
		{
			eflags |= newf;
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DATATYPE_PRIMITIVE, &s->token);
		}
	}
	else if (StrEq(_t s->token.str, "union") || StrEq(_t s->token.str, "enum") || StrEq(_t s->token.str, "struct"))
	{
		// Possible datatype definition, variable definition, function definition
		eflags |= ~EFLAGS_COMPOSED_DATATYPE;

		// Add child
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DATATYPE_USER_DEFINED, &s->token);
		if (s->token.str[0] == 'u')
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_UNION, &s->token);
		else if (s->token.str[0] == 'e')
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ENUM, &s->token);
		else
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_STRUCT, &s->token);
	}
	else if (StrEq(_t s->token.str, "*"))
	{
		// Remove qualifier restriction and add pointer
		eflags &= ~EFLAGS_QUALIFIER;
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_POINTER, &s->token);
	}
	else if (StrEq(_t s->token.str, "{"))
	{
		// Datatype embedded definition
		__builtin_trap(); // TODO: breakpoint
	}
	else if (s->token.type == CPARSER_TOKEN_TYPE_IDENTIFIER)
	{
		if (StringInSet(s->token.str, keywords_c, KEYWORDS_C_COUNT))
		{
			// Detected C keyword
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Use of C keywords as identifiers is not allowed");
		}
		else if (DictionaryExistsKey(s->dictionary, s->token.str))
		{
			// Detected identifier already in use
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Identifier already in use");
		}
		else
		{
			// User datatype/variable/function identifier
			if (eflags & DATATYPE_DEFINED_FLAGS)
			{
				// Identifier, so end datatype and add an identifier to parent
				oo = ObjectGetParent(oo);
				oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_IDENTIFIER, &s->token);
			}
			else
			{
				// Datatype not defined, so add user defined datatype identifier
				eflags |= EFLAGS_USER_DEFINED_DATATYPE;
				oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DATATYPE_USER_DEFINED, &s->token);
			}

			// Add define identifier to dictionary
			DictionarySetKeyValue(s->dictionary, s->token.str, oo);
		}
	}
	else
	{
		// Unexpected token
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
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
	if (s->token.type == CPARSER_TOKEN_TYPE_IDENTIFIER)
	{
		// New unclassified identifier
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_TEMPORAL, NULL);
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DATATYPE, &s->token);
		s->state = STATE_DATATYPE;

		// Add token to datatype declaration or definition
		oo = ProcessStateDatatype(oo, s);
	}
	else
	{
		// Unexpected token in global scope
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected token in global scope");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}

	return oo;
}

static object_t * ProcessCComment(object_t *oo, state_t *s)
{
	if (
			(s->conditional_compilation_state == CONDITIONAL_COMPILATION_STATE_IDLE) ||
			(s->conditional_compilation_state == CONDITIONAL_COMPILATION_STATE_ACCEPTING)
		)
	{
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_C_COMMENT, &s->token);
		oo = ObjectGetParent(oo);
	}

	return oo;
}

static object_t * ProcessCppComment(object_t *oo, state_t *s)
{
	if (
			(s->conditional_compilation_state == CONDITIONAL_COMPILATION_STATE_IDLE) ||
			(s->conditional_compilation_state == CONDITIONAL_COMPILATION_STATE_ACCEPTING)
		)
	{
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_CPP_COMMENT, &s->token);
		oo = ObjectGetParent(oo);
	}

	return oo;
}

static object_t * ProcessNewDirective(object_t *oo, state_t *s)
{
	// Check directive is in new line
	if (s->token.first_token_in_line)
	{
		// Add new preprocessor directive object and start parsing it
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_DIRECTIVE, &s->token);
		s->preprocessor_state = PREPROCESSOR_STATE_NEW_DIRECTIVE;
	}
	else
	{
		// Invalid preprocessor directive
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Invalid preprocessor directive");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}

	return oo;
}

static object_t * ProcessPreprocessorStateNewDirective(object_t *oo, state_t *s)
{
	switch (s->conditional_compilation_state)
	{

	case CONDITIONAL_COMPILATION_STATE_IDLE:
		if (StrEq(_t s->token.str, "include"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_INCLUDE, &s->token);	// Add include to preprocessor
			oo = ObjectGetParent(oo);											// Return to preprocessor

			s->preprocessor_state = PREPROCESSOR_STATE_INCLUDE_FILENAME;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME;
		}
		else if (StrEq(_t s->token.str, "define"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DEFINE, &s->token);	// Add define to preprocessor object
			oo = ObjectGetParent(oo);											// Return to preprocessor

			s->preprocessor_state = PREPROCESSOR_STATE_DEFINE_IDENTIFIER;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "pragma"))
		{
			__builtin_trap(); // TODO: pragma
		}
		else if (StrEq(_t s->token.str, "error"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ERROR, &s->token);	// Add error to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor

			// Go to preprocessor state ERROR to read error string literal
			s->preprocessor_state = PREPROCESSOR_STATE_ERROR;
		}
		else if (StrEq(_t s->token.str, "if"))
		{
			__builtin_trap(); // TODO: if
		}
		else if (StrEq(_t s->token.str, "ifdef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IFDEF, &s->token);	// Add ifdef to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			s->preprocessor_state = PREPROCESSOR_STATE_IFDEF;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "ifndef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IFNDEF, &s->token);	// Add ifndef to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			s->preprocessor_state = PREPROCESSOR_STATE_IFNDEF;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "elif"))
		{
			// elif without if
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("#elif without #if");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		else if (StrEq(_t s->token.str, "else"))
		{
			// else without if
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("#else without #if");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		else if (StrEq(_t s->token.str, "endif"))
		{
			// endif without if
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("#endif without #if");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		else
		{
			// Invalid preprocessor directive
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Invalid preprocessor directive");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		break;

	case CONDITIONAL_COMPILATION_STATE_ACCEPTING:
		if (StrEq(_t s->token.str, "include"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_INCLUDE, &s->token);	// Add include to preprocessor
			oo = ObjectGetParent(oo);											// Return to preprocessor
			s->preprocessor_state = PREPROCESSOR_STATE_INCLUDE_FILENAME;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME;
		}
		else if (StrEq(_t s->token.str, "define"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DEFINE, &s->token);	// Add define to preprocessor object
			oo = ObjectGetParent(oo);											// Return to preprocessor
			s->preprocessor_state = PREPROCESSOR_STATE_DEFINE_IDENTIFIER;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "pragma"))
		{
			__builtin_trap(); // TODO: pragma
		}
		else if (StrEq(_t s->token.str, "error"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ERROR, &s->token);	// Add error to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor

			// Go to preprocessor state ERROR to read error string literal
			s->preprocessor_state = PREPROCESSOR_STATE_ERROR;
		}
		else if (StrEq(_t s->token.str, "if"))
		{
			__builtin_trap(); // TODO: if
		}
		else if (StrEq(_t s->token.str, "ifdef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IFDEF, &s->token);	// Add ifdef to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor

			// Go to preprocessor state and prepare to read a define identifier
			s->preprocessor_state = PREPROCESSOR_STATE_IFDEF;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "ifndef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IFNDEF, &s->token);	// Add ifndef to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor

			// Go to preprocessor state and prepare to read a define identifier
			s->preprocessor_state = PREPROCESSOR_STATE_IFNDEF;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "elif"))
		{
			__builtin_trap(); // TODO: elif
		}
		else if (StrEq(_t s->token.str, "else"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ELSE, &s->token);		// Add else to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			oo = ObjectGetParent(oo);														// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Update conditional compilation state
			s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_SKIPPING_ELSE;
		}
		else if (StrEq(_t s->token.str, "endif"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ENDIF, &s->token);	// Add endif to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			oo = ObjectGetParent(oo);														// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Pop conditional compilation state
			StackPopBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));
		}
		else
		{
			// Invalid preprocessor directive
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Invalid preprocessor directive");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		break;

	case CONDITIONAL_COMPILATION_STATE_LOOKING:
		if (StrEq(_t s->token.str, "if") || StrEq(_t s->token.str, "ifdef") || StrEq(_t s->token.str, "ifndef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IF, &s->token);	// Add if to preprocessor object
			oo = ObjectGetParent(oo);													// Return to preprocessor
			oo = ObjectGetParent(oo);													// Return to preprocessor parent

			// Return to IDLE preprocessor state
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Increase conditional compilation stack level
			StackPushBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));

			// Go directly to skipping conditional compilation state
			s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_SKIPPING;
		}
		else if (StrEq(_t s->token.str, "elif"))
		{
			__builtin_trap(); // TODO: elif
		}
		else if (StrEq(_t s->token.str, "else"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ELSE, &s->token);	// Add else to preprocessor object
			oo = ObjectGetParent(oo);													// Return to preprocessor
			oo = ObjectGetParent(oo);													// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Go to accepting else conditional compilation state
			s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_ACCEPTING_ELSE;
		}
		else if (StrEq(_t s->token.str, "endif"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ENDIF, &s->token);	// Add endif to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			oo = ObjectGetParent(oo);														// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Pop conditional compilation state
			StackPopBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));
		}
		break;

	case CONDITIONAL_COMPILATION_STATE_SKIPPING:
		if (StrEq(_t s->token.str, "if") || StrEq(_t s->token.str, "ifdef") || StrEq(_t s->token.str, "ifndef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IF, &s->token);	// Add endif to preprocessor object
			oo = ObjectGetParent(oo);													// Return to preprocessor
			oo = ObjectGetParent(oo);													// Return to preprocessor parent

			// Return to IDLE preprocessor state
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Increase conditional compilation stack level
			StackPushBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));
		}
		else if (StrEq(_t s->token.str, "elif"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ELIF, &s->token);	// Add elif to preprocessor object
			oo = ObjectGetParent(oo);													// Return to preprocessor
			oo = ObjectGetParent(oo);													// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;
		}
		else if (StrEq(_t s->token.str, "else"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ELSE, &s->token);	// Add endif to preprocessor object
			oo = ObjectGetParent(oo);													// Return to preprocessor
			oo = ObjectGetParent(oo);													// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;
		}
		else if (StrEq(_t s->token.str, "endif"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ENDIF, &s->token);	// Add endif to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			oo = ObjectGetParent(oo);														// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Pop conditional compilation state
			StackPopBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));
		}
		break;

	case CONDITIONAL_COMPILATION_STATE_SKIPPING_ELSE:
		if (StrEq(_t s->token.str, "if") || StrEq(_t s->token.str, "ifdef") || StrEq(_t s->token.str, "ifndef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IF, &s->token);	// Add if to preprocessor object
			oo = ObjectGetParent(oo);													// Return to preprocessor
			oo = ObjectGetParent(oo);													// Return to preprocessor parent

			// Return to IDLE preprocessor state
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Increase conditional compilation stack level
			StackPushBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));

			// Go directly to skipping conditional compilation state
			s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_SKIPPING;
		}
		else if (StrEq(_t s->token.str, "elif"))
		{
			// #elif after #else
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Invalid #elif after #else");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		if (StrEq(_t s->token.str, "else"))
		{
			// #else after #else
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Invalid #else after #else");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		else if (StrEq(_t s->token.str, "endif"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ENDIF, &s->token);	// Add endif to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			oo = ObjectGetParent(oo);														// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Pop conditional compilation state
			StackPopBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));
		}
		break;

	case CONDITIONAL_COMPILATION_STATE_ACCEPTING_ELSE:
		if (StrEq(_t s->token.str, "if"))
		{
			__builtin_trap(); // TODO: if
		}
		else if (StrEq(_t s->token.str, "ifdef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IFDEF, &s->token);	// Add ifdef to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			s->preprocessor_state = PREPROCESSOR_STATE_IFDEF;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "ifndef"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_IFNDEF, &s->token);	// Add ifndef to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			s->preprocessor_state = PREPROCESSOR_STATE_IFNDEF;
			s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
		}
		else if (StrEq(_t s->token.str, "elif"))
		{
			// #elif after #else
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Invalid #elif after #else");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		if (StrEq(_t s->token.str, "else"))
		{
			// #else after #else
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Invalid #else after #else");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
		else if (StrEq(_t s->token.str, "endif"))
		{
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PREPROCESSOR_ENDIF, &s->token);	// Add endif to preprocessor object
			oo = ObjectGetParent(oo);														// Return to preprocessor
			oo = ObjectGetParent(oo);														// Return to preprocessor parent

			// Return preprocessor state to IDLE
			s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

			// Pop conditional compilation state
			StackPopBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));
		}
		break;

	default:
		break;

	}

	return oo;
}

static object_t * ProcessPreprocessorStateIncludeFilename(object_t *oo, state_t *s)
{
	uint32_t len = strlen(_t s->token.str);
	uint8_t *filename = (len < 3) ? NULL : _T strndup(_t s->token.str + 1, len - 2);
	object_t *nn = CParserParse(s->dictionary, s->paths, filename);

	oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_INCLUDE_FILENAME, &s->token);		// Add include filename
	oo = ObjectGetParent(oo);														// Return to preprocessor
	ObjectAddChild(oo, nn);															// Add include object
	oo = ObjectGetParent(oo);														// Return to preprocessor
	oo = ObjectGetParent(oo); 														// Return to preprocessor parent

	// Return preprocessor state to IDLE
	s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

	return oo;
}

static object_t * ProcessPreprocessorStateDefineIdentifier(object_t *oo, state_t *s)
{
	if (StringInSet(s->token.str, keywords_c, KEYWORDS_C_COUNT))
	{
		// Trying to define a c keyword
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Trying to define a C keyword");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}
	else if (StringInSet(s->token.str, keywords_preprocessor, KEYWORDS_PREPROCESSOR_COUNT))
	{
		// Trying to define a preprocessor keyword
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Trying to define a preprocessor keyword");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}
	else if (DictionaryExistsKey(s->dictionary, s->token.str))
	{
		// Trying to redefine an already defined symbol
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Trying to define an already defined symbol");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}
	else
	{
		// Add define identifier and return to preprocessor
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DEFINE_IDENTIFIER, &s->token);		// Add identifier to preprocessor

		// Add define identifier to dictionary
		DictionarySetKeyValue(s->dictionary, s->token.str, oo);							// Add definition to dictionary
		oo = ObjectGetParent(oo);														// Return to preprocessor

		// Update state and prepare for parsing a define literal
		s->preprocessor_state = PREPROCESSOR_STATE_DEFINE_LITERAL;
		s->tokenizer_flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_LITERAL;
	}

	return oo;
}

static object_t * ProcessPreprocessorStateDefineLiteral(object_t *oo, state_t *s)
{
	oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DEFINE_EXPRESSION, &s->token);	// Add define expression
	oo = ObjectGetParent(oo);													// Return to preprocessor
	oo = ObjectGetParent(oo);													// Return to preprocessor parent

	// Return preprocessor state to IDLE
	s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

	return oo;
}

static object_t *ProcessStateIdentifier(object_t *oo, state_t *s)
{
	if (StrEq(_t s->token.str, ";"))
	{
		// Sentence end after variable identifier
		oo->type = OBJECT_TYPE_VARIABLE;
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_SENTENCE_END, &s->token);	// Add sentence end
		oo = ObjectGetParent(oo);												// Return to variable
		oo = ObjectGetParent(oo);												// Return to variable parent
		s->state = STATE_IDLE;
	}
	else if (StrEq(_t s->token.str, "["))
	{
		// Array variable identifier
		oo->type = OBJECT_TYPE_VARIABLE;
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ARRAY_DEFINITION, &s->token);
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_OPEN_SQ_BRACKET, &s->token);
		oo = ObjectGetParent(oo);		// return to array definition
		s->state = STATE_ARRAY_DEFINITION;
	}
	else if (StrEq(_t s->token.str, "("))
	{
		// Function identifier
		oo->type = OBJECT_TYPE_FUNCTION;
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_FUNCTION_PARAMETERS, &s->token);
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_OPEN_PARENTHESYS, &s->token);
		oo = ObjectGetParent(oo);
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PARAMETER, &s->token);
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DATATYPE, &s->token);
		s->state = STATE_FUNCTION_PARAMETERS;
	}
	else if (StrEq(_t s->token.str, "{"))
	{
		// User defined union, enum or struct identifier
		__builtin_trap(); // TODO: breakpoint
	}
	else if (StrEq(_t s->token.str, "="))
	{
		// Initialization: Initial value assignation
		oo->type = OBJECT_TYPE_VARIABLE;
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_INITIALIZATION, &s->token);
		oo = ObjectGetParent(oo);		// return to array definition
		s->state = STATE_INITIALIZATION;
	}
	else
	{
		// Unexpected token after identifier
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
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
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ARRAY_DATA, &s->token);		// Add new array data
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_OPEN_BRACKET, &s->token);		// Add new open bracket to array data
		oo = ObjectGetParent(oo);													// Return to array data
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ARRAY_ITEM, &s->token);		// Add new array item
		array_data_nesting_level++;
	}
	else if (StrEq(_t s->token.str, "}"))
	{
		array_data_nesting_level--;

		if (array_data_nesting_level >= 0)
		{
			oo = ObjectGetParent(oo);												// Return to array data
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_CLOSE_BRACKET, &s->token);	// Add close bracket
			oo = ObjectGetParent(oo);												// Return to array data
			oo = ObjectGetParent(oo);												// Return to array parent
		}
		else
		{
			// Unexpected close bracket
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
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
			oo = ObjectGetParent(oo);												// Return to array data
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ARRAY_ITEM, &s->token);	// Add new array item
		}
		else
		{
			// Unexpected , token during initialization
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
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
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_SENTENCE_END, &s->token);	// Add new expression
			oo = ObjectGetParent(oo);												// Return to variable
			oo = ObjectGetParent(oo);												// Return to variable parent
			s->state = STATE_IDLE;
		}
		else
		{
			// Unexpected sentence end
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Unexpected sentence end during array variable initialization");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
	}
	else
	{
		// Expression -> Add expression tokens
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_EXPRESSION_TOKEN, &s->token);	// Add new expression
		oo = ObjectGetParent(oo);													// Return to array item
	}

	return oo;
}

static object_t *ProcessStateArrayDefinition(object_t *oo, state_t *s)
{
	if (StrEq(_t s->token.str, "["))
	{
		// Unexpected open square bracket
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected open square bracket");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}
	else if (StrEq(_t s->token.str, "]"))
	{
		if (oo->type == OBJECT_TYPE_ARRAY_DEFINITION)
		{
			// Close bracket, so return to identifier state
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_CLOSE_SQ_BRACKET, &s->token);
			oo = ObjectGetParent(oo);	// return to array definition
			oo = ObjectGetParent(oo);	// return to array definition parent
			s->state = STATE_IDENTIFIER;
		}
		else
		{
			// Unexpected token after identifier
			oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
			oo->info = _T strdup("Unexpected close square bracket");
			oo = ObjectGetParent(oo);
			s->state = STATE_ERROR;
		}
	}
	else
	{
		// Digest expression token
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_EXPRESSION_TOKEN, &s->token);
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

		oo = ObjectGetParent(oo);														// Return to function parameters
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_CLOSE_PARENTHESYS, &s->token);		// Add parenthesys
		oo = ObjectGetParent(oo);														// Return to function parameters
		oo = ObjectGetParent(oo);														// Return to function
	}
	else if (StrEq(_t s->token.str, ","))
	{
		// Return to parameter (if no identifier has been parsed)
		if (oo->type == OBJECT_TYPE_DATATYPE)
			oo = ObjectGetParent(oo);

		oo = ObjectGetParent(oo);														// Return to parameter
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PARAMETER_SEPARATOR, &s->token);	// Add new parameter definition
		oo = ObjectGetParent(oo);														// Return to function parameters
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_PARAMETER, &s->token);				// Add parameter
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DATATYPE, &s->token);				// Add datatype for next parameter
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
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_SENTENCE_END, &s->token);	// Add sentence end
		oo = ObjectGetParent(oo);												// Return to function
		oo = ObjectGetParent(oo);												// Return to function parent
		s->state = STATE_IDLE;
	}
	else if (StrEq(_t s->token.str, "{"))
	{
		// Beginning of function definition
		__builtin_trap(); // TODO: breakpoint
	}
	else
	{
		// Unexpected token after function declaration
		oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);
		oo->info = _T strdup("Unexpected token after function declaration");
		oo = ObjectGetParent(oo);
		s->state = STATE_ERROR;
	}

	return oo;
}

static object_t * ProcessPreprocessorStateIfndef(object_t *oo, state_t *s)
{
	oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DEFINE_IDENTIFIER, &s->token);	// Add include to preprocessor
	oo = ObjectGetParent(oo);													// Return to preprocessor
	oo = ObjectGetParent(oo);													// Return to preprocessor parent

	// Return preprocessor state to IDLE
	s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

	// Increase conditional compilation stack level
	StackPushBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));

	// Update conditional compilation state depending on the requested identifier exists or not
	if (!DictionaryExistsKey(s->dictionary, s->token.str))
	{
		s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_ACCEPTING;
	}
	else
	{
		s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_LOOKING;
	}

	return oo;
}

static object_t * ProcessPreprocessorStateIfdef(object_t *oo, state_t *s)
{
	oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_DEFINE_IDENTIFIER, &s->token);	// Add include to preprocessor
	oo = ObjectGetParent(oo);													// Return to preprocessor
	oo = ObjectGetParent(oo);													// Return to preprocessor parent

	// Return preprocessor state to IDLE
	s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

	// Increase conditional compilation stack level
	StackPushBytes(s->conditional_compilation_stack, &s->conditional_compilation_state, sizeof(s->conditional_compilation_state));

	// Update conditional compilation state depending on the requested identifier exists or not
	if (DictionaryExistsKey(s->dictionary, s->token.str))
	{
		s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_ACCEPTING;
	}
	else
	{
		s->conditional_compilation_state = CONDITIONAL_COMPILATION_STATE_LOOKING;
	}

	return oo;
}

static object_t * ProcessPreprocessorStateError(object_t *oo, state_t *s)
{
	oo = ObjectAddChildFromToken(oo, OBJECT_TYPE_ERROR, &s->token);	// Add error
	oo = ObjectGetParent(oo);										// Return to preprocessor
	oo = ObjectGetParent(oo);										// Return to preprocessor parent

	// Return to idle preprocessor state
	s->preprocessor_state = PREPROCESSOR_STATE_IDLE;

	// Stop parsing going to ERROR parsing state
	s->state = STATE_ERROR;

	return oo;
}

object_t *CParserParse(cparserdictionary_t *dictionary, cparserpaths_t *paths, const uint8_t *filename)
{
	object_t *oo;
	state_t s = {
			NULL, STATE_IDLE, PREPROCESSOR_STATE_IDLE, dictionary, paths, 0,
			{ CPARSER_TOKEN_TYPE_INVALID, false, 0, 0, malloc(MAX_SENTENCE_LENGTH + 10) },
			StackNew(), CONDITIONAL_COMPILATION_STATE_IDLE };

	// Create root parse object
	oo = ObjectAddChildFromToken(NULL, IsCHeaderFilename(filename) ? OBJECT_TYPE_HEADER_FILE : OBJECT_TYPE_SOURCE_FILE, NULL);

	// Open file
	if (IsCSourceFilename(filename))
	{
		// Open source file
		s.file = filename ? fopen(_t filename, "rb") : NULL;
	}
	else
	{
		// Open as header file
		s.file = paths ? PathsOpenFile(paths, filename, _T "rb") : NULL;
	}

	// Check file exists
	if (s.file == NULL)
	{
		// Add file not found error and set state to ERROR to force end parsing
		oo->type = OBJECT_TYPE_ERROR;
		oo->info = _T strdup("File not found");
		s.state = STATE_ERROR;
	}
	else
	{
		// Set object data. At this point s.state is IDLE so parsing will start
		oo->data = _T strdup(_t filename);
	}

	// Process tokens from file
	while ((s.state != STATE_ERROR) && TokenNext(s.file, &s.token, s.tokenizer_flags))
	{
		// Reset flags after read
		s.tokenizer_flags = 0;

		// Gently printing
		printf("R%d, C%d, %d:%s\n", s.token.row, s.token.column, s.token.type, s.token.str);

		// Process tokens
		if (s.token.type == CPARSER_TOKEN_TYPE_C_COMMENT)
		{
			oo = ProcessCComment(oo, &s);
		}
		else if (s.token.type == CPARSER_TOKEN_TYPE_CPP_COMMENT)
		{
			oo = ProcessCppComment(oo, &s);
		}
		else if (s.token.type == CPARSER_TOKEN_TYPE_SINGLE_CHAR && s.token.str[0] == '#')
		{
			oo = ProcessNewDirective(oo, &s);
		}
		else
		{
			// Process preprocessor states
			if (s.preprocessor_state == PREPROCESSOR_STATE_NEW_DIRECTIVE)
			{
				oo = ProcessPreprocessorStateNewDirective(oo, &s);
			}
			else if (s.preprocessor_state == PREPROCESSOR_STATE_INCLUDE_FILENAME)
			{
				oo = ProcessPreprocessorStateIncludeFilename(oo, &s);
			}
			else if (s.preprocessor_state == PREPROCESSOR_STATE_DEFINE_IDENTIFIER)
			{
				oo = ProcessPreprocessorStateDefineIdentifier(oo, &s);
			}
			else if (s.preprocessor_state == PREPROCESSOR_STATE_DEFINE_LITERAL)
			{
				oo = ProcessPreprocessorStateDefineLiteral(oo, &s);
			}
			else if (s.preprocessor_state == PREPROCESSOR_STATE_IFNDEF)
			{
				oo = ProcessPreprocessorStateIfndef(oo, &s);
			}
			else if (s.preprocessor_state == PREPROCESSOR_STATE_IFDEF)
			{
				oo = ProcessPreprocessorStateIfdef(oo, &s);
			}
			else if (s.preprocessor_state == PREPROCESSOR_STATE_ERROR)
			{
				oo = ProcessPreprocessorStateError(oo, &s);
			}
			else if (
					(s.conditional_compilation_state == CONDITIONAL_COMPILATION_STATE_ACCEPTING) ||
					(s.conditional_compilation_state == CONDITIONAL_COMPILATION_STATE_IDLE)
					)
			{
				// Process parsing states
				if (s.state == STATE_IDLE)
				{
					oo = ProcessStateIdle(oo, &s);
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
					__builtin_trap(); // TODO: unimplemented state
				}
			}
			else
			{
				__builtin_trap(); // TODO: unimplemented preprocessor state
			}
		}
	}

	// Delete token requested str buffer
	free(s.token.str);

	// Delete stack
	StackDelete(s.conditional_compilation_stack);

	return oo;
}
