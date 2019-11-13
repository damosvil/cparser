/*
 * cparsertools.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <string.h>
#include "cparsertools.h"

const uint8_t *StrDup(const char *s)
{
	if (s == NULL)
		throw "StrDup received invalid parameter";

	return (const uint8_t *)strdup(s);
}

const uint8_t *StrDup(const uint8_t *s)
{
	if (s == NULL)
		throw "StrDup received invalid parameter";

	return (const uint8_t *)strdup(_t s);
}

const uint32_t StrLen(const char *s)
{
	const char *t = s;

	while (*t++);

	return t - s - 1;
}

const uint32_t StrLen(const uint8_t *s)
{
	return StrLen(_t s);
}

const uint8_t *StrStr(const uint8_t *u, const char *v)
{
	return (uint8_t *)strstr((const char *)u, v);
}

const uint8_t *StrStr(const uint8_t *u, const uint8_t *v)
{
	return (uint8_t *)strstr((const char *)u, (const char *)v);
}

const bool StrEq(const uint8_t *u, const char *v)
{
	return strcmp((const char *)u, v) == 0;
}

const bool StrEq(const uint8_t *u, const uint8_t *v)
{
	return strcmp((const char *)u, (const char *)v) == 0;
}

bool IsCHeaderFilename(const uint8_t *filename)
{
	uint32_t len = StrLen(filename);

	if (len < 3 || filename[len - 2] != '.' || filename[len - 1] != 'h')
		return false;

	return true;
}

bool IsCSourceFilename(const uint8_t *filename)
{
	uint32_t len = StrLen(filename);

	if (len < 3 || filename[len - 2] != '.' || filename[len - 1] != 'c')
		return false;

	return true;
}

void AddToPtrArray(void *data, void **(&array), uint32_t &size, uint32_t &count)
{
	// Check array is full
	if (count == size)
	{
		// Create new array and new size
		uint32_t ss = size + ARRAY_GROWTH_SPEED;
		void **aa = new void *[ss];

		// Copy old array into the new one if the old one exists
		if (array != NULL)
		{
			memcpy(aa, array, sizeof(void *) * size);
			delete array;
		}

		// Update array and size variables
		array = aa;
		size = ss;
	}

	array[count++] = data;
}


