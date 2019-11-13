/*
 * cparserpaths.h
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#ifndef CPARSERPATHS_H_
#define CPARSERPATHS_H_

#include <stdint.h>
#include <stdio.h>


namespace cparser {

class cparser_paths {

private:
	const uint8_t **m_paths;
	uint32_t m_paths_size;
	uint32_t m_paths_count;

public:
	cparser_paths();
	cparser_paths(const cparser_paths *p);
	virtual ~cparser_paths();

	void AddPath(const uint8_t *path);
	uint32_t GetPathsCount() const;
	const uint8_t * GetPathByIndex(uint32_t i) const;
	FILE * OpenFile(const uint8_t *filename, const uint8_t *mode) const;
	void DeletePathByIndex(uint32_t i);

};

} /* namespace cparser */

#endif /* CPARSERPATHS_H_ */
