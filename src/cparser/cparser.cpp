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
	m_parsers = new cparser *[0];
	m_parsers_size = 0;
	m_parsers_count = 0;

	FILE *f = fopen(_t filename, "r");
	if (!f)
		return;

	// Parse file character by character
	for (uint8_t c = fgetc(f); c != EOF; c = fgetc(f))
	{

	}

	fclose(f);
}

cparser::~cparser()
{
	while (m_parsers_count--)
		delete m_parsers[m_parsers_count];

	delete m_parsers;
}

void cparser::StoreParser(cparser *p)
{
	if (m_parsers_count == m_parsers_size)
	{
		/* Store new size and array */
		uint32_t ss = m_parsers_count + ARRAY_GROWTH_SPEED;
		cparser **pp = new cparser *[ss];

		/* Copy old array to the new one, and delete the old one */
		memcpy(pp, m_parsers, sizeof(cparser *) * m_parsers_count);
		delete m_parsers;

		/* Assign the new size and array */
		m_parsers_size = ss;
		m_parsers = pp;
	}

	/* Store parser */
	m_parsers[m_parsers_count++] = p;
}


} /* namespace cparser */
