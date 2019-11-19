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

struct dictionary
{
	pair **pairs;
	uint32_t pairs_size;
	uint32_t pairs_count;
};

dictionary * DictionaryInit(void)
{
	dictionary *d = new dictionary;

	d->pairs = NULL;
	d->pairs_size = 0;
	d->pairs_count = 0;

	return d;
}

static int DictionaryKeyCompare(const void * ka, const void *kb)
{
	pair *kka = (pair *)ka;
	pair *kkb = (pair *)kb;

	return strcmp(_t kka->key, _t kkb->key);
}

void DictionaryRemoveKey(dictionary *d, const uint8_t *key)
{
	pair *p = (pair *) bsearch(key, d->pairs, d->pairs_count, sizeof(pair *), DictionaryKeyCompare);
	if (!p) return;

	uint32_t ix = &p - d->pairs;	// Keys are disposed in a vector

	// Decrease pairs, and bulk back copy all remaining pointers
	d->pairs_count--;
	memcpy(d->pairs + ix, d->pairs + ix + 1, d->pairs_count - ix);
}

void DictionarySetKeyValue(dictionary *d, const uint8_t *key, const void *value)
{
	pair *p = (pair *) bsearch(key, d->pairs, d->pairs_count, sizeof(pair *), DictionaryKeyCompare);

	// Add pair if no key found
	if (!p)
	{
		// Increase pairs size
		if (d->pairs_count == d->pairs_size)
		{
			// Create new size and array
			uint32_t ss = d->pairs_size + 100;
			pair **pp = new pair *[ss];

			// Copy old data into new array and delete old data
			memcpy(pp, d->pairs, d->pairs_count);
			delete d->pairs;

			// Update dictionary pairs and size
			d->pairs = pp;
			d->pairs_size = ss;
		}

		// Create and add new pair
		pair *p = new pair;
		p->key = StrDup(key);
		d->pairs[d->pairs_count++] = p;

		// Sort the whole thing
		qsort(d->pairs, d->pairs_count, sizeof(pair *), DictionaryKeyCompare);
	}

	// Update pair value
	p->value = value;
}

const void * DictionaryGetKeyValue(dictionary *d, const uint8_t *key)
{
	pair *p = (pair *) bsearch(key, d->pairs, d->pairs_count, sizeof(pair *), DictionaryKeyCompare);

	return p ? p->value : NULL;
}

uint32_t DictionaryGetKeyCount(dictionary *d)
{
	return d->pairs_count;
}

const uint8_t * DictionaryGetKeyByIndex(dictionary *d, uint32_t ix)
{
	if (ix >= d->pairs_count)
		return NULL;

	return d->pairs[ix]->key;
}

const void * DictionaryGetValueByIndex(dictionary *d, uint32_t ix)
{
	if (ix >= d->pairs_count)
		return NULL;

	return d->pairs[ix]->value;
}

void DictionaryDelete(dictionary *d)
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
