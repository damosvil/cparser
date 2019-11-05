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

	// Parsers of included files
	cparser_block *block;

public:
	cparser(const uint8_t *filename, const cparser_paths *paths);
	virtual ~cparser();

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
