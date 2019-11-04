/*
 * cparserpaths.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <string.h>
#include "cparsertools.h"
#include "cparserpaths.h"


#define PATHS_GROTH_SPEED 		10


namespace cparser {

cparser_paths::cparser_paths()
{
	m_paths = new uint8_t *[PATHS_GROTH_SPEED];
	m_paths_size = PATHS_GROTH_SPEED;
	m_paths_count = 0;
}

cparser_paths::~cparser_paths()
{
	// Delete paths
	for (uint32_t i = 0; i < m_paths_count; i++)
		delete m_paths[i];

	// Delete paths array
	delete m_paths;
}

void cparser_paths::AddPath(const uint8_t *path)
{
	if (m_paths_count == m_paths_size)
	{
		// Create new size and paths array
		uint32_t ss = m_paths_count + PATHS_GROTH_SPEED;
		uint8_t **pp = new uint8_t *[ss];

		// Copy to new paths array and delete the old one
		memcpy(pp, m_paths, sizeof(uint8_t *) * m_paths_count);
		delete m_paths;

		// Assing the new size and paths array
		m_paths = pp;
		m_paths_size = ss;
	}

	// Add the new path
	m_paths[m_paths_count++] = _T strdup(_t path);
}

} /* namespace cparser */
