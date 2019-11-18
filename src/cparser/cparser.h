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
		STATE_FUNCTION_PARAMETERS,
		STATE_FUNCTION_DECLARED,
		STATE_ERROR
	};

	const cparser_paths *paths;
	const uint8_t *filename;

	static object_s * ProcessStateIdle(object_s *oo, states_e &s, token_s *tt, uint32_t &tokenizer_flags);
	static object_s * ProcessStatePreprocessor(object_s *oo, states_e &s, token_s *tt, uint32_t &tokenizer_flags);
	static object_s * ProcessStateIncludeFilename(object_s *oo, states_e &s, token_s *tt);
	static object_s * ProcessStateDefineIdentifier(object_s *oo, states_e &s, token_s *tt, uint32_t &tokenizer_flags);
	static object_s * ProcessStateDefineLiteral(object_s *oo, states_e &s, token_s *tt);
	static object_s * DigestDataType(object_s *oo, token_s *tt);
	static object_s * ProcessStateDatatype(object_s *oo, states_e &s, token_s *tt);
	static object_s * ProcessStateIdentifier(object_s *oo, states_e &s, token_s *tt);
	static object_s * ProcessStateInitialization(object_s *oo, states_e &s, token_s *tt);
	static object_s * ProcessStateArrayDefinition(object_s *oo, states_e &s, token_s *tt);
	static object_s * ProcessStateFunctionParameters(object_s *oo, states_e &s, token_s *tt);

public:
	cparser(const cparser_paths *paths, const uint8_t *filename);
	virtual ~cparser();

	object_s *Parse(object_s *current_object);

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
