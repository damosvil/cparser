/*
 * cparsertools.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "cparsertools.h"


const bool StrEq(const char *u, const char *v)
{
	return strcmp(u, v) == 0;
}

bool IsCHeaderFilename(const uint8_t *filename)
{
	uint32_t len;

	if (!filename)
		return false;

	len = strlen(_t filename);

	if (len < 3 || filename[len - 2] != '.' || filename[len - 1] != 'h')
		return false;

	return true;
}

bool IsCSourceFilename(const uint8_t *filename)
{
	uint32_t len;

	if (!filename)
		return false;

	len = strlen(_t filename);

	if (len < 3 || filename[len - 2] != '.' || filename[len - 1] != 'c')
		return false;

	return true;
}

void AddToPtrArray(void *data, void ***p_array, uint32_t *p_size, uint32_t *p_count)
{
	// Check array is full
	if (*p_count == *p_size)
	{
		// Create new array and new size
		uint32_t ss = *p_size + ARRAY_GROWTH_SPEED;
		void **aa = malloc(sizeof(void **) * ss);

		// Copy old array into the new one if the old one exists
		if (*p_array != NULL)
		{
			memcpy(aa, *p_array, sizeof(void *) * (*p_count));
			free(*p_array);
		}

		// Update array and size variables
		*p_array = aa;
		*p_size = ss;
	}

	(*p_array)[*p_count] = data;
	*p_count = *p_count + 1;
}
