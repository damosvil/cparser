/*
 * cparser.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cparserpaths.h"
#include "cparsertools.h"
#include "cparsertoken.h"
#include "cparserobject.h"
#include "cparser.h"

//static const char *keywords[] = {
//	"auto", "break", "case", "char",
//	"const", "continue", "default", "do",
//	"double", "else", "enum", "extern",
//	"float", "for", "goto", "if",
//	"int", "long", "register", "return",
//	"short", "signed", "sizeof", "static",
//	"struct", "switch", "typedef", "union",
//	"unsigned", "void", "volatile", "while",
//	NULL
//};


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
};


#define DATATYPE_DEFINED_FLAGS  	(\
									EFLAGS_MODIFIER_SIGNED 		       |\
									EFLAGS_MODIFIER_UNSIGNED 	       |\
									EFLAGS_MODIFIER_SHORT 		       |\
									EFLAGS_MODIFIER_LONG 		       |\
									EFLAGS_MODIFIER_LONG_LONG 	       |\
									EFLAGS_VOID 				       |\
									EFLAGS_CHAR 				       |\
									EFLAGS_INT 					       |\
									EFLAGS_FLOAT 				       |\
									EFLAGS_DOUBLE 				       |\
									EFLAGS_USER_DEFINED_DATATYPE		\
									)



namespace cparser {

cparser::cparser(const cparser_paths *paths, const uint8_t *filename)
{
	if (!paths)
		throw "cparser incorrect input parameter: paths";

	if (!filename)
		throw "cparser incorrect input parameter: filename";

	if (!IsCSourceFilename(filename) && !IsCHeaderFilename(filename))
		throw "cparser filename is neither C header file nor C source file";

	// Keep a copy of paths and filename
	this->paths = new cparser_paths(paths);
	this->filename = StrDup(filename);
}

cparser::~cparser()
{
	delete paths;
	delete filename;
}

object_s * cparser::StateDatatypeProcessToken(object_s *oo, states_e &s, token_s *tt)
{
	static uint32_t eflags = EFLAGS_NONE;

	// Initialize acceptance flags
	if (oo->children_count == 0)
	{
		eflags = EFLAGS_NONE;
	}

	// Compose datatype
	if (StrEq(tt->str, "static") || StrEq(tt->str, "extern") || StrEq(tt->str, "auto") ||
			StrEq(tt->str, "register") || StrEq(tt->str, "typedef"))
	{
		// Check specifiers
		if (eflags & EFLAGS_SPECIFIER)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Specifier already defined");
			s = STATE_IDLE;
		}
		else
		{
			eflags |= EFLAGS_SPECIFIER;
			oo = ObjectAddChild(oo, OBJECT_TYPE_SPECIFIER, tt);
		}
	}
	else if (StrEq(tt->str, "const") || StrEq(tt->str, "volatile"))
	{
		// Check qualifiers
		if (eflags & EFLAGS_QUALIFIER)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Qualifier already defined");
			s = STATE_IDLE;
		}
		else
		{
			eflags |= EFLAGS_QUALIFIER;
			oo = ObjectAddChild(oo, OBJECT_TYPE_QUALIFIER, tt);
		}
	}
	else if (StrEq(tt->str, "signed") || StrEq(tt->str, "unsigned") || StrEq(tt->str, "short") || StrEq(tt->str, "long"))
	{
		// Check modifiers
		uint32_t newf = 0;
		if (tt->str[2] == 'g')
			newf = EFLAGS_MODIFIER_SIGNED;
		else if (tt->str[2] == 's')
			newf = EFLAGS_MODIFIER_UNSIGNED;
		else if (tt->str[2] == 'o')
			newf = EFLAGS_MODIFIER_SHORT;
		else
			newf = EFLAGS_MODIFIER_LONG;

		if (eflags & newf)
		{
			// long long int is allowed
			if ((eflags & EFLAGS_MODIFIER_LONG) && !(eflags & (EFLAGS_MODIFIER_LONG_LONG | EFLAGS_DOUBLE)))
			{
				eflags |= newf;
				oo = ObjectAddChild(oo, OBJECT_TYPE_MODIFIER, tt);
			}
			else
			{
				oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
				oo->info = _T StrDup("Cannot apply the same modifier twice");
				s = STATE_IDLE;
			}
		}
		else if (eflags & EFLAGS_USER_DEFINED_DATATYPE)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot apply modifiers to user defined datatypes");
			s = STATE_IDLE;
		}
		else if ((eflags & EFLAGS_VOID) && newf)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot apply modifiers to void datatype");
			s = STATE_IDLE;
		}
		else if ((eflags & EFLAGS_CHAR) && (newf & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot apply modifier short nor long to char datatype");
			s = STATE_IDLE;
		}
		else if ((eflags & EFLAGS_FLOAT) && newf)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot apply modifiers to float datatype");
			s = STATE_IDLE;
		}
		else if ((eflags & EFLAGS_DOUBLE) && (newf & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SIGNED)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot apply modifiers short, unsigned not signed to double datatype");
			s = STATE_IDLE;
		}
		else if (((eflags & EFLAGS_MODIFIER_UNSIGNED) && (newf & EFLAGS_MODIFIER_SIGNED)) ||
				((eflags & EFLAGS_MODIFIER_SIGNED) && (newf & EFLAGS_MODIFIER_UNSIGNED)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot apply signed and unsigned modifiers at the same time");
			s = STATE_IDLE;
		}
		else if (((eflags & EFLAGS_MODIFIER_LONG) && (newf & EFLAGS_MODIFIER_SHORT)) ||
				((eflags & EFLAGS_MODIFIER_SHORT) && (newf & EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot apply long and short modifiers at the same time");
			s = STATE_IDLE;
		}
		else
		{
			eflags |= newf;
			oo = ObjectAddChild(oo, OBJECT_TYPE_MODIFIER, tt);
		}
	}
	else if (StrEq(tt->str, "void") || StrEq(tt->str, "char") || StrEq(tt->str, "int") || StrEq(tt->str, "float") || StrEq(tt->str, "double"))
	{
		// Check basic built in datatype
		uint32_t newf = 0;
		if (tt->str[0] == 'v')
			newf = EFLAGS_VOID;
		else if (tt->str[0] == 'c')
			newf = EFLAGS_CHAR;
		else if (tt->str[0] == 'i')
			newf = EFLAGS_INT;
		else if (tt->str[0] == 'f')
			newf = EFLAGS_FLOAT;
		else
			newf = EFLAGS_DOUBLE;

		if (eflags & newf)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot specify the same basic built in datatype twice");
			s = STATE_IDLE;
		}
		else if (eflags & EFLAGS_USER_DEFINED_DATATYPE)
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot specify a basic built in datatype when it is already defined a used defined datatype");
			s = STATE_IDLE;
		}
		else if ((newf & EFLAGS_VOID) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot specify modifiers to void datatype");
			s = STATE_IDLE;
		}
		else if ((newf & EFLAGS_CHAR) && (eflags & (EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot specify short nor long modifiers to char datatype");
			s = STATE_IDLE;
		}
		else if ((newf & EFLAGS_FLOAT) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT | EFLAGS_MODIFIER_LONG)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot specify modifiers to float datatype");
			s = STATE_IDLE;
		}
		else if ((newf & EFLAGS_DOUBLE) && (eflags & (EFLAGS_MODIFIER_SIGNED | EFLAGS_MODIFIER_UNSIGNED | EFLAGS_MODIFIER_SHORT)))
		{
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Cannot specify signed, unsigned nor short modifiers to double datatype");
			s = STATE_IDLE;
		}
		else
		{
			eflags |= newf;
			oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_PRIMITIVE, tt);
		}
	}
	else if (StrEq(tt->str, "union") || StrEq(tt->str, "enum") || StrEq(tt->str, "struct"))
	{
		throw "TODO";

		// Possible datatype definition, variable definition, function definition
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_UNKNOWN, tt);
	}
	else if (StrEq(tt->str, "*"))
	{
		// Remove qualifier restriction and add pointer
		eflags &= ~EFLAGS_QUALIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_POINTER, tt);
	}
	else if (tt->type == CPARSER_TOKEN_TYPE_IDENTIFIER)
	{
		// User datatype/variable/function identifier
		if (eflags & DATATYPE_DEFINED_FLAGS)
		{
			// Identifier, so end datatype and add an identifier to parent
			oo = oo->parent;
			oo = ObjectAddChild(oo, OBJECT_TYPE_IDENTIFIER, tt);
			s = STATE_IDENTIFIER;
		}
		else
		{
			// Datatype not defined, so add user defined datatype identifier
			eflags |= EFLAGS_USER_DEFINED_DATATYPE;
			oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_USER_DEFINED, tt);
		}
	}
	else
	{
		// Unexpected token
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
		oo->info = _T StrDup("Unexpected token");
		s = STATE_IDLE;
	}

	oo = oo->parent;

	return oo;
}

object_s *cparser::StateIdentifierProcessToken(object_s *oo, states_e &s, token_s *tt)
{
	if (StrEq(tt->str, ";"))
	{
		// The last two children of this object are part of a variable (datatype and identifier),
		// so move them below a variable
		throw "REWORK ";

		oo = ObjectAddChild(oo, OBJECT_TYPE_VARIABLE, tt);
		oo->children[0] = oo->parent->children[oo->parent->children_count - 3];	// Move datatype
		oo->children[1] = oo->parent->children[oo->parent->children_count - 2];	// Move identifier
		oo->parent->children[oo->parent->children_count - 3] = oo;				// Move back variable into parent childrens'
		oo->parent->children_count -= 2;  										// Now parent has two children less
		oo = oo->parent;
		s = STATE_IDLE;
	}
	else if (StrEq(tt->str, "["))
	{
		// Array variable identifier
		oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_DEFINITION, tt);
		oo = ObjectAddChild(oo, OBJECT_TYPE_OPEN_SQ_BRACKET, tt);
		oo = oo->parent;		// return to array definition
		s = STATE_ARRAY_DEFINITION;
	}
	else if (StrEq(tt->str, "("))
	{
		// Funtion identifier
		throw "TODO";
	}
	else if (StrEq(tt->str, "{"))
	{
		// User defined union, enum or struct identifier
		throw "TODO";
	}
	else if (StrEq(tt->str, "="))
	{
		// Initialization: Initial value assignation
		oo = ObjectAddChild(oo, OBJECT_TYPE_INITIALIZATION, tt);
		oo = ObjectAddChild(oo, OBJECT_TYPE_ASSIGNATION, tt);
		oo = oo->parent;		// return to array definition
		s = STATE_INITIALIZATION;
	}
	else
	{
		// Unexpected token after identifier
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
		oo->info = _T StrDup("Unexpected token after identifier");
		oo = oo->parent;
		s = STATE_IDLE;
	}

	return oo;
}

object_s *cparser::StateInitializationProcessToken(object_s *oo, states_e &s, token_s *tt)
{
	static int32_t array_nesting_level = 0;

	if (oo->type != OBJECT_TYPE_ARRAY_ITEM)
	{
		array_nesting_level = 0;
	}

	if (StrEq(tt->str, "{"))
	{
		// Array initialization data
		oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_DATA, tt);
		oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_ITEM, tt);
		array_nesting_level++;
	}
	else if (StrEq(tt->str, "}"))
	{
		array_nesting_level--;

		if (array_nesting_level >= 0)
		{
			oo = oo->parent;										// Return to array data
			oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_ITEM, tt);	// Add new array item
			oo = oo->parent;
		}
		else
		{
			// Unexpected close bracket
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Unexpected close bracket");
			oo = oo->parent;
			s = STATE_IDLE;
		}
	}
	else if (StrEq(tt->str, ","))
	{
		// Add new array item
		oo = oo->parent;										// Return to array data
		oo = ObjectAddChild(oo, OBJECT_TYPE_ARRAY_ITEM, tt);	// Add new array item
	}
	else if (StrEq(tt->str, ";"))
	{
		// End of array definition
		throw "TODO";

		if (array_nesting_level == 0)
		{

		}
		else
		{

		}
	}
	else
	{
		// Expression -> Add expression tokens
		oo = ObjectAddChild(oo, OBJECT_TYPE_EXPRESSION_TOKEN, tt);	// Add new expression
		oo = oo->parent;											// Return to array item
	}

	return oo;
}

object_s *cparser::StateArrayDefinitionProcessToken(object_s *oo, states_e &s, token_s *tt)
{
	if (StrEq(tt->str, "["))
	{
		// Unexpected open square bracket
		oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
		oo->info = _T StrDup("Unexpected open square bracket");
		oo = oo->parent;
		s = STATE_IDLE;
	}
	else if (StrEq(tt->str, "]"))
	{
		if (oo->type == OBJECT_TYPE_ARRAY_DEFINITION)
		{
			// Close bracket, so return to identifier state
			oo = ObjectAddChild(oo, OBJECT_TYPE_CLOSE_SQ_BRACKET, tt);
			oo = oo->parent;	// return to array definition
			oo = oo->parent;	// return to array definition parent
			s = STATE_IDENTIFIER;
		}
		else
		{
			// Unexpected token after identifier
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Unexpected close square bracket");
			oo = oo->parent;
			s = STATE_IDLE;
		}
	}
	else if (StrEq(tt->str, "("))
	{
		oo = ObjectAddChild(oo, OBJECT_TYPE_EXPRESSION, tt);
		oo = ObjectAddChild(oo, OBJECT_TYPE_OPEN_PARENTHESYS, tt);
		oo = oo->parent;		// return to array definition
	}
	else if (StrEq(tt->str, ")"))
	{
		if (oo->parent->type == OBJECT_TYPE_EXPRESSION)
		{
			// Close bracket, so return to identifier state
			oo = ObjectAddChild(oo, OBJECT_TYPE_CLOSE_PARENTHESYS, tt);
			oo = oo->parent;	// return to expression
			oo = oo->parent;	// return to expression parent
		}
		else
		{
			// Unexpected token after identifier
			oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, tt);
			oo->info = _T StrDup("Unexpected close parenthesys");
			oo = oo->parent;
			s = STATE_IDLE;
		}
	}
	else
	{
		// Digest expression token
		oo = ObjectAddChild(oo, OBJECT_TYPE_EXPRESSION_TOKEN, tt);
		oo = oo->parent;	// return to expression
	}

	return oo;
}

object_s *cparser::Parse(object_s *oo)
{
	FILE *f;
	token_s tt = { CPARSER_TOKEN_TYPE_INVALID, 0, 0, { 0 } };

	// Open file
	if (IsCSourceFilename(filename))
	{
		// Open source file
		f = fopen(_t filename, "rb");
	}
	else
	{
		// Open header file
		f = paths->OpenFile(filename, _T "rb");
	}

	// Check file exists
	if (f == NULL)
		throw "cparser::Parse cannot open source file";

	// Create root parse object
	if (oo == NULL)
	{
		oo = ObjectAddChild(NULL, IsCHeaderFilename(filename) ? OBJECT_TYPE_HEADER_FILE : OBJECT_TYPE_SOURCE_FILE, NULL);
		oo->row = 0;
		oo->column = 0;
		oo->data = _T StrDup(filename);
	}

	// Process tokens from file
	states_e s = STATE_IDLE;
	uint32_t flags = 0;
	while (TokenNext(f, &tt, flags))
	{
		// Reset flags after read
		flags = 0;

		// Gently printing
		printf("R%d, C%d, %d:%s\n", tt.row, tt.column, tt.type, tt.str);

		// Process tokens
		if (tt.type == CPARSER_TOKEN_TYPE_C_COMMENT) {
			oo = ObjectAddChild(oo, OBJECT_TYPE_C_COMMENT, &tt);
			oo = oo->parent;
		}
		else if (tt.type == CPARSER_TOKEN_TYPE_CPP_COMMENT) {
			oo = ObjectAddChild(oo, OBJECT_TYPE_CPP_COMMENT, &tt);
			oo = oo->parent;
		}
		else
		{
			// Check new tokens
			if (s == STATE_IDLE)
			{
				if (tt.type == CPARSER_TOKEN_TYPE_SINGLE_CHAR)
				{
					if (tt.str[0] == '#')
					{
						s = STATE_PREPROCESSOR;
						oo = ObjectAddChild(oo, OBJECT_TYPE_PREPROCESSOR_DIRECTIVE, &tt);
					}
					else
					{
						throw "TODO";
					}
				}
				else if (tt.type == CPARSER_TOKEN_TYPE_IDENTIFIER)
				{
					// New unclasiffied identifier
					s = STATE_DATATYPE;
					oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, NULL);

					// Add token to datatype declaration or definition
					oo = StateDatatypeProcessToken(oo, s, &tt);
				}
				else
				{
					throw "TODO";
				}
			}
			else if (s == STATE_PREPROCESSOR)
			{
				if (StrEq(tt.str, "include"))
				{
					s = STATE_INCLUDE;
					oo = ObjectAddChild(oo, OBJECT_TYPE_INCLUDE, &tt);
					flags = CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME;
				}
				else if (StrStr(tt.str, "define") == tt.str)
				{
					s = STATE_DEFINE;
					oo = ObjectAddChild(oo, OBJECT_TYPE_DEFINE, &tt);
					flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
				}
				else if (StrEq(tt.str, "pragma"))
				{
					s = STATE_PRAGMA;
				}
				else
				{
					// Unexpected token
					oo = ObjectAddChild(oo, OBJECT_TYPE_ERROR, &tt);
					oo->info = _T StrDup("Unexpected preprocessor directive");
					oo = oo->parent;
					s = STATE_IDLE;
				}
			}
			else if (s == STATE_INCLUDE)
			{
				s = STATE_IDLE;

				// Add include filename
				oo = ObjectAddChild(oo, OBJECT_TYPE_INCLUDE, &tt);
				oo = oo->parent;	// Return from include filename
				oo = oo->parent;	// Return from include word
				oo = oo->parent; 	// Return from preprocessor directive
			}
			else if (s == STATE_DEFINE)
			{
				s = STATE_DEFINE_IDENTIFIER;

				// Add define literal
				oo = ObjectAddChild(oo, OBJECT_TYPE_DEFINE, &tt);
				flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_LITERAL;
			}
			else if (s == STATE_DEFINE_IDENTIFIER)
			{
				s = STATE_IDLE;

				// Add define literal
				oo = ObjectAddChild(oo, OBJECT_TYPE_DEFINE, &tt);
				oo = oo->parent;	// Return from define identifier
				oo = oo->parent;	// Return from define
				oo = oo->parent;	// Return from define literal
				oo = oo->parent; 	// Return from preprocessor directive
			}
			else if (s == STATE_DATATYPE)
			{
				// Add token to datatype declaration or definition
				oo = StateDatatypeProcessToken(oo, s, &tt);
			}
			else if (s == STATE_IDENTIFIER)
			{
				oo = StateIdentifierProcessToken(oo, s, &tt);
			}
			else if (s == STATE_ARRAY_DEFINITION)
			{
				oo = StateArrayDefinitionProcessToken(oo, s, &tt);
			}
			else
			{
				throw "TODO";
			}
		}

	}

	return oo;
}


} /* namespace cparser */
