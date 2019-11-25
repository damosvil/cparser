/*
 * cparserpaths.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cparsertools.h"
#include "cparserpaths.h"


typedef struct cparserpaths_s
{
	const uint8_t **m_paths;
	uint32_t m_paths_size;
	uint32_t m_paths_count;
} cparserpaths_t;


cparserpaths_t *CParserPathsNew(void)
{
	cparserpaths_t *res = malloc(sizeof(cparserpaths_t));

	// Initialize structure
	res->m_paths = NULL;
	res->m_paths_size = 0;
	res->m_paths_count = 0;

	return res;
}

cparserpaths_t *CParserPathsClone(cparserpaths_t *p)
{
	cparserpaths_t *res = malloc(sizeof(cparserpaths_t));

	// Copy already defined structure
	res->m_paths = (const uint8_t **) malloc(sizeof(uint8_t *) * p->m_paths_size);
	res->m_paths_size = p->m_paths_size;
	for (res->m_paths_count = 0; res->m_paths_count < p->m_paths_count; res->m_paths_count++)
		res->m_paths[res->m_paths_count] = _T strdup(_t p->m_paths[res->m_paths_count]);

	return res;
}

void CParserPathsDelete(cparserpaths_t *p)
{
	// Delete paths
	while (p->m_paths_count--)
		free((void *)p->m_paths[p->m_paths_count]);

	// Delete paths array
	free(p->m_paths);
}

void CParserPathsAddPath(cparserpaths_t *p, const uint8_t *path)
{
	if (p->m_paths_count == p->m_paths_size)
	{
		// Create new size and paths array
		uint32_t ss = p->m_paths_count + ARRAY_GROWTH_SPEED;
		const uint8_t **pp = (const uint8_t **) malloc(sizeof(uint8_t) *ss);

		// Copy to new paths array and delete the old one
		memcpy(pp, p->m_paths, sizeof(uint8_t *) * p->m_paths_count);
		free(p->m_paths);

		// Assing the new size and paths array
		p->m_paths = pp;
		p->m_paths_size = ss;
	}

	// Add the new path
	p->m_paths[p->m_paths_count++] = _T strdup(_t path);
}

uint32_t CParserPathsGetPathsCount(cparserpaths_t *p)
{
	return p->m_paths_count;
}

const uint8_t * CParserPathsGetPathByIndex(cparserpaths_t *p, uint32_t i)
{
	if (i >= p->m_paths_count)
		return NULL;

	return p->m_paths[i];
}

FILE * CParserPathsOpenFile(const cparserpaths_t *p, const uint8_t *filename, const uint8_t *mode)
{
	char *pc;
	uint32_t lp, lf;
	FILE *f;

	// Check filename
	if (!filename)
		return NULL;

	for (uint32_t i = 0; (f == NULL) && (i < p->m_paths_count); i++)
	{
		// Get path and filename lengths
		lp = strlen(_t p->m_paths[i]);
		lf = strlen(_t filename);

		// Get full filename path
		pc = malloc(sizeof(char) * (lp + 1 + lf + 1));
		strcpy(pc, _t p->m_paths[i]);
		strcat(pc, _t "/");
		strcat(pc, _t filename);

		// Open file;
		f = fopen(pc, _t mode);

		// Delete filename path
		free(pc);
	}

	return f;
}

void CParserPathsDeletePathByIndex(cparserpaths_t *p, uint32_t i)
{
	if (i >= p->m_paths_count)
		return;

	// Delete path string and move back the rest all
	free((void *)p->m_paths[i]);
	p->m_paths_count--;
	for (; i < p->m_paths_count; i++)
		p->m_paths[i] = p->m_paths[i + 1];
}

