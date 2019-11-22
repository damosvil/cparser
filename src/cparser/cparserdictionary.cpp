/*
 * cparserdictionary.c
 *
 *  Created on: 19/11/2019
 *      Author: blue
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cparsertools.h"
#include "cparserdictionary.h"


namespace cparser
{

struct pair
{
	const uint8_t *key;
	const void *value;
};

struct dictionary_s
{
	pair **pairs;
	int32_t pairs_size;
	int32_t pairs_count;
};

static int DictionaryKeyCompare(const void * ka, const void * kb)
{
	pair *kka = *(pair **)ka;
	pair *kkb = *(pair **)kb;

	return strcmp(_t kka->key, _t kkb->key);
}

dictionary_s * DictionaryNew(void)
{
	dictionary_s *d = new dictionary_s;

	d->pairs = NULL;
	d->pairs_size = 0;
	d->pairs_count = 0;

	return d;
}

void DictionaryRemoveKey(dictionary_s *d, const uint8_t *key)
{
	pair pp = { key, NULL };
	pair *ppp = &pp;
	pair *p = (pair *) bsearch(&ppp, d->pairs, d->pairs_count, sizeof(pair *), DictionaryKeyCompare);
	if (!p) return;

	uint32_t ix = &p - d->pairs;	// Keys are disposed in a linear array

	// Decrease pairs, and bulk back copy all remaining pointers
	d->pairs_count--;
	memcpy(d->pairs + ix, d->pairs + ix + 1, d->pairs_count - ix);
}

void DictionarySetKeyValue(dictionary_s *d, const uint8_t *key, const void *value)
{
	int ix = 0;
	pair pp = { key, NULL };
	pair *ppp = &pp;
	pair *p = (pair *) bsearch(&ppp, d->pairs, d->pairs_count, sizeof(pair *), DictionaryKeyCompare);

	// Add pair if no key found
	if (!p)
	{
		// Increase pairs size if pairs array is full
		if (d->pairs_count == d->pairs_size)
		{
			// Create new size and array
			uint32_t ss = d->pairs_size + 100;
			pair **pp = new pair *[ss];

			// Copy old data into new array and delete old data
			memcpy(pp, d->pairs, sizeof(pair *) * d->pairs_count);
			delete d->pairs;

			// Update dictionary pairs and size
			d->pairs = pp;
			d->pairs_size = ss;
		}

		// Create a new pair
		p = new pair;
		p->key = StrDup(key);

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
		d->pairs[ix] = p;
		d->pairs_count++;
	}

	// Update / set pair value
	p->value = value;
}

const void * DictionaryGetKeyValue(dictionary_s *d, const uint8_t *key)
{
	pair pp = { key, NULL };
	pair *ppp = &pp;
	pair *p = (pair *) bsearch(&ppp, d->pairs, d->pairs_count, sizeof(pair *), DictionaryKeyCompare);

	return p ? p->value : NULL;
}

uint32_t DictionaryGetKeyCount(dictionary_s *d)
{
	return d->pairs_count;
}

const uint8_t * DictionaryGetKeyByIndex(dictionary_s *d, uint32_t ix)
{
	if (ix >= d->pairs_count)
		return NULL;

	return d->pairs[ix]->key;
}

const void * DictionaryGetValueByIndex(dictionary_s *d, uint32_t ix)
{
	if (ix >= d->pairs_count)
		return NULL;

	return d->pairs[ix]->value;
}

void DictionaryDelete(dictionary_s *d)
{
	// Delete keys and pairs
	while (d->pairs_count--)
	{
		delete d->pairs[d->pairs_count]->key;
		delete d->pairs[d->pairs_count];
	}

	// Delete dictionary
	delete d;
}


}
