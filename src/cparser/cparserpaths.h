/*
 * cparserpaths.h
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#ifndef CPARSERPATHS_H_
#define CPARSERPATHS_H_

#include <stdint.h>


namespace cparser {

class cparser_paths {

private:
	uint8_t **m_paths;
	uint32_t m_paths_size;
	uint32_t m_paths_count;

public:
	cparser_paths();
	virtual ~cparser_paths();

	void AddPath(const uint8_t *path);
	uint32_t GetPathsCount();
	const uint8_t * GetPathByIndex(uint32_t i);
	void DeletePathByIndex(uint32_t i);

};

} /* namespace cparser */

#endif /* CPARSERPATHS_H_ */
