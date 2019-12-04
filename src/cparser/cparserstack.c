/*
 * cparserstack.c
 *
 *  Created on: 2 dic. 2019
 *      Author: iso9660
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cparsertools.h"
#include "cparserstack.h"

struct cparserstack_s
{
	uint32_t count;
	uint32_t size;
	uint8_t *data;
};

cparserstack_t *StackNew(void)
{
	cparserstack_t *res = malloc(sizeof(cparserstack_t));

	res->count = 0;
	res->size = 0;
	res->data = NULL;

	return res;
}

void StackDelete(cparserstack_t *s)
{
	free(s);
}

void StackPushBytes(cparserstack_t *s, const void *data, size_t size)
{
	// Skip if no data
	if (data == NULL || size == 0)
		return;

	// Check data size
	if ((s->data == NULL) || (s->size < (s->count + size)))
	{
		uint32_t ss = s->size + ARRAY_GROWTH_SPEED * size;
		uint8_t *dd = malloc(sizeof(uint8_t) * ss);

		// Copy old data to newly created buffer
		if (s->data)
		{
			memcpy(dd, s->data, s->count);
			free(s->data);
		}

		// Assign new buffer and size to stack
		s->data = dd;
		s->size = ss;
	}

	// Move data to stack
	memcpy(s->data + s->count, data, size);
	s->count += size;
}

bool StackPopBytes(cparserstack_t *s, void *data, size_t size)
{
	// Skip if no destination buffer, not enough room or not enough data in stack
	if (data == NULL || size == 0 || size > s->count)
		return false;

	// Move data to destination address
	memcpy(data, s->data, size);
	s->count += size;

	return true;
}
