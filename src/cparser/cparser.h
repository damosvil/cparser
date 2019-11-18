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
		STATE_INCLUDE_FILENAME,
		STATE_DEFINE_IDENTIFIER,
		STATE_DEFINE_LITERAL,
		STATE_DATATYPE,
		STATE_PRAGMA,
		STATE_IDENTIFIER,
		STATE_ARRAY_DEFINITION,
		STATE_INITIALIZATION,
	};

	const cparser_paths *paths;
	const uint8_t *filename;

	static object_s * StateDatatypeProcessToken(object_s *oo, states_e &s, token_s *tt);
	static object_s * StateIdentifierProcessToken(object_s *oo, states_e &s, token_s *tt);
	static object_s * StateInitializationProcessToken(object_s *oo, states_e &s, token_s *tt);
	static object_s * StateArrayDefinitionProcessToken(object_s *oo, states_e &s, token_s *tt);

public:
	cparser(const cparser_paths *paths, const uint8_t *filename);
	virtual ~cparser();

	object_s *Parse(object_s *current_object);

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
