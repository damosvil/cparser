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
	m_paths = new const uint8_t *[0];
	m_paths_size = 0;
	m_paths_count = 0;
}

cparser_paths::cparser_paths(const cparser_paths *p)
{
	m_paths = new const uint8_t *[p->m_paths_size];
	m_paths_size = p->m_paths_size;
	for (m_paths_count = 0; m_paths_count < p->m_paths_count; m_paths_count++)
		m_paths[m_paths_count] = StrDup(p->m_paths[m_paths_count]);
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
		const uint8_t **pp = new const uint8_t *[ss];

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

uint32_t cparser_paths::GetPathsCount() const
{
	return m_paths_count;
}

const uint8_t * cparser_paths::GetPathByIndex(uint32_t i) const
{
	if (i >= m_paths_count)
		return NULL;

	return m_paths[i];
}

FILE * cparser_paths::OpenFile(const uint8_t *filename, const uint8_t *mode) const
{
	char *p;
	uint32_t lp, lf;
	FILE *f;

	// Check filename
	if (!filename)
		return NULL;

	for (uint32_t i = 0; (f == NULL) && (i < m_paths_count); i++)
	{
		// Get path and filename lengths
		lp = StrLen(m_paths[i]);
		lf = StrLen(filename);

		// Get full filename path
		p = new char[lp + 1 + lf + 1];
		strcpy(p, _t m_paths[i]);
		strcat(p, _t "/");
		strcat(p, _t filename);

		// Open file;
		f = fopen(p, _t mode);

		// Delete filename path
		delete p;
	}

	return f;
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
