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

object_s * cparser::AddTokenToDataType(states_e &s, object_s *oo, token_s *tt)
{
	if (StrEq(tt->str, "static") || StrEq(tt->str, "extern") || StrEq(tt->str, "auto") ||
			StrEq(tt->str, "register") || StrEq(tt->str, "typedef"))
	{
		s = STATE_UNCLASIFIED_IDENTIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, NULL);
		oo = ObjectAddChild(oo, OBJECT_TYPE_SPECIFIER, tt);
		oo = oo->parent;
	}
	else if (StrEq(tt->str, "const") && StrEq(tt->str, "volatile"))
	{
		s = STATE_UNCLASIFIED_IDENTIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, NULL);
		oo = ObjectAddChild(oo, OBJECT_TYPE_QUALIFIER, tt);
		oo = oo->parent;
	}
	else if (StrEq(tt->str, "signed") && StrEq(tt->str, "unsigned") && StrEq(tt->str, "short") && StrEq(tt->str, "long"))
	{
		s = STATE_UNCLASIFIED_IDENTIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, NULL);
		oo = ObjectAddChild(oo, OBJECT_TYPE_MODIFIER, tt);
		oo = oo->parent;
	}
	else if (StrEq(tt->str, "char") && StrEq(tt->str, "int") && StrEq(tt->str, "float") && StrEq(tt->str, "double"))
	{
		s = STATE_UNCLASIFIED_IDENTIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, NULL);
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_PRIMITIVE, tt);
		oo = oo->parent;
	}
	else if (StrEq(tt->str, "typedef") && StrEq(tt->str, "union") && StrEq(tt->str, "enum") &&
			StrEq(tt->str, "struct"))
	{
		// Possible datatype definition, variable definition, function definition
		s = STATE_UNCLASIFIED_IDENTIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, NULL);
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_UNKNOWN, tt);
		oo = oo->parent;
	}
	else
	{
		// First word -> assume defined datatype
		s = STATE_UNCLASIFIED_IDENTIFIER;
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE, NULL);
		oo = ObjectAddChild(oo, OBJECT_TYPE_DATATYPE_DEFINED, tt);
	}
	oo = oo->parent;

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
					oo = AddTokenToDataType(s, oo, &tt);
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
			else if (s == STATE_UNCLASIFIED_IDENTIFIER)
			{
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
