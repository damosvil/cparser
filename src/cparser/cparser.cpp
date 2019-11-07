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
#include "cparser.h"


namespace cparser {

cparser::cparser(const uint8_t *filename, const cparser_paths *paths)
{
	uint8_t buffer[MAX_SENTENCE_LENGTH + 1];
	uint8_t buffer_length = 0;
	uint32_t row = 1, column = 1;
	object_s *current_object;

	// Prepare current object
	current_object = BeginChild(NULL, OBJECT_TYPE_SOURCE_FILE, row, column);
	current_object->data = StrDup(filename);

	// Open file
	FILE *f = fopen(_t filename, "rb");
	if (!f)
		throw "cparser Could not open file";

	// Parse file
	for (int16_t b = fgetc(f); b != EOF; b = fgetc(f))
	{
		// Update row and column
		if (b == '\n')
		{
			row++;
			column = 1;
		}
		else
		{
			column++;
		}

		// Skip empty characters if no object is being read at this time
		if (buffer_length == 0 && IsEmptyChar(b))
			continue;

		// Add byte to buffer
		if (buffer_length < MAX_SENTENCE_LENGTH)
		{
			buffer[buffer_length++] = (uint8_t)b;
			buffer[buffer_length] = 0;
		}

		// Parsing
		switch (current_object->type)
		{

		case OBJECT_TYPE_SOURCE_FILE:
			if (StrEq(buffer, "/*"))
			{
				// Begin child
				current_object = BeginChild(current_object, OBJECT_TYPE_C_COMMENT, row, column - buffer_length);
			}
			else if (StrEq(buffer, "//"))
			{
				// Begin child
				current_object = BeginChild(current_object, OBJECT_TYPE_CPP_COMMENT, row, column - buffer_length);
			}
			else if (StrEq(buffer, "#include"))
			{
				// Reset buffer and begin child
				current_object = BeginChild(current_object, OBJECT_TYPE_INCLUDE, row, column - buffer_length);
				buffer_length = 0;
			}
			else if (StrEq(buffer, "static") || StrEq(buffer, "extern"))
			{
				// Reset buffer and begin child
				current_object = BeginChild(current_object, OBJECT_TYPE_VARIABLE_SCOPE, row, column - buffer_length);
				current_object = EndChild(current_object, buffer);
				buffer_length = 0;
			}
			else if (StrEq(buffer, "void") || StrEq(buffer, "char") || StrEq(buffer, "int") || StrEq(buffer, "float") || StrEq(buffer, "double"))
			{
				// Reset buffer and begin child
				current_object = BeginChild(current_object, OBJECT_TYPE_VARIABLE_BASIC_TYPE, row, column - buffer_length);
				current_object = EndChild(current_object, buffer);
				buffer_length = 0;
			}
			else if (StrEq(buffer, "short") || StrEq(buffer, "long") || StrEq(buffer, "signed") || StrEq(buffer, "unsigned"))
			{
				// Reset buffer and begin child
				current_object = BeginChild(current_object, OBJECT_TYPE_VARIABLE_BASIC_TYPE_MODIFIER, row, column - buffer_length);
				current_object = EndChild(current_object, buffer);
				buffer_length = 0;
			}
			else if (IsEmptyChar(b))
			{
				// Add internal error
				current_object = BeginChild(current_object, OBJECT_TYPE_ERROR, row, column - buffer_length);
				current_object = EndChild(current_object, _T"cparser unknown expression");

				// Throw exception
				throw "cparser unknown expression";
			}
			break;

		case OBJECT_TYPE_C_COMMENT:
			if (buffer[buffer_length - 2] == '*' && buffer[buffer_length - 1] == '/')
			{
				// End child, and clear buffer
				current_object = EndChild(current_object, buffer);
				buffer_length = 0;
				buffer[buffer_length] = 0;
			}
			break;

		case OBJECT_TYPE_CPP_COMMENT:
			if (buffer[buffer_length - 1] == '\n')
			{
				// End child, and clear buffer
				current_object = EndChild(current_object, buffer);
				buffer_length = 0;
				buffer[buffer_length] = 0;
			}
			break;

		case OBJECT_TYPE_INCLUDE:
			if (buffer_length > 0 && IsEmptyChar(b))
			{
				// Remove last character
				buffer_length--;
				buffer[buffer_length] = 0;

				// Evaluate filename
				if (buffer[0] == '\"' && buffer[buffer_length - 1] == '\"')
				{
					// Add filename and return to parent
					current_object = EndChild(current_object, buffer);
				}
				else if (buffer[0] == '<' && buffer[buffer_length - 1] == '>')
				{
					// Add filename and return to parent
					current_object = EndChild(current_object, buffer);
				}
				else
				{
					// Add syntax error and return to parent
					current_object = BeginChild(current_object, OBJECT_TYPE_ERROR, row, column);
					current_object = EndChild(current_object, _T"#include expects \"FILENAME\" or <FILENAME>");
					current_object = current_object->parent;
				}

				// Clear buffer
				buffer_length = 0;
				buffer[buffer_length] = 0;
			}
			break;

		default:
			throw "cparser object type unknown";
			break;

		}
	}

	fclose(f);
}

cparser::~cparser()
{
}

cparser::object_s *cparser::BeginChild(object_s *parent, cparser::object_type_e type, uint32_t row, uint32_t column)
{
	cparser::object_s *child = new cparser::object_s;

	// Initialize new object
	child->type = type;
	child->parent = parent;
	child->row = row;
	child->column = column;
	child->children = NULL;
	child->children_size = 0;
	child->children_count = 0;
	child->data = NULL;

	// Add object to parent if it is not root node
	if (parent != NULL)
		AddToPtrArray(child, (void **&)parent->children, parent->children_size, parent->children_count);

	// Return children
	return child;
}

cparser::object_s *cparser::EndChild(object_s *c, const uint8_t *data)
{
	// Assign string data
	c->data = StrDup(data);

	// Return to parent object
	return c->parent;
}


} /* namespace cparser */
