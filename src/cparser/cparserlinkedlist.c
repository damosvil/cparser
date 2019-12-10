/*
 * cparserlinkedlist.c
 *
 *  Created on: 9 dic. 2019
 *      Author: iso9660
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "cparserlinkedlist.h"


struct cparserlinkedlist_s
{
	cparserlinkedlist_t *previous;
	cparserlinkedlist_t *next;
	void *item;
};


cparserlinkedlist_t *LinkedListNew(void *item)
{
	cparserlinkedlist_t *res = malloc(sizeof(cparserlinkedlist_t));

	res->next = NULL;
	res->previous = NULL;
	res->item = item;

	return res;
}

cparserlinkedlist_t * LinkedListDelete(cparserlinkedlist_t *l)
{
	cparserlinkedlist_t *res;
	cparserlinkedlist_t *next;
	cparserlinkedlist_t *prev;

	// Check valid
	if (l == NULL)
		return NULL;

	// Store next and previous items
	next = l->next;
	prev = l->previous;

	// Link
	if (next && prev)
	{
		next->previous = prev;
		prev->next = next;
		res = next;
	}
	else if (next)
	{
		next->previous = NULL;
		res = next;
	}
	else if (prev)
	{
		prev->next = NULL;
		res = prev;
	}
	else
	{
		res = NULL;
	}

	// Free linked list item
	free(l);

	return res;
}

cparserlinkedlist_t *LinkedListFirst(cparserlinkedlist_t *l)
{
	while (l->previous)
		l = l->previous;

	return l;
}

cparserlinkedlist_t *LinkedListLast(cparserlinkedlist_t *l)
{
	while (l->next)
		l = l->next;

	return l;
}

cparserlinkedlist_t *LinkedListPrevious(cparserlinkedlist_t *l)
{
	return l->previous;
}

cparserlinkedlist_t *LinkedListNext(cparserlinkedlist_t *l)
{
	return l->next;
}

cparserlinkedlist_t *LinkedListInsertBefore(cparserlinkedlist_t *l, void *item)
{
	cparserlinkedlist_t *new;

	if (l == NULL)
		return NULL;

	new = malloc(sizeof(cparserlinkedlist_t));
	new->item = item;
	new->previous = l->previous;
	new->next = l;

	l->previous = new;

	return new;
}

cparserlinkedlist_t *LinkedListInsertAfter(cparserlinkedlist_t *l, void *item)
{
	cparserlinkedlist_t *new;

	if (l == NULL)
		return NULL;

	new = malloc(sizeof(cparserlinkedlist_t));
	new->item = item;
	new->previous = l;
	new->next = l->next;

	l->next = new;

	return new;
}

void *LinkedListGetItem(cparserlinkedlist_t *l)
{
	if (l == NULL)
		return NULL;

	return l->item;
}

void *LinkedListUpdateItem(cparserlinkedlist_t *l, void *item)
{
	void *res;

	if (l == NULL)
		return NULL;

	res = l->item;
	l->item = item;

	return res;
}
