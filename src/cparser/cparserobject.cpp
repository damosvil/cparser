/*
 * cparserobject.cpp
 *
 *  Created on: 15 nov. 2019
 *      Author: iso9660
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <cparsertools.h>
#include <cparsertoken.h>
#include <cparserobject.h>


namespace cparser {

object_s *ObjectAddChild(object_s *parent, object_type_e type, token_s *token)
{
	object_s *child = new object_s;

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
		child->data = _T StrDup(token->str);
	}

	// Add object to parent if it is not root node
	if (parent != NULL)
		AddToPtrArray(child, (void **&)parent->children, parent->children_size, parent->children_count);

	// Return children
	return child;
}

object_s *ObjectGetChildByType(object_s *parent, object_type_e type)
{
	for (uint32_t i = 0; i < parent->children_count; i++)
		if (parent->children[i]->type == type)
			return parent->children[i];

	return NULL;
}

object_s *ObjectGetLastChild(object_s *parent, object_type_e type)
{
	if (parent->children_count == 0)
		return NULL;

	return parent->children[parent->children_count - 1];
}


} /* namespace cgl */