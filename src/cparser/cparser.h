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
		OBJECT_TYPE_CPP_COMMENT = 1,
		OBJECT_TYPE_DEFINE = 2,
		OBJECT_TYPE_INCLUDE = 3,
		OBJECT_TYPE_ENUM = 4,
		OBJECT_TYPE_STRUCT = 5,
		OBJECT_TYPE_VARIABLE = 6,
		OBJECT_TYPE_FUNCTION = 7,
		OBJECT_TYPE_IF = 8,
		OBJECT_TYPE_IF_THEN = 9,
		OBJECT_TYPE_IF_ELSE = 10,
		OBJECT_TYPE_FOR = 11,
		OBJECT_TYPE_WHILE = 12,
		OBJECT_TYPE_DO_WHILE = 13,
		OBJECT_TYPE_TAG = 14,
		OBJECT_TYPE_GOTO = 15,
		OBJECT_TYPE_EXPRESSION = 16,
		OBJECT_TYPE_SOURCE_FILE = 17,
		OBJECT_TYPE_HEADER_FILE = 18
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

	object_s *BeginChildren(object_s *p, object_type_e t);
	object_s *EndChildren(object_s *c, const uint8_t *data);

public:
	cparser(const uint8_t *filename, const cparser_paths *paths);
	virtual ~cparser();

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
