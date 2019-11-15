/*
 * cparserobject.h
 *
 *  Created on: 15 nov. 2019
 *      Author: iso9660
 */

#ifndef CPARSER_CPARSEROBJECT_H_
#define CPARSER_CPARSEROBJECT_H_


namespace cparser {


// Parse object type
enum object_type_e
{
	OBJECT_TYPE_C_COMMENT = 0,
	OBJECT_TYPE_CPP_COMMENT,
	OBJECT_TYPE_DEFINE,
	OBJECT_TYPE_PREPROCESSOR_DIRECTIVE,
	OBJECT_TYPE_INCLUDE,
	OBJECT_TYPE_SOURCE_FILE,
	OBJECT_TYPE_HEADER_FILE,
	OBJECT_TYPE_WARNING,
	OBJECT_TYPE_ERROR,
	OBJECT_TYPE_DATATYPE,
	OBJECT_TYPE_SPECIFIER,
	OBJECT_TYPE_QUALIFIER,
	OBJECT_TYPE_MODIFIER,
	OBJECT_TYPE_DATATYPE_PRIMITIVE,
	OBJECT_TYPE_DATATYPE_UNKNOWN,
	OBJECT_TYPE_DATATYPE_DEFINED,
	OBJECT_TYPE_VARIABLE,
	OBJECT_TYPE_FUNCTION
};

// Parse object
struct object_s
{
	object_type_e type;
	object_s *parent;
	object_s **children;
	uint32_t children_size;
	uint32_t children_count;

	uint32_t row;
	uint32_t column;
	uint8_t * data;
	uint8_t * info;
};


object_s *ObjectAddChild(object_s *parent, object_type_e type, token_s *token);


} /* namespace cparser */

#endif /* CPARSER_CPARSEROBJECT_H_ */
