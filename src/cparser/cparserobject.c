/*
 * cparserobject.cpp
 *
 *  Created on: 15 nov. 2019
 *      Author: iso9660
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cparsertools.h>
#include <cparsertoken.h>
#include <cparserobject.h>


object_t *ObjectAddChild(object_t *parent, object_type_t type, token_t *token)
{
	object_t *child = malloc(sizeof(object_t));

	// Initialize new object
	child->type = type;
	child->parent = parent;
	child->children = NULL;
	child->children_size = 0;
	child->children_count = 0;
	child->info = NULL;

	// Add token data if any
	if (token)
	{
		child->row = token->row;
		child->column = token->column;
		child->data = _T strdup(_t token->str);
	}
	else
	{
		child->row = 0;
		child->column = 0;
		child->data = NULL;
	}

	// Add object to parent if it is not root node
	if (parent != NULL)
		AddToPtrArray(child, &parent->children, &parent->children_size, &parent->children_count);

	// Return children
	return child;
}

object_t *ObjectGetChildByType(object_t *parent, object_type_t type)
{
	for (uint32_t i = 0; i < parent->children_count; i++)
		if (parent->children[i]->type == type)
			return parent->children[i];

	return NULL;
}

object_t *ObjectGetLastChild(object_t *parent, object_type_t type)
{
	if (parent->children_count == 0)
		return NULL;

	return parent->children[parent->children_count - 1];
}

object_t *ObjectGetParent(object_t *o)
{
	return o ? o->parent : NULL;
}
