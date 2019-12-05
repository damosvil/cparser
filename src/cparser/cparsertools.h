/*
 * cparsertools.h
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#ifndef CPARSERTOOLS_H_
#define CPARSERTOOLS_H_


/* String type handling */
#define _T	(uint8_t *)
#define _t  (char *)

/* Growth speed for arrays */
#define ARRAY_GROWTH_SPEED 			10

/* Maximum sentence lenght */
#define MAX_SENTENCE_LENGTH			(1 << 24)	// 16 Mb

/* String functions */
const bool StrEq(const char *u, const char *v);
bool IsCHeaderFilename(const uint8_t *filename);
bool IsCSourceFilename(const uint8_t *filename);
void AddToPtrArray(void *data, void ***p_array, uint32_t *p_size, uint32_t *p_count);


#endif /* CPARSERTOOLS_H_ */
