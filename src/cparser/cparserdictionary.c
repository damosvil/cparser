/*
 * cparserdictionary.c
 *
 *  Created on: 19/11/2019
 *      Author: blue
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cparsertools.h"
#include "cparserdictionary.h"


typedef struct pair_s
{
	const uint8_t *key;
	const void *value;
} pair_t;

typedef struct cparserdictionary_s
{
	pair_t **pairs;
	int32_t pairs_size;
	int32_t pairs_count;
} cparserdictionary_t;


static int DictionaryKeyCompare(const void * ka, const void * kb)
{
	pair_t *kka = *(pair_t **)ka;
	pair_t *kkb = *(pair_t **)kb;

	return strcmp(_t kka->key, _t kkb->key);
}

cparserdictionary_t * DictionaryNew(void)
{
	cparserdictionary_t *d = malloc(sizeof(cparserdictionary_t));

	d->pairs = NULL;
	d->pairs_size = 0;
	d->pairs_count = 0;

	return d;
}

void DictionaryRemoveKey(cparserdictionary_t *d, const uint8_t *key)
{
	pair_t pp = { key, NULL };
	pair_t *ppp = &pp;
	pair_t **p = bsearch(&ppp, d->pairs, d->pairs_count, sizeof(pair_t *), DictionaryKeyCompare);
	if (!p) return;

	uint32_t ix = p - d->pairs;	// Keys are disposed in a linear array

	// Decrease pairs, and bulk back copy all remaining pointers
	d->pairs_count--;
	memcpy(d->pairs + ix, d->pairs + ix + 1, d->pairs_count - ix);
}

void DictionarySetKeyValue(cparserdictionary_t *d, const uint8_t *key, const void *value)
{
	int ix = 0;
	pair_t pp = { key, NULL };
	pair_t *ppp = &pp;
	pair_t **p = bsearch(&ppp, d->pairs, d->pairs_count, sizeof(pair_t *), DictionaryKeyCompare);
	pair_t *q;

	// Add pair if no key found
	if (!p)
	{
		// Increase pairs size if pairs array is full
		if ( d->pairs_count == d->pairs_size)
		{
			// Create new size and array
			uint32_t ss = d->pairs_size + 100;
			pair_t **pp = malloc(sizeof(pair_t) * ss);

			// Copy old data into new array and delete old data
			memcpy(pp, d->pairs, sizeof(pair_t *) * d->pairs_count);
			free(d->pairs);

			// Update dictionary pairs and size
			d->pairs = pp;
			d->pairs_size = ss;
		}

		// Create a new pair
		q = malloc(sizeof(pair_t));
		q->key = _T strdup(_t key);
		q->value = value;

		// Look for insert index
		if (d->pairs_count > 0)
		{
			ix = d->pairs_count / 2;
			int32_t delta = ix / 2;

			// Stride over pairs
			while (delta)
			{
				if (strcmp(_t key, _t d->pairs[ix]->key) < 0)
					ix -= delta;
				else
					ix += delta;

				delta /= 2;
			}

			// Step over pairs
			while (ix > 0 && strcmp(_t key, _t d->pairs[ix - 1]->key) < 0)
				ix--;

			// Step over pairs
			while (ix < d->pairs_count && strcmp(_t key, _t d->pairs[ix]->key) > 0)
				ix++;

			// Move forward all pointers
			for (int i = d->pairs_count; i > ix; i--)
				d->pairs[i] = d->pairs[i - 1];
		}

		// Insert the new pair and increase pairs count
		d->pairs[ix] = q;
		d->pairs_count++;
	}
	else
	{
		// Update value in found key
		(*p)->value = value;
	}
}

bool DictionaryExistsKey(cparserdictionary_t *d, const uint8_t *key)
{
	pair_t pp = { key, NULL };
	pair_t *ppp = &pp;
	pair_t **p;

	if (d == NULL)
		return NULL;

	p = bsearch(&ppp, d->pairs, d->pairs_count, sizeof(pair_t *), DictionaryKeyCompare);

	return p != NULL;
}

const void * DictionaryGetKeyValue(cparserdictionary_t *d, const uint8_t *key)
{
	pair_t pp = { key, NULL };
	pair_t *ppp = &pp;
	pair_t **p = bsearch(&ppp, d->pairs, d->pairs_count, sizeof(pair_t *), DictionaryKeyCompare);

	return p ? (*p)->value : NULL;
}

uint32_t DictionaryGetKeyCount(cparserdictionary_t *d)
{
	return d->pairs_count;
}

const uint8_t * DictionaryGetKeyByIndex(cparserdictionary_t *d, uint32_t ix)
{
	if (ix >= d->pairs_count)
		return NULL;

	return d->pairs[ix]->key;
}

const void * DictionaryGetValueByIndex(cparserdictionary_t *d, uint32_t ix)
{
	if (ix >= d->pairs_count)
		return NULL;

	return d->pairs[ix]->value;
}

void DictionaryDelete(cparserdictionary_t *d)
{
	// Delete keys and pairs
	while (d->pairs_count--)
	{
		free((void *)d->pairs[d->pairs_count]->key);
		free((void *)d->pairs[d->pairs_count]);
	}

	// Delete dictionary
	free(d);
}
