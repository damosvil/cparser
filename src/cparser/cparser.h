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

public:
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

private:
	// Parsing states
	enum parsing_state_e
	{
		PARSING_STATE_IDLE = 0,
		PARSING_STATE_C_COMMENT = 1,
		PARSING_STATE_CPP_COMMENT = 2,
		PARSING_STATE_SENTENCE = 3
	};

	const cparser_paths *paths;
	const uint8_t *filename;

	object_s *AddChild(object_s *parent, object_type_e type, token_s *token);

public:
	cparser(const cparser_paths *paths, const uint8_t *filename);
	virtual ~cparser();

	object_s *Parse(object_s *current_object);

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
