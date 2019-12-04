/*
 * cparserpaths.h
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#ifndef CPARSERPATHS_H_
#define CPARSERPATHS_H_


struct cparserpaths_s;
typedef struct cparserpaths_s cparserpaths_t;


cparserpaths_t *PathsNew(void);
cparserpaths_t *PathsClone(cparserpaths_t *p);
void PathsDelete(cparserpaths_t *p);
void PathsAddPath(cparserpaths_t *p, const uint8_t *path);
uint32_t PathsGetPathsCount(cparserpaths_t *p);
const uint8_t * PathsGetPathByIndex(cparserpaths_t *p, uint32_t i);
FILE * PathsOpenFile(const cparserpaths_t *p, const uint8_t *filename, const uint8_t *mode);
void PathsDeletePathByIndex(cparserpaths_t *p, uint32_t i);


#endif /* CPARSERPATHS_H_ */
