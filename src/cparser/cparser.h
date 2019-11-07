/*
 * cparser.h
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#ifndef CPARSER_H_
#define CPARSER_H_


namespace cparser {

class cparser_block;

class cparser {

private:
	// Parsing states
	enum parsing_state_e
	{
		PARSING_STATE_IDLE = 0,
		PARSING_STATE_C_COMMENT = 1,
		PARSING_STATE_CPP_COMMENT = 2,
		PARSING_STATE_SENTENCE = 3
	};

	// Parse object type
	enum object_type_e
	{
		OBJECT_TYPE_C_COMMENT = 0,
		OBJECT_TYPE_CPP_COMMENT,
		OBJECT_TYPE_DEFINE,
		OBJECT_TYPE_DEFINE_IDENTIFIER,
		OBJECT_TYPE_DEFINE_BODY,
		OBJECT_TYPE_INCLUDE,
		OBJECT_TYPE_INCLUDE_FILENAME,
		OBJECT_TYPE_TYPEDEF,
		OBJECT_TYPE_ENUM,
		OBJECT_TYPE_STRUCT,
		OBJECT_TYPE_UNION,
		OBJECT_TYPE_VARIABLE_TYPE,
		OBJECT_TYPE_VARIABLE_IDENTIFIER,
		OBJECT_TYPE_FUNCTION_TYPE,
		OBJECT_TYPE_FUNCTION_IDENTIFIER,
		OBJECT_TYPE_PARAMETER_IDENTIFIER,
		OBJECT_TYPE_PARAMETER_TYPE,
		OBJECT_TYPE_IF,
		OBJECT_TYPE_IF_THEN,
		OBJECT_TYPE_IF_ELSE,
		OBJECT_TYPE_FOR,
		OBJECT_TYPE_WHILE,
		OBJECT_TYPE_DO_WHILE,
		OBJECT_TYPE_TAG,
		OBJECT_TYPE_GOTO,
		OBJECT_TYPE_EXPRESSION,
		OBJECT_TYPE_SOURCE_FILE,
		OBJECT_TYPE_HEADER_FILE,
		OBJECT_TYPE_ERROR,
	};

	// Parse object
	struct object_s
	{
		object_type_e type;

		const uint8_t *data;
		uint32_t row;
		uint32_t column;

		object_s *parent;
		object_s **children;
		uint32_t children_size;
		uint32_t children_count;

	};

	object_s *BeginChild(object_s *parent, object_type_e type, uint32_t row, uint32_t column);
	object_s *EndChild(object_s *child, const uint8_t *data);

public:
	cparser(const uint8_t *filename, const cparser_paths *paths);
	virtual ~cparser();

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
