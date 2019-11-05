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
#include "cparserblock.h"
#include "cparser.h"


namespace cparser {

cparser::cparser(const uint8_t *filename, const cparser_paths *paths)
{
	const uint8_t **terminators = { NULL };

	// Open file
	FILE *f = fopen(_t filename, "rb");
	if (!f)
		throw "cparser Could not open file";

	block = new cparser_block(f, filename, 1, 1, terminators);

	fclose(f);
}

cparser::~cparser()
{
	delete block;
}

} /* namespace cparser */
