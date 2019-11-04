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



namespace cparser {

cparser_paths::cparser_paths()
{
	m_paths = new uint8_t *[0];
	m_paths_size = 0;
	m_paths_count = 0;
}

cparser_paths::~cparser_paths()
{
	// Delete paths
	while (m_paths_count--)
		delete m_paths[m_paths_count];

	// Delete paths array
	delete m_paths;
}

void cparser_paths::AddPath(const uint8_t *path)
{
	if (m_paths_count == m_paths_size)
	{
		// Create new size and paths array
		uint32_t ss = m_paths_count + ARRAY_GROWTH_SPEED;
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

uint32_t cparser_paths::GetPathsCount()
{
	return m_paths_count;
}

const uint8_t * cparser_paths::GetPathByIndex(uint32_t i)
{
	if (i >= m_paths_count)
		return NULL;

	return m_paths[i];
}

void cparser_paths::DeletePathByIndex(uint32_t i)
{
	if (i >= m_paths_count)
		return;

	// Delete path string and move back the rest all
	delete m_paths[i];
	m_paths_count--;
	for (; i < m_paths_count; i++)
		m_paths[i] = m_paths[i + 1];
}


} /* namespace cparser */
