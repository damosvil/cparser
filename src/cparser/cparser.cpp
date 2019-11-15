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

enum states_e
{
	STATE_IDLE,
	STATE_PREPROCESSOR,
	STATE_INCLUDE,
	STATE_DEFINE,
	STATE_DEFINE_IDENTIFIER,
	STATE_PRAGMA,
};

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

cparser::object_s *cparser::AddChild(object_s *parent, cparser::object_type_e type, token_s *token)
{
	cparser::object_s *child = new cparser::object_s;

	// Initialize new object
	child->type = type;
	child->parent = parent;
	child->children = NULL;
	child->children_size = 0;
	child->children_count = 0;
	child->info = NULL;

	// Add token data if any
	if (token)
	{
		child->row = token->row;
		child->column = token->column;
		child->data = _T StrDup(token->str);
	}

	// Add object to parent if it is not root node
	if (parent != NULL)
		AddToPtrArray(child, (void **&)parent->children, parent->children_size, parent->children_count);

	// Return children
	return child;
}

cparser::object_s *cparser::Parse(object_s *oo)
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
		oo = AddChild(NULL, IsCHeaderFilename(filename) ? OBJECT_TYPE_HEADER_FILE : OBJECT_TYPE_SOURCE_FILE, NULL);
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
			oo = AddChild(oo, OBJECT_TYPE_C_COMMENT, &tt);
			oo = oo->parent;
		}
		else if (tt.type == CPARSER_TOKEN_TYPE_CPP_COMMENT) {
			oo = AddChild(oo, OBJECT_TYPE_CPP_COMMENT, &tt);
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
						oo = AddChild(oo, OBJECT_TYPE_PREPROCESSOR_DIRECTIVE, &tt);
					}
					else
					{
						throw "TODO";
					}
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
					oo = AddChild(oo, OBJECT_TYPE_INCLUDE, &tt);
					flags = CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME;
				}
				else if (StrStr(tt.str, "define") == tt.str)
				{
					s = STATE_DEFINE;
					oo = AddChild(oo, OBJECT_TYPE_DEFINE, &tt);
					flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER;
				}
				else if (StrEq(tt.str, "pragma"))
				{
					s = STATE_PRAGMA;
				}
				else
				{
					// Unexpected token
					oo = AddChild(oo, OBJECT_TYPE_ERROR, &tt);
					oo = oo->parent;
					s = STATE_IDLE;
				}
			}
			else if (s == STATE_INCLUDE)
			{
				s = STATE_IDLE;

				// Add include filename
				oo = AddChild(oo, OBJECT_TYPE_INCLUDE, &tt);
				oo = oo->parent;	// Return from include filename
				oo = oo->parent;	// Return from include word
				oo = oo->parent; 	// Return from preprocessor directive
			}
			else if (s == STATE_DEFINE)
			{
				s = STATE_DEFINE_IDENTIFIER;

				// Add define literal
				oo = AddChild(oo, OBJECT_TYPE_DEFINE, &tt);
				flags = CPARSER_TOKEN_FLAG_PARSE_DEFINE_LITERAL;
			}
			else if (s == STATE_DEFINE_IDENTIFIER)
			{
				s = STATE_IDLE;

				// Add define literal
				oo = AddChild(oo, OBJECT_TYPE_DEFINE, &tt);
				oo = oo->parent;	// Return from define identifier
				oo = oo->parent;	// Return from define
				oo = oo->parent;	// Return from define literal
				oo = oo->parent; 	// Return from preprocessor directive
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
