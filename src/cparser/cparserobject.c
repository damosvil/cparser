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


#define STR(A)	(#A)


static const char *object_type_names[OBJECT_TYPE_COUNT] =
{
		STR(OBJECT_TYPE_C_COMMENT),
		STR(OBJECT_TYPE_CPP_COMMENT),
		STR(OBJECT_TYPE_DEFINE),
		STR(OBJECT_TYPE_UNDEF),
		STR(OBJECT_TYPE_PREPROCESSOR_IDENTIFIER),
		STR(OBJECT_TYPE_PREPROCESSOR_EXPRESSION),
		STR(OBJECT_TYPE_PREPROCESSOR_DIRECTIVE),
		STR(OBJECT_TYPE_PREPROCESSOR_IF),
		STR(OBJECT_TYPE_PREPROCESSOR_IFDEF),
		STR(OBJECT_TYPE_PREPROCESSOR_IFNDEF),
		STR(OBJECT_TYPE_PREPROCESSOR_ELSE),
		STR(OBJECT_TYPE_PREPROCESSOR_ELIF),
		STR(OBJECT_TYPE_PREPROCESSOR_ENDIF),
		STR(OBJECT_TYPE_PREPROCESSOR_ERROR),
		STR(OBJECT_TYPE_INCLUDE),
		STR(OBJECT_TYPE_INCLUDE_FILENAME),
		STR(OBJECT_TYPE_INCLUDE_OBJECT),
		STR(OBJECT_TYPE_SOURCE_FILE),
		STR(OBJECT_TYPE_HEADER_FILE),
		STR(OBJECT_TYPE_WARNING),
		STR(OBJECT_TYPE_ERROR),
		STR(OBJECT_TYPE_SPECIFIER),
		STR(OBJECT_TYPE_QUALIFIER),
		STR(OBJECT_TYPE_MODIFIER),
		STR(OBJECT_TYPE_DATATYPE),
		STR(OBJECT_TYPE_DATATYPE_PRIMITIVE),
		STR(OBJECT_TYPE_DATATYPE_UNKNOWN),
		STR(OBJECT_TYPE_DATATYPE_USER_DEFINED),
		STR(OBJECT_TYPE_IDENTIFIER),
		STR(OBJECT_TYPE_POINTER),
		STR(OBJECT_TYPE_VARIABLE),
		STR(OBJECT_TYPE_ARRAY_DEFINITION),
		STR(OBJECT_TYPE_OPEN_SQ_BRACKET),
		STR(OBJECT_TYPE_CLOSE_SQ_BRACKET),
		STR(OBJECT_TYPE_EXPRESSION),
		STR(OBJECT_TYPE_OPEN_PARENTHESYS),
		STR(OBJECT_TYPE_CLOSE_PARENTHESYS),
		STR(OBJECT_TYPE_EXPRESSION_TOKEN),
		STR(OBJECT_TYPE_INITIALIZATION),
		STR(OBJECT_TYPE_ASSIGNATION),
		STR(OBJECT_TYPE_ARRAY_DATA),
		STR(OBJECT_TYPE_OPEN_BRACKET),
		STR(OBJECT_TYPE_CLOSE_BRACKET),
		STR(OBJECT_TYPE_ARRAY_ITEM),
		STR(OBJECT_TYPE_SENTENCE_END),
		STR(OBJECT_TYPE_FUNCTION),
		STR(OBJECT_TYPE_FUNCTION_DECLARATION),
		STR(OBJECT_TYPE_FUNCTION_PARAMETERS),
		STR(OBJECT_TYPE_PARAMETER),
		STR(OBJECT_TYPE_PARAMETER_SEPARATOR),
		STR(OBJECT_TYPE_FUNCTION_BODY),
		STR(OBJECT_TYPE_UNION),
		STR(OBJECT_TYPE_ENUM),
		STR(OBJECT_TYPE_STRUCT),
		STR(OBJECT_TYPE_TEMPORAL)
};


void ObjectAddChild(object_t *parent, object_t *child)
{
	// Add object to parent if it is not root node
	if (parent != NULL)
		AddToPtrArray(child, (void ***)&parent->children, &parent->children_size, &parent->children_count);
}

object_t *ObjectNewPreprocessorExpression(const uint8_t *expression)
{
	object_t *oo = malloc(sizeof(object_t));

	// Initialize new object
	oo->type = OBJECT_TYPE_PREPROCESSOR_EXPRESSION;
	oo->parent = NULL;
	oo->children = NULL;
	oo->children_size = 0;
	oo->children_count = 0;
	oo->info = NULL;
	oo->row = 0;
	oo->column = 0;
	oo->data = _T strdup(_t expression);

	// Return children
	return oo;
}

object_t *ObjectAddChildFromToken(object_t *parent, object_type_t type, token_t *token)
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

	// Add child
	ObjectAddChild(parent, child);

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

void ObjectPrint(object_t *o, uint32_t level)
{
	if (!o)
		return;

	printf("%*c<object type=\"%s\" row=\"%d\" column=\"%d\">\n", 4 * level, ' ', object_type_names[o->type], o->row, o->column);

	if (o->data && strlen(_t o->data))
	{
		printf("%*c<data>\n", 4 * (level + 1), ' ');
		printf("%*c%s\r\n", 4 * (level + 2), ' ', o->data);
		printf("%*c</data>\n", 4 * (level + 1), ' ');
	}

	if (o->info && strlen(_t o->info))
	{
		printf("%*c<info>\n", 4 * (level + 1), ' ');
		printf("%*c%s\n", 4 * (level + 2), ' ', o->info);
		printf("%*c</info>\n", 4 * (level + 1), ' ');
	}

	if (o->children_count)
	{
		printf("%*c<children>\n", 4 * (level + 1), ' ');
		for (uint32_t i = 0; i < o->children_count; i++)
		{
			ObjectPrint(o->children[i], level + 2);
		}
		printf("%*c</children>\n", 4 * (level + 1), ' ');
	}

	printf("%*c</object>\n", 4 * level, ' ');
}

void ObjectPrintRoot(object_t *o)
{
	while (o->parent)
		o = o->parent;

	ObjectPrint(o, 0);
}
