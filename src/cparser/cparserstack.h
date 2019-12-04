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


cparserstack_t *StackNew(void);
void StackDelete(cparserstack_t *s);
void StackPushBytes(cparserstack_t *s, const void *data, size_t size);
bool StackPopBytes(cparserstack_t *s, void *data, size_t size);


#endif /* CPARSER_CPARSERSTACK_H_ */
