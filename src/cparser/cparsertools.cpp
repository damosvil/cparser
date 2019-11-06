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

template<typename T>
void AddToPtrArray(T data, T *(&array), uint32_t &size, uint32_t &count)
{
	if (count == size)
	{
		uint32_t ss = size + ARRAY_GROWTH_SPEED;
		T *aa = new T[ss];

		memcpy(aa, array, sizeof(T) * size);
		delete array;

		array = aa;
		size = ss;
	}

	array[count++] = data;
}


