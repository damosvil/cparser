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


struct cparserpaths_s;
typedef struct cparserpaths_s cparserpaths_t;


cparserpaths_t *CParserPathsNew(void);
cparserpaths_t *CParserPathsClone(cparserpaths_t *p);
void CParserPathsDelete(cparserpaths_t *p);
void CParserPathsAddPath(cparserpaths_t *p, const uint8_t *path);
uint32_t CParserPathsGetPathsCount(cparserpaths_t *p);
const uint8_t * CParserPathsGetPathByIndex(cparserpaths_t *p, uint32_t i);
FILE * CParserPathsOpenFile(const cparserpaths_t *p, const uint8_t *filename, const uint8_t *mode);
void CParserPathsDeletePathByIndex(cparserpaths_t *p, uint32_t i);


#endif /* CPARSERPATHS_H_ */
