/*
 * cparserstack.h
 *
 *  Created on: 2 dic. 2019
 *      Author: iso9660
 */

#ifndef CPARSER_CPARSERSTACK_H_
#define CPARSER_CPARSERSTACK_H_


struct cparserstack_s;
typedef struct cparserstack_s cparserstack_t;


cparserstack_t *CParserStackNew(void);
void CParserStackDelete(cparserstack_t *s);
void CParserStackPushBytes(cparserstack_t *s, const void *data, size_t size);
bool CParserStackPopBytes(cparserstack_t *s, void *data, size_t size);


#endif /* CPARSER_CPARSERSTACK_H_ */
