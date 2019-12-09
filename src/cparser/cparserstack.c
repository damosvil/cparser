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
	size_t item_size;
};

cparserstack_t *StackNew(size_t item_size)
{
	cparserstack_t *res;

	if (item_size == 0)
		return NULL;

	res = malloc(sizeof(cparserstack_t));

	res->count = 0;
	res->size = 0;
	res->data = NULL;
	res->item_size = item_size;

	return res;
}

void StackDelete(cparserstack_t *s)
{
	free(s);
}

void StackPush(cparserstack_t *s, const void *data)
{
	// Skip if no data
	if (s->size == 0)
		return;

	// Check data size
	if (s->size == (s->count * s->item_size))
	{
		uint32_t ss = s->size + ARRAY_GROWTH_SPEED * s->item_size;
		uint8_t *dd = malloc(sizeof(uint8_t) * ss);

		// Copy old data to newly created buffer
		if (s->data)
		{
			memcpy(dd, s->data, s->size);
			free(s->data);
		}

		// Assign new buffer and size to stack
		s->data = dd;
		s->size = ss;
	}

	// Move data to stack
	memcpy(s->data + s->count * s->item_size, data, s->item_size);

	// Increase stack count
	s->count++;
}

bool StackPop(cparserstack_t *s, void *data)
{
	// Skip if no destination buffer, not enough room or not enough data in stack
	if (data == NULL || s->count == 0)
		return false;

	// Decrease stack count
	s->count--;

	// Move data to destination address
	memcpy(data, s->data + s->count * s->item_size, s->item_size);

	return true;
}
