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
#include "cparsertokenizer.h"
#include "cparser.h"

static const char *keywords[] = {
	"auto", "break", "case", "char",
	"const", "continue", "default", "do",
	"double", "else", "enum", "extern",
	"float", "for", "goto", "if",
	"int", "long", "register", "return",
	"short", "signed", "sizeof", "static",
	"struct", "switch", "typedef", "union",
	"unsigned", "void", "volatile", "while",
	NULL
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

	// Create parsing object in case it doesn't exist
	if (oo == NULL)
	{
		oo = AddChild(NULL, IsCHeaderFilename(filename) ? OBJECT_TYPE_HEADER_FILE : OBJECT_TYPE_SOURCE_FILE, NULL);
	}

	// Process tokens from file
	while (cparser_tokenizer::NextToken(f, &tt))
	{
		printf("R%d, C%d, %d:%s\n", tt.row, tt.column, tt.type, tt.str);

		switch (tt.type)
		{

		case CPARSER_TOKEN_TYPE_C_COMMENT:
			oo = AddChild(oo, OBJECT_TYPE_C_COMMENT, &tt);
			oo = oo->parent;
			break;

		case CPARSER_TOKEN_TYPE_CPP_COMMENT:
			oo = AddChild(oo, OBJECT_TYPE_CPP_COMMENT, &tt);
			oo = oo->parent;
			break;

		default:
			break;

		}
	}

	return oo;
}


} /* namespace cparser */
