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
	enum states_e
	{
		STATE_IDLE,
		STATE_PREPROCESSOR,
		STATE_INCLUDE,
		STATE_DEFINE,
		STATE_DEFINE_IDENTIFIER,
		STATE_DATATYPE,
		STATE_PRAGMA,
		STATE_IDENTIFIER,
		STATE_ARRAY_EXPRESSION,
	};

	const cparser_paths *paths;
	const uint8_t *filename;

	static object_s * AddTokenToDatatype(object_s *oo, states_e &s, token_s *tt);

public:
	cparser(const cparser_paths *paths, const uint8_t *filename);
	virtual ~cparser();

	object_s *Parse(object_s *current_object);

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
