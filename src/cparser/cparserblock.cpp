/*
 * cparsercomment.cpp
 *
 *  Created on: 5 nov. 2019
 *      Author: iso9660
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <cparsertools.h>
#include <cparserblock.h>


namespace cparser {

cparser_block::cparser_block(FILE *f, const uint8_t *file, uint32_t row, uint32_t column, const uint8_t **terminators)
{
	this->file = f;
	this->filename = StrDup(file);
	this->row = row;
	this->column = column;
	this->terminators = terminators;

	data = new uint8_t[0];
	data_size = 0;
	data_count = 0;

	blocks = new cparser_block *[0];
	blocks_size = 0;
	blocks_count = 0;

	// Parse file character by character
	for (uint16_t c = fgetc(f); c != EOF; c = fgetc(f))
		DigestByte(c);
}

cparser_block::~cparser_block()
{
	delete filename;
	delete data;

	while (blocks_count--)
		delete blocks[blocks_count];
}

void cparser_block::AppendDataByte(uint8_t b)
{
	if ((data_count + 1) >= data_size)
	{
		// Create new size and data array
		uint32_t ss = data_size + ARRAY_GROWTH_SPEED;
		uint8_t *dd = new uint8_t[ss];

		// Copy existing data to new array and delete the old one
		memcpy(dd, data, sizeof(uint8_t) * data_size);
		delete data;

		// Update data array and size
		data_size = ss;
		data = dd;
	}

	// Add byte and close string
	data[data_count++] = b;
	data[data_count] = 0;
}

void cparser_block::AppendBlock(cparser_block *b)
{
	if ((blocks_count + 1) >= blocks_size)
	{
		// Create new size and data array
		uint32_t ss = blocks_size + ARRAY_GROWTH_SPEED;
		cparser_block **bb = new cparser_block *[ss];

		// Copy existing data to new array and delete the old one
		memcpy(bb, blocks, sizeof(cparser_block) * blocks_size);
		delete blocks;

		// Update data array and size
		blocks_size = ss;
		blocks = bb;
	}

	// Add byte and close string
	blocks[blocks_count++] = b;
	blocks[blocks_count] = 0;
}

bool cparser_block::ByteIn(uint8_t b, const uint8_t *set)
{
	while (*set)
		if (*set++ == b)
			return true;

	return false;
}

bool cparser_block::DigestByte(uint8_t b)
{
	// Count row and column
	if (b == '\n')
	{
		row++;
		column = 1;
	}
	else
	{
		column++;
	}

	// Skip empty characters at the beginning
	if (data_count == 0 && ByteIn(b, _T " \t\r\n"))
		return true;

	if (data_count > 0)
	{
		if (data_count == 1 && data[0] == '/' && b == '*')
		{
			const uint8_t *tt[] = { _T "*/", NULL };
			AppendBlock(new cparser_block(file, filename, row, column, tt));
		}
		else if (data_count == 1 && data[0] == '/' && b == '/')
		{
			const uint8_t *tt[] = { _T "\r", _T "\n", NULL };
			AppendBlock(new cparser_block(file, filename, row, column, tt));
		}
	}

	// Append byte to the current block
	AppendDataByte(b);

	// Check terminators
	const uint8_t **tt = terminators;
	while (*tt)
	{
		uint32_t ttlen = strlen(_t *tt);

		if ((data_count >= ttlen) && strcmp(_t *tt, _t (data + data_count - ttlen)) == 0)
		{
			return false;
		}
		tt++;
	}



	return true;
}



} /* namespace cgl */
