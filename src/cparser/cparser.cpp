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
	current_object = CreateChildren(NULL, OBJECT_TYPE_SOURCE_FILE);

	// Open file
	FILE *f = fopen(_t filename, "rb");
	if (!f)
		throw "cparser Could not open file";

	// Parse file
	for (uint16_t b = fgetc(f); b != EOF; b = fgetc(f))
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
		if (buffer_length == 0 && (b == ' ' || b == '\t' || b == '\r' || b == '\n'))
			continue;

		switch (current_object->type)
		{

		case OBJECT_TYPE_SOURCE_FILE:
			if (buffer_length == 1 && buffer[0] == '/' && b == '*')
			{
				current_object = CreateChildren(current_object, OBJECT_TYPE_C_COMMENT);
			}
			break;

		case OBJECT_TYPE_C_COMMENT:
			break;

		default:
			throw "cparser object type unknown";
			break;

		}

		// Add byte to buffer
		buffer[buffer_length++] = (uint8_t)b;
	}

	fclose(f);
}

cparser::~cparser()
{
}

cparser::object_s *cparser::CreateChildren(object_s *p, cparser::object_type_e t)
{
	cparser::object_s *o = new cparser::object_s;

	// Initialize new object
	o->type = t;
	o->parent = p;
	o->children = new cparser::object_s *[0];
	o->children_size = 0;
	o->children_count = 0;
	o->data = NULL;

	// Add object to parent
	AddToPtrArray<object_s *>(o, o->children, o->children_size, o->children_count);

	// Return children
	return o;
}

} /* namespace cparser */
