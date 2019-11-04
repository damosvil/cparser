/*
 * cparser.h
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#ifndef CPARSER_H_
#define CPARSER_H_


namespace cparser {

class cparser {

private:
	enum parsing_state_e
	{
		PARSING_STATE_NORMAL = 0,
		PARSING_STATE_SKIPPING_C_COMMENT = 1,
		PARSING_STATE_SKIPPING_CPP_COMMENT = 2
	};

public:
	cparser(const uint8_t *filename, const cparser_paths *paths);
	virtual ~cparser();

};

} /* namespace cparser */

#endif /* CPARSER_H_ */
