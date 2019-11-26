/*
 * cparser.h
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#ifndef CPARSER_H_
#define CPARSER_H_


object_t *CParserParse(cparserdictionary_t *dictionary, cparserpaths_t *paths, const uint8_t *filename);


#endif /* CPARSER_H_ */
