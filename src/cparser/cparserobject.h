/*
 * cparserobject.h
 *
 *  Created on: 15 nov. 2019
 *      Author: iso9660
 */

#ifndef CPARSER_CPARSEROBJECT_H_
#define CPARSER_CPARSEROBJECT_H_


// Parse object type
typedef enum object_type_e
{
	OBJECT_TYPE_C_COMMENT = 0,
	OBJECT_TYPE_CPP_COMMENT,
	OBJECT_TYPE_DEFINE,
	OBJECT_TYPE_DEFINE_IDENTIFIER,
	OBJECT_TYPE_DEFINE_EXPRESSION,
	OBJECT_TYPE_PREPROCESSOR_DIRECTIVE,
	OBJECT_TYPE_PREPROCESSOR_IFNDEF,
	OBJECT_TYPE_PREPROCESSOR_IFDEF,
	OBJECT_TYPE_PREPROCESSOR_ELSE,
	OBJECT_TYPE_PREPROCESSOR_ENDIF,
	OBJECT_TYPE_PREPROCESSOR_ERROR,
	OBJECT_TYPE_INCLUDE,
	OBJECT_TYPE_INCLUDE_FILENAME,
	OBJECT_TYPE_INCLUDE_OBJECT,
	OBJECT_TYPE_SOURCE_FILE,
	OBJECT_TYPE_HEADER_FILE,
	OBJECT_TYPE_WARNING,
	OBJECT_TYPE_ERROR,
	OBJECT_TYPE_SPECIFIER,
	OBJECT_TYPE_QUALIFIER,
	OBJECT_TYPE_MODIFIER,
	OBJECT_TYPE_DATATYPE,
	OBJECT_TYPE_DATATYPE_PRIMITIVE,
	OBJECT_TYPE_DATATYPE_UNKNOWN,
	OBJECT_TYPE_DATATYPE_USER_DEFINED,
	OBJECT_TYPE_IDENTIFIER,
	OBJECT_TYPE_POINTER,
	OBJECT_TYPE_VARIABLE,
	OBJECT_TYPE_ARRAY_DEFINITION,
	OBJECT_TYPE_OPEN_SQ_BRACKET,
	OBJECT_TYPE_CLOSE_SQ_BRACKET,
	OBJECT_TYPE_EXPRESSION,
	OBJECT_TYPE_OPEN_PARENTHESYS,
	OBJECT_TYPE_CLOSE_PARENTHESYS,
	OBJECT_TYPE_EXPRESSION_TOKEN,
	OBJECT_TYPE_INITIALIZATION,
	OBJECT_TYPE_ASSIGNATION,
	OBJECT_TYPE_ARRAY_DATA,
	OBJECT_TYPE_OPEN_BRACKET,
	OBJECT_TYPE_CLOSE_BRACKET,
	OBJECT_TYPE_ARRAY_ITEM,
	OBJECT_TYPE_SENTENCE_END,
	OBJECT_TYPE_FUNCTION,
	OBJECT_TYPE_FUNCTION_DECLARATION,
	OBJECT_TYPE_FUNCTION_PARAMETERS,
	OBJECT_TYPE_PARAMETER,
	OBJECT_TYPE_PARAMETER_SEPARATOR,
	OBJECT_TYPE_FUNCTION_BODY,
	OBJECT_TYPE_UNION,
	OBJECT_TYPE_ENUM,
	OBJECT_TYPE_STRUCT,
	OBJECT_TYPE_TEMPORAL,
	OBJECT_TYPE_COUNT
} object_type_t;

// Parse object
typedef struct object_s
{
	object_type_t type;
	struct object_s *parent;
	struct object_s **children;
	uint32_t children_size;
	uint32_t children_count;

	uint32_t row;
	uint32_t column;
	uint8_t * data;
	uint8_t * info;
} object_t;

void ObjectAddChild(object_t *parent, object_t *child);
object_t *ObjectAddChildFromToken(object_t *parent, object_type_t type, token_t *token);
object_t *ObjectGetChildByType(object_t *parent, object_type_t type);
object_t *ObjectGetLastChild(object_t *parent, object_type_t type);
object_t *ObjectGetParent(object_t *o);
void ObjectPrint(object_t *o, uint32_t level);
void ObjectPrintRoot(object_t *o);


#endif /* CPARSER_CPARSEROBJECT_H_ */
